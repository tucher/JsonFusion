#pragma once

#include <iterator>
#include <utility>  // std::declval
#include <ranges>
#include <type_traits>
#include <limits>

#include "parse_errors.hpp"
#include "fp_to_str.hpp"

namespace JsonFusion {

namespace tokenizer {

enum class TryParseStatus {
    no_match,   // not our case, iterator unchanged
    ok,         // parsed and consumed
    error       // malformed, ctx already has error
};

template<class C>
concept TokenizerLike = true;



namespace detail {
template <typename T>
concept DynamicContainerTypeConcept = requires (T  v) {
    typename T::value_type;
    v.push_back(std::declval<typename T::value_type>());
    v.clear();
};
}

template<class It, class Sent, class DeserializationContext>
class JsonIteratorReader {
public:
    using iterator_type = It;
    using sentinel_type = Sent;

    constexpr JsonIteratorReader(It & first, const Sent & last, DeserializationContext & ctx)
        : current_(first), end_(last), ctx_(ctx) {}



    constexpr inline  TryParseStatus read_null() {
        if(atEnd())  {
            ctx_.setError(ParseError::UNEXPECTED_END_OF_DATA, current_);
            return TryParseStatus::error;
        }

        if (*current_ != 'n') {
            return TryParseStatus::no_match;
        }
        current_ ++;
        if (!match_literal("ull")) {
            ctx_.setError(ParseError::ILLFORMED_NULL, current_);
            return TryParseStatus::error;
        }
        return TryParseStatus::ok;
    }

    constexpr inline  TryParseStatus read_bool(bool & b) {
        if(atEnd())  {
            ctx_.setError(ParseError::UNEXPECTED_END_OF_DATA, current_);
            return TryParseStatus::error;
        }
        switch(*current_) {
        case 't': {
            current_ ++;
            if (match_literal("rue"))  {
                if(isPlainEnd(*current_) || atEnd()){
                    b = true;
                    return TryParseStatus::ok;
                }
            }
            ctx_.setError(ParseError::ILLFORMED_BOOL, current_);
            return TryParseStatus::error;
        }
        case 'f': {
            current_ ++;
            if (match_literal("alse"))  {
                if (isPlainEnd(*current_) || atEnd()){
                    b = false;
                    return TryParseStatus::ok;
                }
            }
            ctx_.setError(ParseError::ILLFORMED_BOOL, current_);
            return TryParseStatus::error;
        }
        default:
            return TryParseStatus::no_match;
        }
    }
    constexpr inline  bool skip_whitespaces_till_the_end() {
        while (current_ != end_ && isSpace(*current_)) {
            ++current_;
        }
        if (current_ != end_) {
            ctx_.setError(ParseError::EXCESS_CHARACTERS, current_);
            return false;
        }
        return true;
    }
    constexpr inline  bool skip_whitespaces_till_any() {
        while (current_ != end_ && isSpace(*current_)) {
            ++current_;
        }
        if (atEnd())   {
            ctx_.setError(ParseError::UNEXPECTED_END_OF_DATA, current_);
            return false;
        }
        return true;
    }

    // Array/object structural events
    constexpr inline  bool read_array_begin() {
        if(*current_ != '[')  {
            return false;

        }
        current_++;
        return skip_whitespace();
    }
    constexpr inline  bool read_object_begin() {
        if(*current_ != '{')  {
            return false;

        }
        current_++;
        return skip_whitespace();
    }

    // Try-peek style helpers
    constexpr inline  TryParseStatus read_array_end() {
        if(!skip_whitespace()) {
            return TryParseStatus::error;
        }
        if(*current_ == ']') {
            current_ ++;
            return TryParseStatus::ok;
        }
        return TryParseStatus::no_match;
    }
    constexpr inline  TryParseStatus read_object_end() {
        if(!skip_whitespace()) {
            return TryParseStatus::error;
        }
        if(*current_ == '}') {
            current_ ++;
            return TryParseStatus::ok;
        }
        return TryParseStatus::no_match;
    }

    // Comma handling
    constexpr inline  bool consume_value_separator(bool& had_comma) {
        had_comma = false;
        if(!skip_whitespace())  {
            return false;
        }
        if(*current_ == ',')  {
            current_ ++;
            had_comma = true;
        }
        if(!skip_whitespace()) {
            return false;
        }
        return true;
    }
    constexpr inline  bool consume_kv_separator() {
        if(!skip_whitespace()) {
            return false;
        }
        if(*current_ != ':') {
            ctx_.setError(ParseError::ILLFORMED_OBJECT, current_);
            return false;
        }
        current_ ++;
        if(!skip_whitespace()) {
            return false;
        }
        return true;
    }

