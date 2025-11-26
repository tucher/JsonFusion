#pragma once

#include <array>
#include <cstdint>
#include <iterator>
#include <optional>
#include <string_view>
#include <utility>  // std::declval
#include <ranges>
#include <type_traits>
#include <algorithm>
#include <limits>
#include <bitset>

#include "static_schema.hpp"
#include "options.hpp"
#include "validators.hpp"

#include "fp_to_str.hpp"
#include "struct_introspection.hpp"

namespace JsonFusion {

enum class ParseError {
    NO_ERROR,
    ILLFORMED_NUMBER,
    ILLFORMED_NULL,
    ILLFORMED_STRING,
    ILLFORMED_ARRAY,
    ILLFORMED_OBJECT,

    UNEXPECTED_END_OF_DATA,
    UNEXPECTED_SYMBOL,
    FIXED_SIZE_CONTAINER_OVERFLOW,
    NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE,
    FLOAT_VALUE_IN_INTEGER_STORAGE,
    ILLFORMED_BOOL,
    EXCESS_FIELD,
    NULL_IN_NON_OPTIONAL,

    EXCESS_DATA,
    SKIPPING_STACK_OVERFLOW,
    SCHEMA_VALIDATION_ERROR,

    ARRAY_DESTRUCRING_SCHEMA_ERROR,
    DATA_CONSUMER_ERROR
};


// 1) Iterator you can:
//    - read as *it   (convertible to char)
//    - advance as it++ / ++it
template <class It>
concept CharInputIterator =
    std::input_iterator<It> &&
    std::convertible_to<std::iter_reference_t<It>, char>;

// 2) Matching "end" type you can:
//    - compare as it == end / it != end
template <class It, class Sent>
concept CharSentinelFor =
    CharInputIterator<It> &&
    std::sentinel_for<Sent, It>;


template <CharInputIterator InpIter>
class ParseResult {
    ParseError m_error = ParseError::NO_ERROR;
    InpIter m_pos;
    validators::ValidationResult validationResult;
public:
    constexpr ParseResult(ParseError err, validators::ValidationResult schemaErrors, InpIter pos):
        m_error(err), m_pos(pos), validationResult(schemaErrors)
    {}
    constexpr operator bool() const {
        return m_error == ParseError::NO_ERROR && validationResult;
    }
    constexpr InpIter pos() const {
        return m_pos;
    }
    constexpr ParseError error() const {
        if (m_error == ParseError::NO_ERROR) {
            return ParseError::NO_ERROR;
        } else {
            if(!validationResult) {
                return ParseError::SCHEMA_VALIDATION_ERROR;
            }
        }
        return m_error;
    }

};


namespace  parser_details {


template <CharInputIterator InpIter>
class DeserializationContext {

    ParseError error = ParseError::NO_ERROR;
    InpIter m_pos;
    validators::ValidationCtx _validationCtx;
public:
    constexpr DeserializationContext(InpIter b) {
        m_pos = b;
    }
    constexpr void setError(ParseError err, InpIter pos) {
        error = err;
        m_pos = pos;
    }

