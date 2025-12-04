#pragma once

#ifndef JSONFUSION_USE_FAST_FLOAT
#define JSONFUSION_USE_FAST_FLOAT 1  // desktop default
#endif

#if JSONFUSION_USE_FAST_FLOAT
#include "3party/fast_double_parser.h"
#include "3party/simdjson/to_chars.hpp"
#else
#include <cstdlib>
#endif

namespace JsonFusion::fp_to_str_detail {

#ifndef JSONFUSION_NUMBER_BUF_SIZE
constexpr std::size_t NumberBufSize = 64;
#else
constexpr std::size_t NumberBufSize = JSONFUSION_NUMBER_BUF_SIZE;
#endif


inline bool parse_number_to_double(const char * buf, double& out) {
#if JSONFUSION_USE_FAST_FLOAT
    const char* endp = fast_double_parser::parse_number(buf, &out);
    if (!endp) return false;
    return true;
#else
    char* endp = nullptr;
    errno = 0;
    double x = std::strtod(buf, &endp);
    if (endp == buf) {
        return false;
    }
    out = x;
    return true;
#endif
}

// Format double into buffer, return pointer past last char
inline char* format_double_to_chars(char* first, char* last, double value, std::size_t decimals) {
#if JSONFUSION_USE_FAST_FLOAT
    // simdjson dtoa backend
    bool negative = std::signbit(value);
    if (negative) {
        value = -value;
        *first++ = '-';
    }

    if (value == 0) {
        *first++ = '0';
        if(negative) {
            *first++ = '.';
            *first++ = '0';
        }
        return first;
    }

    int len = 0;
    int decimal_exponent = 0;
    simdjson::internal::dtoa_impl::grisu2(first, len, decimal_exponent, value);
    constexpr int kMinExp = -4;
    constexpr int kMaxExp = std::numeric_limits<double>::digits10;
    return simdjson::internal::dtoa_impl::format_buffer(first, len, decimal_exponent,
                                                        kMinExp, kMaxExp);
#else
    // Caller must ensure first <= last and buffer is at least 1 char
    if (first == last) {
        return first;
    }

    // Clamp precision to something reasonable for double
    // (std::to_chars for general usually uses "significant digits").
    int prec;
    if (decimals == 0) {
        prec = 1;
    } else if (decimals > 17) {
        prec = 17;  // more than 17 significant digits is pointless for double
    } else {
        prec = static_cast<int>(decimals);
    }

    std::size_t bufSize = static_cast<std::size_t>(last - first);
    if (bufSize == 0) {
        return first;
    }

    // Use %.*g to approximate std::chars_format::general with `prec` significant digits.
    // snprintf writes a null terminator; we just ignore it and return a pointer
    // to the last non-null character.
    int n = std::snprintf(first,
                          bufSize,
                          "%.*g",
                          prec,
                          value);

    if (n <= 0) {
        // Fallback: write "0"
        *first++ = '0';
        return first;
    }

    // If the result was truncated, snprintf returns the number of chars
    // that WOULD have been written (excluding '\0').
    std::size_t written = static_cast<std::size_t>(n);
    if (written >= bufSize) {
        // We only have bufSize-1 chars + NUL, so clamp to bufSize-1.
        written = bufSize - 1;
    }

    return first + written;
#endif
}

} // namespace JsonFusion::fp_to_str_detail