    // Value parsing is handled by higher-level ParseValue(T&, Reader&, Ctx&),
    // so reader doesn’t know types; it just supplies char-level ops.
    constexpr inline  It current() const { return current_; }
    constexpr inline  Sent end()  const { return end_; }


    constexpr inline  bool read_number_token(char (&buf)[fp_to_str_detail::NumberBufSize],
                                     std::size_t& index,
                                     bool& seenDot,
                                     bool& seenExp)
    {
        index   = 0;
        seenDot = false;
        seenExp = false;

        bool inExp            = false;
        bool seenDigitBeforeExp = false;
        bool seenDigitAfterExp  = false;
        bool firstDigit       = true;  // Track first digit for leading zero check (RFC 8259)

        if (atEnd())   {
            ctx_.setError(ParseError::UNEXPECTED_END_OF_DATA, current_);
            return false;
        }

        // Optional leading '-'
        {
            char c = *current_;
            if (c == '-') {
                if (index >= fp_to_str_detail::NumberBufSize - 1) {
                    ctx_.setError(ParseError::ILLFORMED_NUMBER, current_);
                    return false;
                }
                buf[index++] = c;
                ++current_;
            }
        }

        if (atEnd())   {
            ctx_.setError(ParseError::UNEXPECTED_END_OF_DATA, current_);
            return false;
        }

        auto push_char = [&](char c) -> bool {
            if (index >= fp_to_str_detail::NumberBufSize - 1) {
                ctx_.setError(ParseError::ILLFORMED_NUMBER, current_);
                return false;
            }
            buf[index++] = c;
            return true;
        };

        while (true) {
            if(atEnd()) {
                break;
            }
            if(isPlainEnd(*current_)){
                break;
            }
            char c = *current_;

            if (c >= '0' && c <= '9') {
                // RFC 8259: Leading zeros not allowed (except "0" itself)
                if (firstDigit && c == '0') {
                    auto peek = current_;
                    ++peek;
                    if (peek != end_ && *peek >= '0' && *peek <= '9') {
                        ctx_.setError(ParseError::ILLFORMED_NUMBER, current_);
                        return false;
                    }
                }
                firstDigit = false;  // Mark that we've seen the first digit

                if (!inExp) {
                    seenDigitBeforeExp = true;
                } else {
                    seenDigitAfterExp = true;
                }
                if (!push_char(c)) return false;
                ++current_;
                continue;
            }

            if (c == '.' && !seenDot && !inExp) {
                seenDot = true;
                if (!push_char(c)) return false;
                ++current_;
                continue;
            }

            if ((c == 'e' || c == 'E') && !inExp) {
                inExp  = true;
                seenExp = true;
                if (!push_char(c)) return false;
                ++current_;

                // Optional sign immediately after exponent
                if (current_ != end_ && (*current_ == '+' || *current_ == '-')) {
                    if (!push_char(*current_)) return false;
                    ++current_;
                }
                continue;
            }

            // '+' or '-' is only allowed immediately after 'e'/'E', which we handled above
            // Anything else is invalid inside a JSON number
            ctx_.setError(ParseError::ILLFORMED_NUMBER, current_);
            return false;
        }

        buf[index] = '\0';

        // Basic sanity: must have at least one digit before exponent,
        // and if exponent present, at least one digit after it.
        if (!seenDigitBeforeExp || (seenExp && !seenDigitAfterExp && inExp)) {
            ctx_.setError(ParseError::ILLFORMED_NUMBER, current_);
            return false;
        }

        return true;
    }