    constexpr ParseResult<InpIter> result() {
        return ParseResult<InpIter>(error, _validationCtx.result(), m_pos);
    }
    constexpr validators::ValidationCtx & validationCtx() {return _validationCtx;}
};

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

template <CharInputIterator It, CharSentinelFor<It> Sent>
constexpr inline bool skipWhiteSpace(It & currentPos, const Sent & end, DeserializationContext<It> & ctx) {
    while (currentPos != end && isSpace(*currentPos)) {
        ++currentPos;
    }
    if (currentPos == end) {
        ctx.setError(ParseError::UNEXPECTED_END_OF_DATA, currentPos);
        return false;
    }
    return true;
}

constexpr bool match_literal(auto& it, const auto& end, const std::string_view & lit) {
    for (char c : lit) {
        if (it == end || *it != c) {
            return false;
        }
        ++it;
    }
    return true;
}

template <class Opts, class ObjT, CharInputIterator It, CharSentinelFor<It> Sent>
    requires static_schema::JsonBool<ObjT>
constexpr bool ParseNonNullValue(ObjT & obj, It &currentPos, const Sent & end, DeserializationContext<It> &ctx) {
    if(currentPos == end) {
        ctx.setError(ParseError::UNEXPECTED_END_OF_DATA, currentPos);
        return false;
    }
    if (*currentPos == 't' && match_literal(++currentPos, end, "rue") && (isPlainEnd(*currentPos) || currentPos == end)) {
        obj = true;
    } else if (*currentPos == 'f' && match_literal(++currentPos, end, "alse") && (isPlainEnd(*currentPos)|| currentPos == end)) {
        obj = false;
    } else {
        ctx.setError(ParseError::ILLFORMED_BOOL, currentPos);
        return false;
    }
    return validators::validator<Opts>::template validate<validators::parsing_events_tags::bool_parsing_finished>(obj, ctx.validationCtx());
}


template <CharInputIterator It, CharSentinelFor<It> Sent>
constexpr bool read_number_token(It& currentPos,
                       const Sent& end,
                       DeserializationContext<It>& ctx,
                       char (&buf)[fp_to_str_detail::NumberBufSize],
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

    if (currentPos == end) {
        ctx.setError(ParseError::UNEXPECTED_END_OF_DATA, currentPos);
        return false;
    }

    // Optional leading '-'
    {
        char c = *currentPos;
        if (c == '-') {
            if (index >= fp_to_str_detail::NumberBufSize - 1) {
                ctx.setError(ParseError::ILLFORMED_NUMBER, currentPos);
                return false;
            }
            buf[index++] = c;
            ++currentPos;
        }
    }

    if (currentPos == end) {
        ctx.setError(ParseError::UNEXPECTED_END_OF_DATA, currentPos);
        return false;
    }

    auto push_char = [&](char c) -> bool {
        if (index >= fp_to_str_detail::NumberBufSize - 1) {
            ctx.setError(ParseError::ILLFORMED_NUMBER, currentPos);
            return false;
        }
        buf[index++] = c;
        return true;
    };

    while (currentPos != end && !isPlainEnd(*currentPos)) {
        char c = *currentPos;

        if (c >= '0' && c <= '9') {
            if (!inExp) {
                seenDigitBeforeExp = true;
            } else {
                seenDigitAfterExp = true;
            }
            if (!push_char(c)) return false;
            ++currentPos;
            continue;
        }

        if (c == '.' && !seenDot && !inExp) {
            seenDot = true;
            if (!push_char(c)) return false;
            ++currentPos;
            continue;
        }

        if ((c == 'e' || c == 'E') && !inExp) {
            inExp  = true;
            seenExp = true;
            if (!push_char(c)) return false;
            ++currentPos;

            // Optional sign immediately after exponent
            if (currentPos != end && (*currentPos == '+' || *currentPos == '-')) {
                if (!push_char(*currentPos)) return false;
                ++currentPos;
            }
            continue;
        }

        // '+' or '-' is only allowed immediately after 'e'/'E', which we handled above
        // Anything else is invalid inside a JSON number
        ctx.setError(ParseError::ILLFORMED_NUMBER, currentPos);
        return false;
    }

    buf[index] = '\0';

    // Basic sanity: must have at least one digit before exponent,
    // and if exponent present, at least one digit after it.
    if (!seenDigitBeforeExp || (seenExp && !seenDigitAfterExp && inExp)) {
        ctx.setError(ParseError::ILLFORMED_NUMBER, currentPos);
        return false;
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
        if (value > (limit - digit) / 10u) {
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

// Strategy: custom integer parsing (no deps), delegated float parsing (configurable)

template <class Opts, class ObjT, CharInputIterator It, CharSentinelFor<It> Sent>
    requires static_schema::JsonNumber<ObjT>
constexpr bool ParseNonNullValue(ObjT& obj, It &currentPos, const Sent & end, DeserializationContext<It> &ctx) {
    char buf[fp_to_str_detail::NumberBufSize];
    std::size_t index = 0;
    bool seenDot = false;
    bool seenExp = false;

    if (!read_number_token(currentPos, end, ctx, buf, index, seenDot, seenExp)) {
        return false;
    }
    if constexpr (std::is_integral_v<ObjT>) {
        // Reject decimals/exponents for integer fields
        if (seenDot || seenExp) {
            ctx.setError(ParseError::FLOAT_VALUE_IN_INTEGER_STORAGE, currentPos);
            return false;
        }

        ObjT value{};
        if(!parse_decimal_integer<ObjT>(buf, value)) {
            ctx.setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE, currentPos);
            return false;
        }

        obj = value;
    } else if constexpr (std::is_floating_point_v<ObjT>) {
        double x;
        if(fp_to_str_detail::parse_number_to_double(buf, x)) {
            if(static_cast<double>(std::numeric_limits<ObjT>::lowest()) > x
                || static_cast<double>(std::numeric_limits<ObjT>::max()) < x) {
                ctx.setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE, currentPos);
                return false;
            }
            obj = static_cast<ObjT>(x);
        } else {
            ctx.setError(ParseError::ILLFORMED_NUMBER, currentPos);
            return false;
        }
    } else {
        // Should never happen if JsonNumber is correct
        static_assert(std::is_integral_v<ObjT> || std::is_floating_point_v<ObjT>,
                      "[[[ JsonFusion ]]] JsonNumber underlying type must be integral or floating");
        ctx.setError(ParseError::ILLFORMED_NUMBER, currentPos);
        return false;
    }
    if(!validators::validator<Opts>::template validate<validators::parsing_events_tags::number_parsing_finished>(obj, ctx.validationCtx())) {
        ctx.setError(ParseError::SCHEMA_VALIDATION_ERROR, currentPos);
        return false;
    }
    return true;
}

template <typename T>
concept DynamicContainerTypeConcept = requires (T  v) {
    typename T::value_type;
    v.push_back(std::declval<typename T::value_type>());
    v.clear();
};

template <CharInputIterator It, CharSentinelFor<It> Sent>
constexpr bool readHex4(It &currentPos, const Sent &end, DeserializationContext<It> &ctx, std::uint16_t &out) {
    out = 0;
    for (int i = 0; i < 4; ++i) {
        if (currentPos == end) [[unlikely]] {
            ctx.setError(ParseError::UNEXPECTED_END_OF_DATA, currentPos);
            return false;
        }
        char currChar = *currentPos;
        std::uint8_t v;
        if (currChar >= '0' && currChar <= '9') {
            v = static_cast<std::uint8_t>(currChar - '0');
        } else if (currChar >= 'A' && currChar <= 'F') {
            v = static_cast<std::uint8_t>(currChar - 'A' + 10);
        } else if (currChar >= 'a' && currChar <= 'f') {
            v = static_cast<std::uint8_t>(currChar - 'a' + 10);
        } else {
            ctx.setError(ParseError::UNEXPECTED_SYMBOL, currentPos);
            return false;
        }
        out = static_cast<std::uint16_t>((out << 4) | v);
        ++currentPos;
    }
    return true;
}

template <class Visitor, CharInputIterator It, CharSentinelFor<It> Sent>
constexpr bool parseString(Visitor&& inserter, It &currentPos, const Sent & end, DeserializationContext<It> &ctx) {
    if(*currentPos != '"'){
        ctx.setError(ParseError::ILLFORMED_STRING, currentPos);
        return false;
    }
    currentPos++;

    while(true) {
        if(currentPos == end) {
            ctx.setError(ParseError::UNEXPECTED_END_OF_DATA, currentPos);
            return false;
        }
        auto c = *currentPos;
        switch(c) {
        case '"': {
            currentPos ++;

            return true;
        }
        case '\\': {
            currentPos++;
            if(currentPos == end) [[unlikely]] {
                ctx.setError(ParseError::UNEXPECTED_END_OF_DATA, currentPos);
                return false;
            }

            char out{};
            switch (*currentPos) {
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
                ++currentPos; // move past 'u'

                std::uint16_t u1 = 0;
                if (!readHex4(currentPos, end, ctx, u1)) {
                    return false; // ctx already set
                }

                std::uint32_t codepoint = 0;

                // Handle surrogate pairs per JSON/UTF-16 rules
                if (u1 >= 0xD800u && u1 <= 0xDBFFu) {
                    // High surrogate -> must be followed by \uDC00..DFFF
                    if (currentPos == end) [[unlikely]] {
                        ctx.setError(ParseError::UNEXPECTED_END_OF_DATA, currentPos);
                        return false;
                    }
                    if (*currentPos != '\\') {
                        ctx.setError(ParseError::ILLFORMED_STRING, currentPos);
                        return false;
                    }
                    ++currentPos;
                    if (currentPos == end) [[unlikely]] {
                        ctx.setError(ParseError::UNEXPECTED_END_OF_DATA, currentPos);
                        return false;
                    }
                    if (*currentPos != 'u') {
                        ctx.setError(ParseError::ILLFORMED_STRING, currentPos);
                        return false;
                    }
                    ++currentPos;

                    std::uint16_t u2 = 0;
                    if (!readHex4(currentPos, end, ctx, u2)) {
                        return false;
                    }
                    if (u2 < 0xDC00u || u2 > 0xDFFFu) {
                        // Low surrogate not in valid range
                        ctx.setError(ParseError::ILLFORMED_STRING, currentPos);
                        return false;
                    }

                    codepoint = 0x10000u
                                + ((static_cast<std::uint32_t>(u1) - 0xD800u) << 10)
                                + (static_cast<std::uint32_t>(u2) - 0xDC00u);
                } else if (u1 >= 0xDC00u && u1 <= 0xDFFFu) {
                    // Lone low surrogate → invalid
                    ctx.setError(ParseError::ILLFORMED_STRING, currentPos);
                    return false;
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
                    if (!inserter(utf8[i])) {
                        ctx.setError(ParseError::FIXED_SIZE_CONTAINER_OVERFLOW, currentPos);
                        return false;
                    }
                }
            }
            break;

            default:
                /* Unexpected symbol */
                ctx.setError(ParseError::UNEXPECTED_SYMBOL, currentPos);
                return false;
            }
            if (out) { // only emit if we actually set a simple escape
                if (!inserter(out)) {
                    ctx.setError(ParseError::FIXED_SIZE_CONTAINER_OVERFLOW, currentPos);
                    return false;
                }
                currentPos++;
            }

        }
        break;

        default:
            // RFC 8259 §7: Control characters (U+0000-U+001F) MUST be escaped
            if (static_cast<unsigned char>(c) <= 0x1F) {
                ctx.setError(ParseError::ILLFORMED_STRING, currentPos);
                return false;
            }
            if(!inserter(*currentPos)) {
                return false;
            }
            currentPos++;
        }
    }
}


template <class Opts, class ObjT, CharInputIterator It, CharSentinelFor<It> Sent>
    requires static_schema::JsonString<ObjT>
constexpr bool ParseNonNullValue(ObjT& obj, It &currentPos, const Sent & end, DeserializationContext<It> &ctx) {
    std::size_t parsedSize = 0;
    if constexpr (DynamicContainerTypeConcept<ObjT>) {
        obj.clear();
    }
    auto inserter = [&obj, &parsedSize, &ctx, &currentPos] (char c) -> bool {
        if constexpr (!DynamicContainerTypeConcept<ObjT>) {
            if (parsedSize < obj.size()-1) {
                obj[parsedSize] = c;
            } else {
               ctx.setError(ParseError::FIXED_SIZE_CONTAINER_OVERFLOW, currentPos);
               return false;
            }
        }  else {
            obj.push_back(c);
        }
        parsedSize++;
        if(!validators::validator<Opts>::template validate<validators::parsing_events_tags::string_parsed_some_chars>(obj, ctx.validationCtx(), parsedSize)) {
            ctx.setError(ParseError::SCHEMA_VALIDATION_ERROR, currentPos);
            return false;
        }

        return true;
    };

    if(parseString(inserter, currentPos, end, ctx)) {
        if constexpr (!DynamicContainerTypeConcept<ObjT>) {
            if(parsedSize < obj.size())
                obj[parsedSize] = 0;
        }
        if(!validators::validator<Opts>::template validate<validators::parsing_events_tags::string_parsing_finished>(obj, ctx.validationCtx(), parsedSize)) {
            ctx.setError(ParseError::SCHEMA_VALIDATION_ERROR, currentPos);
            return false;
        }

        return true;
    } else {
        return false;
    }
}

template <class Opts, class ObjT, CharInputIterator It, CharSentinelFor<It> Sent>
    requires static_schema::JsonParsableArray<ObjT>
constexpr bool ParseNonNullValue(ObjT& obj, It &currentPos, const Sent & end, DeserializationContext<It> &ctx) {
    if constexpr (DynamicContainerTypeConcept<ObjT>) {
        obj.clear();
    }
    if(*currentPos != '[') {
        ctx.setError(ParseError::ILLFORMED_ARRAY, currentPos);
        return false;
    }
    currentPos++;
    if(!skipWhiteSpace(currentPos, end, ctx)) [[unlikely]] {
        return false;
    }
    std::size_t parsed_items_count = 0;
    bool has_trailing_comma = false;

    using FH   = static_schema::array_write_cursor<ObjT>;
    FH cursor{ obj };
    cursor.reset();
    while(true) {
        if(currentPos == end) [[unlikely]] {
            ctx.setError(ParseError::UNEXPECTED_END_OF_DATA, currentPos);
            cursor.finalize(false);
            return false;
        }
        if(!skipWhiteSpace(currentPos, end, ctx)) [[unlikely]] {
            cursor.finalize(false);
            return false;
        }
        if(*currentPos == ']') {

            if(has_trailing_comma) {
                ctx.setError(ParseError::ILLFORMED_ARRAY, currentPos);
                cursor.finalize(false);
                return false;
            }
            if(!validators::validator<Opts>::template validate<validators::parsing_events_tags::array_parsing_finished>(obj, ctx.validationCtx(), parsed_items_count)) {
                ctx.setError(ParseError::SCHEMA_VALIDATION_ERROR, currentPos);
                cursor.finalize(false);
                return false;
            }

            cursor.finalize(true);
            currentPos ++;
            return true;
        }
        if(parsed_items_count > 0 && !has_trailing_comma) {
            ctx.setError(ParseError::ILLFORMED_ARRAY, currentPos);
            cursor.finalize(false);
            return false;
        }

        stream_write_result alloc_r = cursor.allocate_slot();
        if(alloc_r != stream_write_result::slot_allocated) {
            if(alloc_r == stream_write_result::overflow ) {
                ctx.setError(ParseError::FIXED_SIZE_CONTAINER_OVERFLOW, currentPos);
                return false;
            } else {
                ctx.setError(ParseError::DATA_CONSUMER_ERROR, currentPos);
                return false;
            }

        }
        auto & newItem = cursor.get_slot();
        if(!ParseValue(newItem, currentPos, end, ctx)) {
            cursor.finalize(false);
            return false;
        }

        parsed_items_count ++;
        if(!validators::validator<Opts>::template validate<validators::parsing_events_tags::array_item_parsed>(obj, ctx.validationCtx(), parsed_items_count)) {
            ctx.setError(ParseError::SCHEMA_VALIDATION_ERROR, currentPos);
            cursor.finalize(false);
            return false;
        }


        if(!skipWhiteSpace(currentPos, end, ctx)) [[unlikely]] {
            cursor.finalize(false);
            return false;
        }
        has_trailing_comma = false;
        if(*currentPos == ',') {
            has_trailing_comma = true;
            currentPos ++;
        }

    }
    cursor.finalize(false);
    return false;
}

#ifndef JSONFUSION_MAX_SKIP_NESTING
constexpr int MAX_SKIP_NESTING = 64;
#else
constexpr int MAX_SKIP_NESTING = JSONFUSION_MAX_SKIP_NESTING;
#endif


template <CharInputIterator It, CharSentinelFor<It> Sent>
bool matchLiteralTail(It& currentPos, const Sent& end,
                      const char* tail, std::size_t len,
                      ParseError err, DeserializationContext<It>& ctx)
{
    for (std::size_t i = 0; i < len; ++i) {
        if (currentPos == end || *currentPos != tail[i]) {
            ctx.setError(err, currentPos);
            return false;
        }
        ++currentPos;
    }
    return true;
}


template <CharInputIterator It, CharSentinelFor<It> Sent>
bool SkipValue(It &currentPos, const Sent & end, DeserializationContext<It> &ctx) {
    if (!skipWhiteSpace(currentPos, end, ctx)) {
        return false;
    }
    if (currentPos == end) {
        ctx.setError(ParseError::UNEXPECTED_END_OF_DATA, currentPos);
        return false;
    }

    auto noopInserter = [](char) -> bool { return true; };

    char c = *currentPos;

    // 1) Simple values we can skip without nesting
    if (c == '"') {
        // string
        return parseString(noopInserter, currentPos, end, ctx);
    }

    if (c == 't') {
        // 't' seen; match "rue"
        ++currentPos;
        return matchLiteralTail(currentPos, end, "rue", 3,
                                ParseError::ILLFORMED_BOOL, ctx);
    }
    if (c == 'f') {
        // 'f' seen; match "alse"
        ++currentPos;
        return matchLiteralTail(currentPos, end, "alse", 4,
                                ParseError::ILLFORMED_BOOL, ctx);
    }
    if (c == 'n') {
        // 'n' seen; match "ull"
        ++currentPos;
        return matchLiteralTail(currentPos, end, "ull", 3,
                                ParseError::ILLFORMED_NULL, ctx);
    }

    // number-like: skip until a plain end (whitespace, ',', ']', '}', etc.)
    auto skipNumberLike = [&]() {
        while (currentPos != end && !isPlainEnd(*currentPos)) {
            ++currentPos;
        }
        return true;
    };

    if (c != '{' && c != '[') {
        // neither object nor array → treat as number-like token
        return skipNumberLike();
    }

    // 2) Compound value: object or array with possible nesting.
    // We use an explicit stack of expected closing delimiters to avoid recursion.

    char stack[MAX_SKIP_NESTING];
    int  depth = 0;

    auto push_close = [&](char open) -> bool {
        if (depth >= MAX_SKIP_NESTING) {
            ctx.setError(ParseError::SKIPPING_STACK_OVERFLOW, currentPos);
            return false;
        }
        stack[depth++] = (open == '{') ? '}' : ']';
        return true;
    };

    auto pop_close = [&](char close) -> bool {
        if (depth == 0) {
            ctx.setError(ParseError::ILLFORMED_OBJECT, currentPos);
            return false;
        }
        if (stack[depth - 1] != close) {
            ctx.setError(ParseError::ILLFORMED_OBJECT, currentPos);
            return false;
        }
        --depth;
        return true;
    };

    // Initialize with the first '{' or '['
    if (!push_close(c)) {
        return false;
    }
    ++currentPos; // skip first '{' or '['

    while (currentPos != end && depth > 0) {
        char ch = *currentPos;

        // Skip whitespace cheaply
        if (isSpace(ch)) {
            ++currentPos;
            continue;
        }

        switch (ch) {
        case '"': {
            if (!parseString(noopInserter, currentPos, end, ctx)) {
                return false;
            }
            break;
        }
        case '{':
        case '[': {
            if (!push_close(ch)) {
                return false;
            }
            ++currentPos;
            break;
        }
        case '}':
        case ']': {
            if (!pop_close(ch)) {
                return false;
            }
            ++currentPos;
            break;
        }
        case 't': {
            ++currentPos;
            if (!matchLiteralTail(currentPos, end, "rue", 3,
                                  ParseError::ILLFORMED_BOOL, ctx)) {
                return false;
            }
            break;
        }
        case 'f': {
            ++currentPos;
            if (!matchLiteralTail(currentPos, end, "alse", 4,
                                  ParseError::ILLFORMED_BOOL, ctx)) {
                return false;
            }
            break;
        }
        case 'n': {
            ++currentPos;
            if (!matchLiteralTail(currentPos, end, "ull", 3,
                                  ParseError::ILLFORMED_NULL, ctx)) {
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
                ++currentPos; // colon, comma, etc. – don't affect nesting
            }
            break;
        }
        }
    }

    if (depth != 0) {
        ctx.setError(ParseError::UNEXPECTED_END_OF_DATA, currentPos);
        return false;
    }

    return true;
}




struct FieldDescr {
    std::string_view name;
    std::size_t originalIndex;

};

template<class T>
struct FieldsHelper {
    template<std::size_t I>
    static consteval bool fieldIsNotJSON() {
        using Field   = introspection::structureElementTypeByIndex<I, T>;
        using Opts    = options::detail::annotation_meta_getter<Field>::options;
        if constexpr (Opts::template has_option<options::detail::not_json_tag>) {
            return true;
        } else {
            return false;
        }
    }

