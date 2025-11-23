#pragma once

#ifndef JSONFUSION_USE_FAST_FLOAT
#define JSONFUSION_USE_FAST_FLOAT 1  // desktop default
#endif

#ifdef JSONFUSION_USE_FAST_FLOAT
#include "3party/fast_double_parser.h"
#include "3party/simdjson/to_chars.hpp"
#else
#include <cstdlib>
#include <charconv>
#endif

namespace JsonFusion::fp_to_str_detail {
constexpr std::size_t NumberBufSize = 64;

bool parse_number_to_double(const char * buf, double& out) {
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
    // portable fallback: std::to_chars if available
    auto result = std::to_chars(first, last, value, std::chars_format::general, decimals);
    if (result.ec != std::errc{}) {
        // worst-case fallback: clamp or write "0"
        if (first != last) *first++ = '0';
        return first;
    }
    return result.ptr;
#endif
}

} // namespace JsonFusion::fp_to_str_detail