    template<class NumberT, bool skipMaterializing>
    constexpr inline  TryParseStatus read_number(NumberT & storage) {
        char buf[fp_to_str_detail::NumberBufSize];
        std::size_t index = 0;
        bool seenDot = false;
        bool seenExp = false;

        if (!read_number_token(buf, index, seenDot, seenExp)) {
            // ctx_.setError(ParseError::WRONG_JSON_FOR_NUMBER_STORAGE, current_);
            return TryParseStatus::error;
        }
        if constexpr(skipMaterializing) {
            return TryParseStatus::ok;
        } else {
            if constexpr (std::is_integral_v<NumberT>) {
                // Reject decimals/exponents for integer fields
                if (seenDot || seenExp) {
                    return TryParseStatus::no_match;
                }

                NumberT value{};
                if(!parse_decimal_integer<NumberT>(buf, value)) {
                    ctx_.setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE, current_);
                    return TryParseStatus::error;
                }

                storage = value;
                return TryParseStatus::ok;
            } else if constexpr (std::is_floating_point_v<NumberT>) {
                double x;
                if(fp_to_str_detail::parse_number_to_double(buf, x)){
                    if(static_cast<double>(std::numeric_limits<NumberT>::lowest()) > x
                        || static_cast<double>(std::numeric_limits<NumberT>::max()) < x){
                        ctx_.setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE, current_);
                        return TryParseStatus::error;
                    }
                    storage = static_cast<NumberT>(x);
                    return TryParseStatus::ok;
                } else {
                    ctx_.setError(ParseError::ILLFORMED_NUMBER, current_);
                    return TryParseStatus::error;
                }
            } else {
                // Should never happen if JsonNumber is correct
                static_assert(std::is_integral_v<NumberT> || std::is_floating_point_v<NumberT>,
                              "[[[ JsonFusion ]]] JsonNumber underlying type must be integral or floating");
                return TryParseStatus::error;
            }
        }
    }




    template <class Visitor>
    constexpr inline  TryParseStatus read_string(Visitor&& inserter, bool continueOnInserterFailure = false) {
        bool stopInserting = false;

        if(*current_ != '"') {
            return TryParseStatus::no_match;
        }
        current_++;

        while(true) {
            if(atEnd())   {
                ctx_.setError(ParseError::UNEXPECTED_END_OF_DATA, current_);
                return TryParseStatus::error;
            }
            auto c = *current_;
            switch(c) {
            case '"': {
                current_ ++;

                return TryParseStatus::ok;
            }
            case '\\': {
                current_++;
                if(atEnd())  {
                    ctx_.setError(ParseError::UNEXPECTED_END_OF_DATA, current_);
                    return TryParseStatus::error;
                }

                char out{};
                switch (*current_) {
                /* Allowed escaped symbols */
                case '"':  out = '"';  break;
                case '/':  out = '/';  break;
                case '\\': out = '\\'; break;
                case 'b':  out = '\b'; break;
                case 'f':  out = '\f'; break;
                case 'r':  out = '\r'; break;
                case 'n':  out = '\n'; break;
                case 't':  out = '\t'; break;
                case 'u': {
                    ++current_; // move past 'u'

                    std::uint16_t u1 = 0;
                    if (!readHex4(u1)) {
                        return TryParseStatus::error; // ctx_ already set
                    }

                    std::uint32_t codepoint = 0;

                    // Handle surrogate pairs per JSON/UTF-16 rules
                    if (u1 >= 0xD800u && u1 <= 0xDBFFu) {
                        // High surrogate -> must be followed by \uDC00..DFFF
                        if (atEnd())  {
                            ctx_.setError(ParseError::UNEXPECTED_END_OF_DATA, current_);
                            return TryParseStatus::error;
                        }
                        if (*current_ != '\\') {
                            ctx_.setError(ParseError::ILLFORMED_STRING, current_);
                            return TryParseStatus::error;
                        }
                        ++current_;
                        if (atEnd())  {
                            ctx_.setError(ParseError::UNEXPECTED_END_OF_DATA, current_);
                            return TryParseStatus::error;
                        }
                        if (*current_ != 'u') {
                            ctx_.setError(ParseError::ILLFORMED_STRING, current_);
                            return TryParseStatus::error;
                        }
                        ++current_;

                        std::uint16_t u2 = 0;
                        if (!readHex4(u2)) {
                            return TryParseStatus::error;
                        }
                        if (u2 < 0xDC00u || u2 > 0xDFFFu) {
                            // Low surrogate not in valid range
                            ctx_.setError(ParseError::ILLFORMED_STRING, current_);
                            return TryParseStatus::error;
                        }

                        codepoint = 0x10000u
                                    + ((static_cast<std::uint32_t>(u1) - 0xD800u) << 10)
                                    + (static_cast<std::uint32_t>(u2) - 0xDC00u);
                    } else if (u1 >= 0xDC00u && u1 <= 0xDFFFu) {
                        // Lone low surrogate → invalid
                        ctx_.setError(ParseError::ILLFORMED_STRING, current_);
                        return TryParseStatus::error;
                    } else {
                        // Basic Multilingual Plane code point
                        codepoint = u1;
                    }

                    // Encode codepoint as UTF-8
                    char utf8[4];
                    int len = 0;

                    if (codepoint <= 0x7Fu) {
                        utf8[len++] = static_cast<char>(codepoint);
                    } else if (codepoint <= 0x7FFu) {
                        utf8[len++] = static_cast<char>(0xC0 | (codepoint >> 6));
                        utf8[len++] = static_cast<char>(0x80 | (codepoint & 0x3F));
                    } else if (codepoint <= 0xFFFFu) {
                        utf8[len++] = static_cast<char>(0xE0 | (codepoint >> 12));
                        utf8[len++] = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                        utf8[len++] = static_cast<char>(0x80 | (codepoint & 0x3F));
                    } else { // up to 0x10FFFF
                        utf8[len++] = static_cast<char>(0xF0 | (codepoint >> 18));
                        utf8[len++] = static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
                        utf8[len++] = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                        utf8[len++] = static_cast<char>(0x80 | (codepoint & 0x3F));
                    }

                    for (int i = 0; i < len; ++i) {
                        if (!stopInserting) {
                            if (!inserter(utf8[i])) {
                                if (continueOnInserterFailure) {
                                    stopInserting = true;
                                } else {
                                    // ctx_.setError(ParseError::FIXED_SIZE_CONTAINER_OVERFLOW, current_);
                                    return TryParseStatus::error;
                                }
                            }
                        }
                    }
                }
                break;

                default:
                    /* Unexpected symbol */
                    ctx_.setError(ParseError::ILLFORMED_STRING, current_);
                    return TryParseStatus::error;
                }
                if (out){ // only emit if we actually set a simple escape
                    if (!stopInserting) {
                        if (!inserter(out)) {
                            if (continueOnInserterFailure) {
                                stopInserting = true;
                            } else {
                                // ctx_.setError(ParseError::FIXED_SIZE_CONTAINER_OVERFLOW, current_);
                                return TryParseStatus::error;
                            }
                        }
                    }
                    current_++;
                }

            }
            break;

            default:
                // RFC 8259 §7: Control characters (U+0000-U+001F) MUST be escaped
                if (static_cast<unsigned char>(c) <= 0x1F) {
                    ctx_.setError(ParseError::ILLFORMED_STRING, current_);
                    return TryParseStatus::error;
                }
                if (!stopInserting) {
                    if(!inserter(*current_)) {
                        if (continueOnInserterFailure) {
                            stopInserting = true;
                        } else {
                            return TryParseStatus::error;
                        }
                    }
                }
                current_++;
            }
        }
    }