    static constexpr std::size_t rawFieldsCount = introspection::structureElementsCount<T>;

    static constexpr std::size_t fieldsCount = []<std::size_t... I>(std::index_sequence<I...>) consteval{
        //TODO get name from options, if presented
        return (std::size_t{0} + ... + (!fieldIsNotJSON<I>() ? 1: 0));
    }(std::make_index_sequence<rawFieldsCount>{});



    template<std::size_t I>
    static consteval std::string_view fieldName() {
        using Field   = introspection::structureElementTypeByIndex<I, T>;
        using Opts    = options::detail::annotation_meta_getter<Field>::options;

        if constexpr (Opts::template has_option<options::detail::key_tag>) {
            using KeyOpt = typename Opts::template get_option<options::detail::key_tag>;
            return KeyOpt::desc.toStringView();
        } else {
            return introspection::structureElementNameByIndex<I, T>;
        }
    }

    static constexpr std::array<FieldDescr, fieldsCount> fieldIndexesSortedByFieldName =
        []<std::size_t... I>(std::index_sequence<I...>) consteval {
            std::array<FieldDescr, fieldsCount> arr;
            std::size_t index = 0;
            auto add_one = [&](auto ic) consteval {
                constexpr std::size_t J = decltype(ic)::value;
                if constexpr (!fieldIsNotJSON<J>()) {
                    arr[index++] = FieldDescr{ fieldName<J>(), J };
                }
            };
            (add_one(std::integral_constant<std::size_t, I>{}), ...);
            std::ranges::sort(arr, {}, &FieldDescr::name);

            return arr;
        }(std::make_index_sequence<rawFieldsCount>{});

