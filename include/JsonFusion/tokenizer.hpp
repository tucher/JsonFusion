#pragma once

#include <limits>

#include "parse_errors.hpp"
#include "fp_to_str.hpp"
#include "reader_concept.hpp"

namespace JsonFusion {

namespace tokenizer {



namespace detail {
template <typename T>
concept DynamicContainerTypeConcept = requires (T  v) {
    typename T::value_type;
    v.push_back(std::declval<typename T::value_type>());
    v.clear();
};
}

template<class It, class Sent>
class JsonIteratorReader {
public:
    using iterator_type = It;
    using sentinel_type = Sent;

    constexpr JsonIteratorReader(It & first, const Sent & last)
        : current_(first), end_(last), m_error(ParseError::NO_ERROR) {}



    constexpr  TryParseStatus skip_ws_and_read_null() {
        while (current_ != end_ && isSpace(*current_)) {
            ++current_;
        }
        if(atEnd())  {
            setError(ParseError::UNEXPECTED_END_OF_DATA, current_);
            return TryParseStatus::error;
        }

        if (*current_ != 'n') {
            return TryParseStatus::no_match;
        }
        current_ ++;
        if (!match_literal("ull")) {
            setError(ParseError::ILLFORMED_NULL, current_);
            return TryParseStatus::error;
        }
        return TryParseStatus::ok;
    }

    constexpr  TryParseStatus read_bool(bool & b) {
        if(atEnd())  {
            setError(ParseError::UNEXPECTED_END_OF_DATA, current_);
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
            setError(ParseError::ILLFORMED_BOOL, current_);
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
            setError(ParseError::ILLFORMED_BOOL, current_);
            return TryParseStatus::error;
        }
        default:
            return TryParseStatus::no_match;
        }
    }
    constexpr  bool skip_whitespaces_till_the_end() {
        while (current_ != end_ && isSpace(*current_)) {
            ++current_;
        }
        if (current_ != end_) {
            setError(ParseError::EXCESS_CHARACTERS, current_);
            return false;
        }
        return true;
    }


    // Array/object structural events
    constexpr  bool read_array_begin() {
        if(*current_ != '[')  {
            return false;

        }
        current_++;
        return skip_whitespace();
    }
    constexpr  bool read_object_begin() {
        if(*current_ != '{')  {
            return false;

        }
        current_++;
        return skip_whitespace();
    }