    template <std::size_t MAX_SKIP_NESTING, class OutputSinkContainer = void>
    constexpr bool skip_json_value(OutputSinkContainer * outputContainer = nullptr, std::size_t MaxStringLength = std::numeric_limits<std::size_t>::max()) {
        if (!skip_whitespace())  {
            return false;
        }

        std::size_t inserted = 0;
        auto sinkInserter = [this, &inserted, &outputContainer, &MaxStringLength](char c) -> bool {
            if constexpr (std::is_same_v<OutputSinkContainer, void>) {
                return true;
            } else {
                if constexpr (!detail::DynamicContainerTypeConcept<OutputSinkContainer>) {
                    if (inserted < outputContainer->size()-1) {
                        (*outputContainer)[inserted] = c;
                    } else {
                        ctx_.setError(ParseError::FIXED_SIZE_CONTAINER_OVERFLOW, current_);
                        return false;
                    }
                }  else {
                    outputContainer->push_back(c);
                }
                inserted++;
                if(inserted >= MaxStringLength) {
                    ctx_.setError(ParseError::JSON_SINK_OVERFLOW, current_);
                    return false;
                }
                return true;
            }
        };
        // Helper: skip a JSON literal (true/false/null), mirroring chars to sink.
        auto skipLiteral = [&] (const char* lit,
                               std::size_t len,
                               ParseError err) -> bool
        {
            for (std::size_t i = 0; i < len; ++i) {
                if (atEnd())  {
                    ctx_.setError(ParseError::UNEXPECTED_END_OF_DATA, current_);
                    return false;
                }

                if (*current_ != lit[i]) {
                    ctx_.setError(err, current_);
                    return false;
                }
                if (!sinkInserter(*current_)) {
                    return false;
                }
                ++current_;
            }
            return true;
        };

        // Helper: skip a number-like token, mirroring chars to sink.
        auto skipNumberLike = [&]() -> bool {
            while (!isPlainEnd(*current_)) {
                if(atEnd())  {
                    break;
                }

                if (!sinkInserter(*current_)) {
                    return false;
                }
                ++current_;
            }
            return true;
        };

        char c = *current_;

        // 1) Simple values we can skip without nesting

        if (c == '"') {
            // string: let parseString drive the iterator, but use sinkInserter
            return read_string(sinkInserter) == TryParseStatus::ok;
        }

        if (c == 't') {
            // true
            return skipLiteral("true", 4, ParseError::ILLFORMED_BOOL);
        }
        if (c == 'f') {
            // false
            return skipLiteral("false", 5, ParseError::ILLFORMED_BOOL);
        }
        if (c == 'n') {
            // null
            return skipLiteral("null", 4, ParseError::ILLFORMED_NULL);
        }

        // number-like: skip until a plain end (whitespace, ',', ']', '}', etc.)
        if (c != '{' && c != '[') {
            // neither object nor array → treat as number-like token
            return skipNumberLike();
        }

        // 2) Compound value: object or array with possible nesting.
        // We use an explicit stack of expected closing delimiters to avoid recursion.

        char stack[MAX_SKIP_NESTING];
        int  depth = 0;

        auto push_close = [&](char open) -> bool {
            if (depth >= static_cast<int>(MAX_SKIP_NESTING)) {
                ctx_.setError(ParseError::SKIPPING_STACK_OVERFLOW, current_);
                return false;
            }
            stack[depth++] = (open == '{') ? '}' : ']';
            return true;
        };

        auto pop_close = [&](char close) -> bool {
            if (depth == 0)  {
                ctx_.setError(ParseError::ILLFORMED_OBJECT, current_);
                return false;
            }
            if (stack[depth - 1] != close) {
                ctx_.setError(ParseError::ILLFORMED_OBJECT, current_);
                return false;
            }
            --depth;
            return true;
        };

        // Initialize with the first '{' or '['
        if (!push_close(c))  {
            return false;
        }
        // mirror the opening delimiter
        if (!sinkInserter(c)) {
            return false;
        }
        ++current_; // skip first '{' or '['

        while (current_ != end_ && depth > 0) {
            char ch = *current_;

            // Skip whitespace cheaply; you can decide whether to mirror it or not.
            if (isSpace(ch)) {
                ++current_;
                continue;
            }

            switch (ch) {
            case '"': {
                // mirrors the entire string via sinkInserter
                if (read_string(sinkInserter) != TryParseStatus::ok) {
                    return false;
                }
                break;
            }
            case '{':
            case '[': {
                if (!push_close(ch)) {
                    return false;
                }
                if (!sinkInserter(ch))  {
                    return false;
                }
                ++current_;
                break;
            }
            case '}':
            case ']': {
                if (!pop_close(ch) || !sinkInserter(ch)) {
                    return false;
                }

                ++current_;
                break;
            }
            case 't': {
                if (!skipLiteral("true", 4, ParseError::ILLFORMED_BOOL)) {
                    return false;
                }
                break;
            }
            case 'f': {
                if (!skipLiteral("false", 5, ParseError::ILLFORMED_BOOL))  {
                    return false;
                }
                break;
            }
            case 'n': {
                if (!skipLiteral("null", 4, ParseError::ILLFORMED_NULL))  {
                    return false;
                }
                break;
            }
            default: {
                // number-like or punctuation (':', ',', etc.)
                if ((ch >= '0' && ch <= '9') || ch == '-' || ch == '+') {
                    if (!skipNumberLike()) {
                        return false;
                    }
                } else {
                    // punctuation: mirror and advance
                    if (!sinkInserter(ch))  {
                        return false;
                    }
                    ++current_;
                }
                break;
            }
            }
        }

        if (depth != 0)  {
            ctx_.setError(ParseError::UNEXPECTED_END_OF_DATA, current_);
            return false;
        }

        // null-terminate fixed-size buffers if we used one
        if constexpr (!std::is_same_v<OutputSinkContainer, void>) {
            if constexpr (!detail::DynamicContainerTypeConcept<OutputSinkContainer>) {
                if (outputContainer && inserted < outputContainer->size()) {
                    (*outputContainer)[inserted] = '\0';
                }
            }
        }

        return true;
    }
private:
    It & current_;
    const Sent & end_;
    DeserializationContext& ctx_;