    static constexpr bool fieldsAreUnique = [](std::array<FieldDescr, fieldsCount> sortedArr) consteval{
        return std::ranges::adjacent_find(sortedArr, {}, &FieldDescr::name) == sortedArr.end();
    }(fieldIndexesSortedByFieldName);


    static consteval std::size_t indexInSortedByName(std::string_view name) {
        for(int i = 0; i < fieldsCount; i++) {
            if(fieldIndexesSortedByFieldName[i].name == name) {
                return i;
            }
        }
        return -1;
    }
};

struct IncrementalFieldSearch {
    using It = const FieldDescr*;

    It first;         // current candidate range [first, last)
    It last;
    It original_end;  // original end, used as "empty result"
    std::size_t depth = 0; // how many characters have been fed

    constexpr IncrementalFieldSearch(It begin, It end)
        : first(begin), last(end), original_end(end), depth(0) {}

    // Feed next character; narrows [first, last) by character at position `depth`.
    // Returns true if any candidates remain after this step.
    constexpr bool step(char ch) {
        if (first == last)
            return false;

        // projection: FieldDescr -> char at current depth
        auto char_at_depth = [this](const FieldDescr& f) -> char {
            std::string_view s = f.name;
            // sentinel: '\0' means "past the end"
            return depth < s.size() ? s[depth] : '\0';
        };

        // lower_bound: first element with char_at_depth(elem) >= ch
        auto lower = std::ranges::lower_bound(
            first, last, ch, {}, char_at_depth
            );

        // upper_bound: first element with char_at_depth(elem) > ch
        auto upper = std::ranges::upper_bound(
            lower, last, ch, {}, char_at_depth
            );

        first = lower;
        last  = upper;
        ++depth;
        return first != last;
    }