    // Try-peek style helpers
    constexpr  TryParseStatus read_array_end() {
        if(!skip_whitespace()) {
            return TryParseStatus::error;
        }
        if(*current_ == ']') {
            current_ ++;
            return TryParseStatus::ok;
        }
        return TryParseStatus::no_match;
    }
    constexpr  TryParseStatus read_object_end() {
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
    constexpr  bool consume_value_separator(bool& had_comma) {
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
    constexpr  bool consume_kv_separator() {
        if(!skip_whitespace()) {
            return false;
        }
        if(*current_ != ':') {
            setError(ParseError::ILLFORMED_OBJECT, current_);
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
    constexpr  It current() const { return current_; }
    constexpr  Sent end()  const { return end_; }




    template<class NumberT, bool skipMaterializing>
    constexpr  TryParseStatus read_number(NumberT & storage) {
        char buf[fp_to_str_detail::NumberBufSize];
        std::size_t index = 0;
        bool seenDot = false;
        bool seenExp = false;

        if (!read_number_token(buf, index, seenDot, seenExp)) {
            // setError(ParseError::WRONG_JSON_FOR_NUMBER_STORAGE, current_);
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
                    setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE, current_);
                    return TryParseStatus::error;
                }

                storage = value;
                return TryParseStatus::ok;
            } else if constexpr (std::is_floating_point_v<NumberT>) {
                double x;
                if(fp_to_str_detail::parse_number_to_double(buf, x)){
                    if(static_cast<double>(std::numeric_limits<NumberT>::lowest()) > x
                        || static_cast<double>(std::numeric_limits<NumberT>::max()) < x){
                        setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE, current_);
                        return TryParseStatus::error;
                    }
                    storage = static_cast<NumberT>(x);
                    return TryParseStatus::ok;
                } else {
                    setError(ParseError::ILLFORMED_NUMBER, current_);
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

    constexpr StringChunkResult read_string_chunk(char* out, std::size_t capacity) {
        std::size_t written = 0;

        // If we're not currently inside a string, expect an opening quote
        if (!in_string_) {
            if (atEnd()) {
                setError(ParseError::UNEXPECTED_END_OF_DATA, current_);
                return {StringChunkStatus::error, 0, false};
            }
            if (*current_ != '"') {
                return {StringChunkStatus::no_match, 0, false};
            }
            in_string_ = true;
            ++current_; // consume opening '"'
            // No output yet, fall through into main loop
        }

        while (true) {
            // 1) Flush any buffered UTF-8 bytes (from previous \uXXXX)
            while (string_buf_pos_ < string_buf_len_ && written < capacity) {
                out[written++] = string_buf_[string_buf_pos_++];
            }

            // If we filled the caller's buffer, stop here
            if (written == capacity) {
                if(*current_ == '"') {
                    ++current_;
                    in_string_      = false;
                    string_buf_len_ = 0;
                    string_buf_pos_ = 0;
                    return {StringChunkStatus::ok, written, true};
                } else
                    return {StringChunkStatus::ok, written, false};
            }

            // No buffered bytes left, read next raw char
            if (atEnd()) {
                setError(ParseError::UNEXPECTED_END_OF_DATA, current_);
                in_string_      = false;
                string_buf_len_ = 0;
                string_buf_pos_ = 0;
                return {StringChunkStatus::error, written, false};
            }

            char c = *current_;

            switch (c) {
            case '"': {
                // End of string
                ++current_;
                in_string_      = false;
                string_buf_len_ = 0;
                string_buf_pos_ = 0;
                return {StringChunkStatus::ok, written, true};
            }

            case '\\': {
                // Escape sequence
                ++current_;
                if (atEnd()) {
                    setError(ParseError::UNEXPECTED_END_OF_DATA, current_);
                    in_string_ = false;
                    return {StringChunkStatus::error, written, false};
                }

                char esc = *current_;
                ++current_;

                // Simple 1-byte escapes
                char simple = 0;
                switch (esc) {
                case '"':  simple = '"';  break;
                case '/':  simple = '/';  break;
                case '\\': simple = '\\'; break;
                case 'b':  simple = '\b'; break;
                case 'f':  simple = '\f'; break;
                case 'r':  simple = '\r'; break;
                case 'n':  simple = '\n'; break;
                case 't':  simple = '\t'; break;
                case 'u':  break; // handled below
                default:
                    setError(ParseError::ILLFORMED_STRING, current_);
                    in_string_ = false;
                    return {StringChunkStatus::error, written, false};
                }

                if (esc != 'u') {
                    // Simple escape → either write now or buffer it
                    if (written < capacity) {
                        out[written++] = simple;
                    } else {
                        // no room this chunk: buffer for next time
                        string_buf_len_ = 1;
                        string_buf_pos_ = 0;
                        string_buf_[0]  = simple;
                        return {StringChunkStatus::ok, written, false};
                    }
                    break; // continue main loop
                }

                // ---- \uXXXX handling ----
                std::uint16_t u1 = 0;
                if (!readHex4(u1)) {
                    in_string_ = false;
                    return {StringChunkStatus::error, written, false};
                }

                std::uint32_t codepoint = 0;

                if (u1 >= 0xD800u && u1 <= 0xDBFFu) {
                    // High surrogate, expect a second \uXXXX
                    if (atEnd()) {
                        setError(ParseError::UNEXPECTED_END_OF_DATA, current_);
                        in_string_ = false;
                        return {StringChunkStatus::error, written, false};
                    }
                    if (*current_ != '\\') {
                        setError(ParseError::ILLFORMED_STRING, current_);
                        in_string_ = false;
                        return {StringChunkStatus::error, written, false};
                    }
                    ++current_;
                    if (atEnd()) {
                        setError(ParseError::UNEXPECTED_END_OF_DATA, current_);
                        in_string_ = false;
                        return {StringChunkStatus::error, written, false};
                    }
                    if (*current_ != 'u') {
                        setError(ParseError::ILLFORMED_STRING, current_);
                        in_string_ = false;
                        return {StringChunkStatus::error, written, false};
                    }
                    ++current_;

                    std::uint16_t u2 = 0;
                    if (!readHex4(u2)) {
                        in_string_ = false;
                        return {StringChunkStatus::error, written, false};
                    }
                    if (u2 < 0xDC00u || u2 > 0xDFFFu) {
                        setError(ParseError::ILLFORMED_STRING, current_);
                        in_string_ = false;
                        return {StringChunkStatus::error, written, false};
                    }

                    codepoint = 0x10000u
                                + ((static_cast<std::uint32_t>(u1) - 0xD800u) << 10)
                                + (static_cast<std::uint32_t>(u2) - 0xDC00u);
                } else if (u1 >= 0xDC00u && u1 <= 0xDFFFu) {
                    // Lone low surrogate → invalid
                    setError(ParseError::ILLFORMED_STRING, current_);
                    in_string_ = false;
                    return {StringChunkStatus::error, written, false};
                } else {
                    codepoint = u1;
                }

                // Encode codepoint as UTF-8 into internal buffer
                string_buf_len_ = 0;
                string_buf_pos_ = 0;

                if (codepoint <= 0x7Fu) {
                    string_buf_[string_buf_len_++] = static_cast<char>(codepoint);
                } else if (codepoint <= 0x7FFu) {
                    string_buf_[string_buf_len_++] = static_cast<char>(0xC0 | (codepoint >> 6));
                    string_buf_[string_buf_len_++] = static_cast<char>(0x80 | (codepoint & 0x3F));
                } else if (codepoint <= 0xFFFFu) {
                    string_buf_[string_buf_len_++] = static_cast<char>(0xE0 | (codepoint >> 12));
                    string_buf_[string_buf_len_++] = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                    string_buf_[string_buf_len_++] = static_cast<char>(0x80 | (codepoint & 0x3F));
                } else { // up to 0x10FFFF
                    string_buf_[string_buf_len_++] = static_cast<char>(0xF0 | (codepoint >> 18));
                    string_buf_[string_buf_len_++] = static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
                    string_buf_[string_buf_len_++] = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                    string_buf_[string_buf_len_++] = static_cast<char>(0x80 | (codepoint & 0x3F));
                }

                // Next loop iteration will immediately flush from string_buf_
                break;
            }

            default:
                // RFC 8259 §7: Control chars U+0000–U+001F must be escaped
                if (static_cast<unsigned char>(c) <= 0x1F) {
                    setError(ParseError::ILLFORMED_STRING, current_);
                    in_string_ = false;
                    return {StringChunkStatus::error, written, false};
                }

                // Normal unescaped char
                if (written == capacity) {
                    // no space; defer this char to next chunk
                    return {StringChunkStatus::ok, written, false};
                }
                out[written++] = c;
                ++current_;
                break;
            } // switch
        }     // while
    }


    template <std::size_t MAX_SKIP_NESTING, class OutputSinkContainer = void>
    constexpr bool skip_json_value(OutputSinkContainer * outputContainer = nullptr, std::size_t MaxStringLength = std::numeric_limits<std::size_t>::max()) {
        if constexpr(std::is_same_v<OutputSinkContainer, void>) {
            NoOpFiller filler{};
            return skip_json_value_internal<MAX_SKIP_NESTING>(filler);
        } else if constexpr(detail::DynamicContainerTypeConcept<OutputSinkContainer>) {
            DynContainerFiller filler{outputContainer, MaxStringLength};
            outputContainer->clear();
            return skip_json_value_internal<MAX_SKIP_NESTING>(filler);
        } else {
            StContainerFiller filler{outputContainer->data(), std::min(MaxStringLength, outputContainer->size())};
            auto r = skip_json_value_internal<MAX_SKIP_NESTING>(filler);

            return r;
        }
    }
    constexpr ParseError getError() {
        return m_error;
    }
private:
    constexpr  bool read_number_token(char (&buf)[fp_to_str_detail::NumberBufSize],
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
            setError(ParseError::UNEXPECTED_END_OF_DATA, current_);
            return false;
        }

        // Optional leading '-'
        {
            char c = *current_;
            if (c == '-') {
                if (index >= fp_to_str_detail::NumberBufSize - 1) {
                    setError(ParseError::ILLFORMED_NUMBER, current_);
                    return false;
                }
                buf[index++] = c;
                ++current_;
            }
        }

        if (atEnd())   {
            setError(ParseError::UNEXPECTED_END_OF_DATA, current_);
            return false;
        }

        auto push_char = [&](char c) -> bool {
            if (index >= fp_to_str_detail::NumberBufSize - 1) {
                setError(ParseError::ILLFORMED_NUMBER, current_);
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
                        setError(ParseError::ILLFORMED_NUMBER, current_);
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
            setError(ParseError::ILLFORMED_NUMBER, current_);
            return false;
        }

        buf[index] = '\0';

        // Basic sanity: must have at least one digit before exponent,
        // and if exponent present, at least one digit after it.
        if (!seenDigitBeforeExp || (seenExp && !seenDigitAfterExp && inExp)) {
            setError(ParseError::ILLFORMED_NUMBER, current_);
            return false;
        }

        return true;
    }
    template <std::size_t MAX_SKIP_NESTING, class Filler>
    constexpr bool skip_json_value_internal(Filler &filler, std::size_t MaxStringLength = std::numeric_limits<std::size_t>::max()) {
        if (!skip_whitespace())  {
            return false;
        }


        // Helper: skip a JSON literal (true/false/null), mirroring chars to sink.
        auto skipLiteral = [&] (const char* lit,
                               std::size_t len,
                               ParseError err) -> bool
        {
            for (std::size_t i = 0; i < len; ++i) {
                if (atEnd())  {
                    setError(ParseError::UNEXPECTED_END_OF_DATA, current_);
                    return false;
                }

                if (*current_ != lit[i]) {
                    setError(err, current_);
                    return false;
                }
                if (!filler(*current_)) {
                    setError(ParseError::JSON_SINK_OVERFLOW, current_);
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

                if (!filler(*current_)) {
                    setError(ParseError::JSON_SINK_OVERFLOW, current_);
                    return false;
                }
                ++current_;
            }
            return true;
        };

        char c = *current_;

        // 1) Simple values we can skip without nesting

        if (c == '"') {

            return read_string_with_filler(filler);
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
                setError(ParseError::SKIPPING_STACK_OVERFLOW, current_);
                return false;
            }
            stack[depth++] = (open == '{') ? '}' : ']';
            return true;
        };

        auto pop_close = [&](char close) -> bool {
            if (depth == 0)  {
                setError(ParseError::ILLFORMED_OBJECT, current_);
                return false;
            }
            if (stack[depth - 1] != close) {
                setError(ParseError::ILLFORMED_OBJECT, current_);
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
        if (!filler(c)) {
            setError(ParseError::JSON_SINK_OVERFLOW, current_);
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
                if (!read_string_with_filler(filler) ) {
                    return false;
                }
                break;
            }
            case '{':
            case '[': {
                if (!push_close(ch)) {
                    return false;
                }
                if (!filler(ch))  {
                    setError(ParseError::JSON_SINK_OVERFLOW, current_);
                    return false;
                }
                ++current_;
                break;
            }
            case '}':
            case ']': {
                if (!pop_close(ch)) {
                    return false;
                }
                if(!filler(ch)) {
                    setError(ParseError::JSON_SINK_OVERFLOW, current_);
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
                    if (!filler(ch))  {
                        setError(ParseError::JSON_SINK_OVERFLOW, current_);
                        return false;
                    }
                    ++current_;
                }
                break;
            }
            }
        }

        if (depth != 0)  {
            setError(ParseError::UNEXPECTED_END_OF_DATA, current_);
            return false;
        }

        filler.finish();

        return true;
    }

    ParseError m_error;
    It & current_;
    It m_errorPos;
    const Sent & end_;



    char   string_buf_[4]   = {}; // temp buffer for escapes / UTF-8
    int    string_buf_len_  = 0;
    int    string_buf_pos_  = 0;
    bool   in_string_       = false;

    constexpr void setError(ParseError e, It pos) {
        m_error = e;
        m_errorPos = pos;
    }



    constexpr bool atEnd() {
        return current_ == end_;
    }
    constexpr bool isSpace(char a) {
        switch(a) {
        case 0x20:
        case 0x0A:
        case 0x0D:
        case 0x09:
            return true;
        }
        return false;
    }

    constexpr bool isPlainEnd(char a) {
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
    constexpr  bool skip_whitespace() {
        while(isSpace(*current_)) {
            if (atEnd())  {
                setError(ParseError::UNEXPECTED_END_OF_DATA, current_);
                return false;
            }
            ++current_;
        }


        return true;
    }

    constexpr  bool match_literal(const std::string_view & lit) {
        for (char c : lit) {
            if (atEnd())  {
                setError(ParseError::UNEXPECTED_END_OF_DATA, current_);
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
    constexpr bool parse_decimal_integer(const char* buf, Int& out) noexcept {
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

    constexpr  bool readHex4(std::uint16_t &out) {
        out = 0;
        for (int i = 0; i < 4; ++i) {
            if (atEnd()) {
                setError(ParseError::UNEXPECTED_END_OF_DATA, current_);
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
                setError(ParseError::ILLFORMED_STRING, current_);
                return false;
            }
            out = static_cast<std::uint16_t>((out << 4) | v);
            ++current_;
        }
        return true;
    }

    template<class Cont>
    struct DynContainerFiller {
        Cont * out;
        std::size_t max_size = 0;
        std::size_t inserted = 0;

        constexpr bool operator()(char ch) {
            if (inserted >= max_size) {
                return false;
            }
            out->push_back(ch);
            inserted++;
            return true;
        }
        constexpr void finish(){

        }
    };

    struct StContainerFiller {
        char * out = nullptr;
        std::size_t max_size = 0;
        std::size_t inserted = 0;
        constexpr bool operator()(char ch) {
            if (inserted >= max_size - 1) {
                return false;
            }
            out[inserted] = ch;
            inserted ++;
            return true;
        }
        constexpr void finish(){
            out[inserted] = 0;
        }
    };
    struct NoOpFiller {
        void * out = nullptr;
        std::size_t max_size = std::numeric_limits<std::size_t>::max();
        std::size_t inserted = 0;
        constexpr bool operator()(char ch) {
            inserted ++;
            return true;
        }
        constexpr void finish(){}
    };

    template<class Filler>
    constexpr bool read_string_with_filler(Filler & f) {
        std::size_t added = 0;
        f('"');
        bool r = read_json_string_into(f.out, f.inserted, f.max_size - f.inserted, added);
        if(r) {
            f.inserted += added;
            f('"');
        }
        return r;
    }
    static constexpr std::size_t STRING_CHUNK_SIZE = 64;

    template<class Cont>
    constexpr bool read_json_string_into(
        Cont * out,
        std::size_t initS,
        std::size_t  max_len,  // validator limit or SIZE_MAX
        std::size_t & outSize
        )
    {
        std::size_t total = 0;
        bool done = false;

        // Optional: pre-reserve up to max_len to reduce reallocations
        if constexpr(!std::is_same_v<Cont, void> && !std::is_same_v<Cont, char>) {
            if (max_len == std::numeric_limits<std::size_t>::max()) {
                out->reserve(STRING_CHUNK_SIZE);

            }
        }

        while (!done) {
            std::size_t remaining = max_len - total;
            if (remaining == 0) {
                // Overflow: we hit the max_len but reader still has string data
                setError(ParseError::JSON_SINK_OVERFLOW, current_);
                return false;
            }

            constexpr std::size_t CHUNK = STRING_CHUNK_SIZE;
            std::size_t ask = remaining < CHUNK ? remaining : CHUNK;

            // Make sure string has space [total, total+ask) to write into.
            // This also keeps size() in a valid state.
            StringChunkResult res;
            if constexpr(!std::is_same_v<Cont, void> && !std::is_same_v<Cont, char>) {
                out->resize(initS + total + ask);
                res = read_string_chunk(out->data() + initS + total, ask);
            } else if constexpr (std::is_same_v<Cont, char>) {
                res = read_string_chunk(out + initS + total,
                                        max_len - total);
            } else {
                char b[CHUNK];
                res = read_string_chunk(b, ask);
            }

            if(res.status != tokenizer::StringChunkStatus::ok) {
                return false;
            }

            total += res.bytes_written;

            if (res.done) {
                // Shrink to actual number of bytes written
                outSize = total;
                if constexpr(!std::is_same_v<Cont, void> && !std::is_same_v<Cont, char>) {
                    out->resize(initS + total);
                }
                return true;
            }

            // Not done yet; if reader filled less than asked but not done,
            // you might optionally treat that as suspicious, but generally fine.
            // Loop continues.
        }

        // Shouldn’t be reachable
        setError(ParseError::ILLFORMED_STRING, current_);
        return false;
    }


};


} // namespace tokenizer

} // namespace JsonFusion