    constexpr inline bool atEnd() {
        return current_ == end_;
    }
    constexpr inline bool isSpace(char a) {
        switch(a) {
        case 0x20:
        case 0x0A:
        case 0x0D:
        case 0x09:
            return true;
        }
        return false;
    }

    constexpr inline bool isPlainEnd(char a) {
        switch(a) {
        case ']':
        case ',':
        case '}':
        case 0x20:
        case 0x0A:
        case 0x0D:
        case 0x09:
            return true;
        }
        return false;
    }

    // Basic whitespace skipping for all callers
    constexpr inline  bool skip_whitespace() {
        while(isSpace(*current_)) {
            if (atEnd())  {
                ctx_.setError(ParseError::UNEXPECTED_END_OF_DATA, current_);
                return false;
            }
            ++current_;
        }


        return true;
    }

    constexpr inline  bool match_literal(const std::string_view & lit) {
        for (char c : lit) {
            if (atEnd())  {
                ctx_.setError(ParseError::UNEXPECTED_END_OF_DATA, current_);
                return false;
            }
            if (*current_ != c)  {
                return false;
            }
            ++current_;
        }
        return true;
    }

    // -------------------------
    //  Parse decimal integer
    // -------------------------
    // buf: null-terminated ASCII, optional leading '+'/'-' then digits.
    // Assumes characters are already known to be digits (no extra validation).
    // Returns false on overflow or invalid '-' for unsigned types.
    template <class Int>
    constexpr inline bool parse_decimal_integer(const char* buf, Int& out) noexcept {
        static_assert(std::is_integral_v<Int>, "[[[ JsonFusion ]]] Int must be an integral type");

        using Limits   = std::numeric_limits<Int>;
        using Unsigned = std::make_unsigned_t<Int>;

        // const unsigned char* p = reinterpret_cast<const unsigned char*>(buf);
        const char *p = buf;
        bool negative = false;

        if constexpr (std::is_signed_v<Int>) {
            if (*p == '+' || *p == '-') {
                negative = (*p == '-');
                ++p;
            }
        } else {
            // Unsigned: allow leading '+' but reject '-'
            if (*p == '+') {
                ++p;
            } else if (*p == '-') {
                return false; // negative value for unsigned -> overflow/error
            }
        }

        Unsigned value = 0;

        // Absolute-value limit we must not exceed while parsing
        Unsigned limit;
        if constexpr (std::is_signed_v<Int>) {
            // For negative numbers we allow up to |min()| = max()+1
            limit = negative ? Unsigned(Limits::max()) + 1u
                             : Unsigned(Limits::max());
        } else {
            limit = std::numeric_limits<Unsigned>::max();
        }

        for (; *p != 0; ++p) {
            // We assume *p is '0'..'9'
            unsigned digit = static_cast<unsigned>(*p - '0');

            // Check overflow: value*10 + digit <= limit
            if (value > (limit - digit) / 10u)  {
                return false; // overflow
            }
            value = value * 10u + static_cast<Unsigned>(digit);
        }

        if constexpr (std::is_signed_v<Int>) {
            if (negative) {
                // Special-case: most-negative value
                if (value == Unsigned(Limits::max()) + 1u) {
                    out = Limits::min();
                } else {
                    out = static_cast<Int>(-static_cast<Int>(value));
                }
            } else {
                out = static_cast<Int>(value);
            }
        } else {
            out = static_cast<Int>(value);
        }

        return true;
    }

    constexpr inline  bool readHex4(std::uint16_t &out) {
        out = 0;
        for (int i = 0; i < 4; ++i) {
            if (atEnd()) {
                ctx_.setError(ParseError::UNEXPECTED_END_OF_DATA, current_);
                return false;
            }
            char currChar = *current_;
            std::uint8_t v;
            if (currChar >= '0' && currChar <= '9') {
                v = static_cast<std::uint8_t>(currChar - '0');
            } else if (currChar >= 'A' && currChar <= 'F') {
                v = static_cast<std::uint8_t>(currChar - 'A' + 10);
            } else if (currChar >= 'a' && currChar <= 'f') {
                v = static_cast<std::uint8_t>(currChar - 'a' + 10);
            } else  {
                ctx_.setError(ParseError::ILLFORMED_STRING, current_);
                return false;
            }
            out = static_cast<std::uint16_t>((out << 4) | v);
            ++current_;
        }
        return true;
    }
};
}

} // namespace JsonFusion