    // Return:
    //   - pointer to unique FieldDescr if exactly one matches AND
    //     fully typed (depth >= name.size())
    //   - original_end if 0 matches OR >1 matches OR undertyped.
    constexpr It result() const {
        if (first == last)
            return original_end; // 0 matches


        // exactly one candidate
        const FieldDescr& candidate = *first;

        // undertyping: not all characters of the name have been given
        if (depth != candidate.name.size())
            return original_end;

        return first;
    }
};




template <class Opts, class ObjT, CharInputIterator It, CharSentinelFor<It> Sent>
    requires static_schema::JsonObject<ObjT>
constexpr bool ParseNonNullValue(ObjT& obj, It &currentPos, const Sent & end, DeserializationContext<It> &ctx) {
    using FH = FieldsHelper<ObjT>;
    static_assert(FH::fieldsAreUnique, "[[[ JsonFusion ]]] Fields are not unique");
    if(*currentPos != '{') {
        ctx.setError(ParseError::ILLFORMED_OBJECT, currentPos);
        return false;
    }
    currentPos++;
    if(!skipWhiteSpace(currentPos, end, ctx)) [[unlikely]] {
        return false;
    }
    bool has_trailing_comma = false;
    bool isFirst = true;

    std::bitset<FH::fieldsCount> parsedFieldsByIndex{};
    while(true) {
        if(currentPos == end) [[unlikely]] {
            ctx.setError(ParseError::UNEXPECTED_END_OF_DATA, currentPos);
            return false;
        }
        if(!skipWhiteSpace(currentPos, end, ctx)) [[unlikely]] {
            return false;
        }

        if(*currentPos == '}') {
            if(has_trailing_comma) {
                ctx.setError(ParseError::ILLFORMED_OBJECT, currentPos);
                return false;
            }
            currentPos ++;

            if(!validators::validator<Opts>::template validate<validators::parsing_events_tags::object_parsing_finished>(obj, ctx.validationCtx(), parsedFieldsByIndex, FH{})) {
                ctx.setError(ParseError::SCHEMA_VALIDATION_ERROR, currentPos);
                return false;
            }

            return true;
        }

        if(!isFirst && !has_trailing_comma) {
            ctx.setError(ParseError::ILLFORMED_OBJECT, currentPos);
            return false;
        }

        IncrementalFieldSearch searcher{
            FH::fieldIndexesSortedByFieldName.data(), FH::fieldIndexesSortedByFieldName.data() + FH::fieldIndexesSortedByFieldName.size()
        };


        if(!parseString([&](char c){
                if constexpr(false) {//early skip. Cool, but actually really harmful: this will happen extremely rare
                    if(!searcher.step(c)) {

                        if constexpr (!Opts::template has_option<options::detail::allow_excess_fields_tag>) {
                            ctx.setError(ParseError::EXCESS_FIELD, currentPos);
                            return false;
                        }
                    }
                    return true;
                } else {
                    searcher.step(c);
                    return true;
                }
            }, currentPos, end, ctx)) {
            // field not found or bad string
            return false;

        }
        auto res = searcher.result();


        if(!skipWhiteSpace(currentPos, end, ctx)) [[unlikely]] {
            ctx.setError(ParseError::ILLFORMED_OBJECT, currentPos);
            return false;
        }
        if(*currentPos != ':') {
            ctx.setError(ParseError::ILLFORMED_OBJECT, currentPos);
            return false;
        }
        currentPos ++;
        if(currentPos == end) [[unlikely]] {
            ctx.setError(ParseError::UNEXPECTED_END_OF_DATA, currentPos);
            return false;
        }

        if(res == FH::fieldIndexesSortedByFieldName.end()) {
            if constexpr (Opts::template has_option<options::detail::allow_excess_fields_tag>) {
                if(!SkipValue(currentPos, end, ctx)) {
                    return false;
                }
            } else {
                ctx.setError(ParseError::EXCESS_FIELD, currentPos);
                return false;
            }
        } else {
            auto try_one = [&](auto ic) {
                constexpr std::size_t StructIndex = decltype(ic)::value;
                if constexpr(FH::template fieldIsNotJSON<StructIndex>()) {
                    return true;
                } else {
                    std::size_t arrIndex = res - FH::fieldIndexesSortedByFieldName.begin();
                    if(parsedFieldsByIndex[arrIndex] == true) {
                        ctx.setError(ParseError::ILLFORMED_OBJECT, currentPos);
                        return false;
                    } else if (!ParseValue(
                                   introspection::getStructElementByIndex<StructIndex>(obj),
                                   currentPos, end, ctx)) {
                        return false;
                    } else {
                        parsedFieldsByIndex[arrIndex] = true;
                        return true;
                    }
                }
            };

            std::size_t structIndex = res->originalIndex;
            bool field_parse_result = [&]<std::size_t... I>(std::index_sequence<I...>) {
                bool ok = false;
                (
                    (structIndex == I
                         ? (ok = try_one(std::integral_constant<std::size_t, I>{}), 0)
                         : 0),
                    ...
                    );
                return ok;
            } (std::make_index_sequence<FH::rawFieldsCount>{});
            if(!field_parse_result) {
                return false;
            }
        }

        if(!skipWhiteSpace(currentPos, end, ctx)) [[unlikely]] {
            return false;
        }
        isFirst = false;
        has_trailing_comma = false;
        if(*currentPos == ',') {
            has_trailing_comma = true;
            currentPos ++;
        }

    }
    return false;
}


/* #### SPECIAL CASE FOR ARRAYS DESRTUCTURING #### */ //TODO may be better to implement with additional helper, to precalculate holes somehow?
template <class Opts, class ObjT, CharInputIterator It, CharSentinelFor<It> Sent>
    requires static_schema::JsonObject<ObjT>
             &&
             Opts::template has_option<options::detail::as_array_tag>
constexpr bool ParseNonNullValue(ObjT& obj, It &currentPos, const Sent & end, DeserializationContext<It> &ctx) {
    if(*currentPos != '[') {
        ctx.setError(ParseError::ILLFORMED_ARRAY, currentPos);
        return false;
    }
    currentPos++;
    if(!skipWhiteSpace(currentPos, end, ctx)) [[unlikely]] {
        return false;
    }
    std::size_t parsed_items_count = 0;
    bool has_trailing_comma = false;
    std::size_t field_offset = 0;
    static constexpr std::size_t totalFieldsCount = introspection::structureElementsCount<ObjT>;
    while(true) {
        if(currentPos == end) [[unlikely]] {
            ctx.setError(ParseError::UNEXPECTED_END_OF_DATA, currentPos);
            return false;
        }
        if(!skipWhiteSpace(currentPos, end, ctx)) [[unlikely]] {
            return false;
        }
        if(*currentPos == ']') {
            if(has_trailing_comma) {
                ctx.setError(ParseError::ILLFORMED_ARRAY, currentPos);
                return false;
            }
            std::size_t final_fields_offest = field_offset;
            auto skip_checker = [&](auto ic) {
                constexpr std::size_t StructIndex = decltype(ic)::value;
                using Field   = introspection::structureElementTypeByIndex<StructIndex, ObjT>;
                using FieldOpts    = options::detail::annotation_meta_getter<Field>::options;
                if constexpr (FieldOpts::template has_option<options::detail::not_json_tag>) {
                    final_fields_offest ++;
                    return true;

                } else {
                    return false;
                }
            };

            bool skip_rest_result = true;
            if(parsed_items_count + field_offset != totalFieldsCount) {
                skip_rest_result = [&]<std::size_t... I>(std::index_sequence<I...>) {
                    bool ok = false;
                    (
                        ((parsed_items_count + field_offset) <= I
                             ? (ok = skip_checker(std::integral_constant<std::size_t, I>{}), 0)
                             : 0),
                        ...
                        );

                    return ok;
                } (std::make_index_sequence<totalFieldsCount>{});
            }

            if(!skip_rest_result || parsed_items_count + final_fields_offest != totalFieldsCount) {
                ctx.setError(ParseError::ARRAY_DESTRUCRING_SCHEMA_ERROR, currentPos);
                return false;
            }

            currentPos ++;
            return true;
        }

        if(parsed_items_count > 0 && !has_trailing_comma) {
            ctx.setError(ParseError::ILLFORMED_ARRAY, currentPos);
            return false;
        }
        bool skipped = false;
        auto try_one = [&](auto ic) {
            constexpr std::size_t StructIndex = decltype(ic)::value;
            using Field   = introspection::structureElementTypeByIndex<StructIndex, ObjT>;
            using FieldOpts    = options::detail::annotation_meta_getter<Field>::options;
            if constexpr (FieldOpts::template has_option<options::detail::not_json_tag>) {
                skipped = true;
                return false;
            } else {
                if(ParseValue(introspection::getStructElementByIndex<StructIndex>(obj), currentPos, end, ctx)) {

                    return true;
                } else {
                    return false;
                }
            }
        };
        bool field_parse_result = false;
        do {
            skipped = false;
            field_parse_result = [&]<std::size_t... I>(std::index_sequence<I...>) {
                bool ok = false;
                (
                    ((parsed_items_count + field_offset) == I
                         ? (ok = try_one(std::integral_constant<std::size_t, I>{}), 0)
                         : 0),
                    ...
                    );

                return ok;
            } (std::make_index_sequence<totalFieldsCount>{});
            if(skipped) {
                field_offset ++;
            }
        } while(skipped);
        if(!field_parse_result) {
            return false;
        }
        parsed_items_count ++;


        if(!skipWhiteSpace(currentPos, end, ctx)) [[unlikely]] {
            return false;
        }
        has_trailing_comma = false;
        if(*currentPos == ',') {
            has_trailing_comma = true;
            currentPos ++;
        }

    }
    return false;
}

template <static_schema::JsonParsableValue Field, CharInputIterator It, CharSentinelFor<It> Sent>
constexpr bool ParseValue(Field & field, It &currentPos, const Sent & end, DeserializationContext<It> &ctx) {
    using FieldMeta    = options::detail::annotation_meta_getter<Field>;

    if(!skipWhiteSpace(currentPos, end, ctx)) [[unlikely]] {
        return false;
    }
    if constexpr(static_schema::JsonNullableParsableValue<Field>) {

        if (currentPos == end) {
            ctx.setError(ParseError::UNEXPECTED_END_OF_DATA, currentPos);
            return false;
        }
        if(*currentPos == 'n') {
            currentPos++;
            if(!match_literal(currentPos, end, "ull")) {
                ctx.setError(ParseError::ILLFORMED_NULL, currentPos);
                return false;
            }
            static_schema::setNull(field);
            return true;
        }
    } else {
        if (currentPos != end && *currentPos == 'n') {
            ctx.setError(ParseError::NULL_IN_NON_OPTIONAL, currentPos);
            return false;
        }
    }
    return ParseNonNullValue<typename FieldMeta::options>(static_schema::getRef(field), currentPos, end, ctx);
}

} // namespace parser_details

template <static_schema::JsonParsableValue InputObjectT, CharInputIterator It, CharSentinelFor<It> Sent>
constexpr ParseResult<It> Parse(InputObjectT & obj, It begin, const Sent & end) {
    parser_details::DeserializationContext<decltype(begin)> ctx(begin);


    parser_details::ParseValue(obj, begin, end, ctx);

    auto res = ctx.result();
    if(!res) {
    } else {
        if(parser_details::skipWhiteSpace(begin, end, ctx)) {
            ctx.setError(ParseError::EXCESS_DATA, begin);
        } else {
            ctx.setError(ParseError::NO_ERROR, begin);
        }
    }
    return res;
}

template<class InputObjectT, class ContainterT>
constexpr auto Parse(InputObjectT & obj, const ContainterT & c) {
    return Parse(obj, c.begin(), c.end());
}


// Pointer + length front-end
template<static_schema::JsonParsableValue InputObjectT>
constexpr ParseResult<const char*> Parse(InputObjectT& obj, const char* data, std::size_t size) {
    const char* begin = data;
    const char* end   = data + size;

    parser_details::DeserializationContext<const char*> ctx(begin);

    const char* cur = begin;
    parser_details::ParseValue(obj, cur, end, ctx);

    auto res = ctx.result();
    if(!res) {
    } else {
        if(parser_details::skipWhiteSpace(cur, end, ctx)) {
            ctx.setError(ParseError::EXCESS_DATA, cur);
        } else {
            ctx.setError(ParseError::NO_ERROR, cur);
        }
    }
    return res;
}

// string_view front-end
template<static_schema::JsonParsableValue InputObjectT>
constexpr ParseResult<const char*> Parse(InputObjectT& obj, std::string_view sv) {
    return Parse(obj, sv.data(), sv.size());
}

template <class T>
    requires (!static_schema::JsonParsableValue<T>)
constexpr auto  Parse(T obj, auto, auto) {
    static_assert(static_schema::detail::always_false<T>::value,
                  "[[[ JsonFusion ]]] T is not a supported JsonFusion parsable value model type.\n"
                  "see JsonParsableValue concept for full rules");
}

template <class T>
    requires (!static_schema::JsonParsableValue<T>)
constexpr auto Parse(T obj, auto) {
    static_assert(static_schema::detail::always_false<T>::value,
                  "[[[ JsonFusion ]]] T is not a supported JsonFusion parsable value model type.\n"
                  "see JsonParsableValue concept for full rules");
}


} // namespace JsonFusion
