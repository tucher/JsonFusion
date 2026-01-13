#pragma once

#ifndef JSONFUSION_FP_BACKEND
#define JSONFUSION_FP_BACKEND 0  // in-house default
#endif

#if JSONFUSION_FP_BACKEND == 0
#include <cstdint>
#include <limits>
#endif

#if JSONFUSION_FP_BACKEND == 1
#include "3party/fast_double_parser.h"
#include "3party/simdjson/to_chars.hpp"
#endif

namespace JsonFusion::fp_to_str_detail {

#ifndef JSONFUSION_NUMBER_BUF_SIZE
constexpr std::size_t NumberBufSize = 64;
#else
constexpr std::size_t NumberBufSize = JSONFUSION_NUMBER_BUF_SIZE;
#endif

#if JSONFUSION_FP_BACKEND == 0

namespace detail {

struct DecimalNumber {
    bool     negative;
    uint64_t mantissa;
    int32_t  exp10;
};

constexpr int32_t kMaxExp10 = 400;
constexpr int32_t kMinExp10 = -400;

constexpr inline bool parse_decimal_number(const char* buf, DecimalNumber& out) noexcept {
    const char* p = buf;

    bool negative = false;
    if (*p == '+' || *p == '-') {
        negative = (*p == '-');
        ++p;
    }

    constexpr int max_sig_digits = 17;
    uint64_t mantissa   = 0;
    int32_t  exp10      = 0;
    int      sig_digits = 0;
    bool     any_digit  = false;

    bool in_fraction = false;

    while (true) {
        char c = *p;
        if (c >= '0' && c <= '9') {
            any_digit = true;
            unsigned digit = static_cast<unsigned>(c - '0');

            // Skip the integer '0' in "0.xxx" so that fractional digits
            // become the first significant digits.
            if (!in_fraction && sig_digits == 0 && digit == 0) {
                char next = *(p + 1);
                if (next == '.') {
                    ++p;
                    continue; // don't touch mantissa/exp10
                }
            }

            if (sig_digits < max_sig_digits) {
                mantissa = mantissa * 10u + digit;
                ++sig_digits;

                if (in_fraction) {
                    if (exp10 > kMinExp10) {
                        --exp10; // each kept fractional digit → 10^-1
                    }
                }
            } else {
                // mantissa is full: we discard extra digits.
                // For *integer* digits, decimal point still moves:
                if (!in_fraction) {
                    if (exp10 < kMaxExp10) {
                        ++exp10; // extra integer digits shift scale up
                    }
                }
                // For extra fractional digits: do *nothing*.
                // They are beyond precision and should not affect scale.
            }

            ++p;
        } else if (c == '.' && !in_fraction) {
            in_fraction = true;
            ++p;
        } else {
            break;
        }
    }

    if (!any_digit) {
        return false;
    }


    // exponent-part parsing unchanged...
    if (*p == 'e' || *p == 'E') {
        ++p;
        bool exp_negative = false;
        if (*p == '+' || *p == '-') {
            exp_negative = (*p == '-');
            ++p;
        }

        if (*p < '0' || *p > '9') {
            return false;
        }

        int32_t exp_part = 0;
        while (*p >= '0' && *p <= '9') {
            unsigned digit = static_cast<unsigned>(*p - '0');
            if (exp_part < kMaxExp10) {
                exp_part = exp_part * 10 + static_cast<int32_t>(digit);
                if (exp_part > kMaxExp10) {
                    exp_part = kMaxExp10;
                }
            }
            ++p;
        }

        if (exp_negative) {
            exp10 -= exp_part;
            if (exp10 < kMinExp10) exp10 = kMinExp10;
        } else {
            exp10 += exp_part;
            if (exp10 > kMaxExp10) exp10 = kMaxExp10;
        }
    }

    if (*p != '\0') {
        return false;
    }

    out.negative = negative;
    out.mantissa = mantissa;
    out.exp10    = exp10;
    return true;
}


constexpr double scale_by_power_of_10(double value, int32_t exp10) noexcept {
    if (exp10 == 0 || value == 0.0L) {
        return value;
    }

    static constexpr double kPow10Pos[] = {
        1e1L,    // 10^1
        1e2L,    // 10^2
        1e4L,    // 10^4
        1e8L,    // 10^8
        1e16L,   // 10^16
        1e32L,   // 10^32
        1e64L,   // 10^64
        1e128L,  // 10^128
        1e256L   // 10^256
    };

    static constexpr double kPow10Neg[] = {
        1e-1L,    // 10^-1
        1e-2L,    // 10^-2
        1e-4L,    // 10^-4
        1e-8L,    // 10^-8
        1e-16L,   // 10^-16
        1e-32L,   // 10^-32
        1e-64L,   // 10^-64
        1e-128L,  // 10^-128
        1e-256L   // 10^-256
    };

    bool negative_exp = exp10 < 0;
    uint32_t e = static_cast<uint32_t>(negative_exp ? -exp10 : exp10);

    // parse_decimal_number already clamps to ±400, so e <= 400 here.
    double result = value;
    unsigned idx = 0;

    while (e != 0 && idx < sizeof(kPow10Pos) / sizeof(kPow10Pos[0])) {
        if (e & 1u) {
            result *= (negative_exp ? kPow10Neg[idx] : kPow10Pos[idx]);
        }
        e >>= 1;
        ++idx;
    }

    return result;
}

}

constexpr inline bool parse_number_to_double(const char * buf, double& out) {
    detail::DecimalNumber dec{};
    if (!detail::parse_decimal_number(buf, dec)) {
        return false; // ill-formed
    }

    if (dec.mantissa == 0) {
        // Preserve signed zero like strtod would.
        out = dec.negative ? -0.0 : 0.0;
        return true;
    }

    double v = static_cast<double>(dec.mantissa);
    v = detail::scale_by_power_of_10(v, dec.exp10);

    if (dec.negative) {
        v = -v;
    }

    out = static_cast<double>(v);
    return true;
}

// Format double into buffer, return pointer past last char
constexpr  inline char* format_double_to_chars(char* first, double value, std::size_t decimals) {
    // JSON forbids NaN/Inf; here we just serialize them as "0" to keep things defined.
    // You can instead assert or set an error if you prefer stricter behavior.
    if (!(value == value) || value == std::numeric_limits<double>::infinity()
        || value == -std::numeric_limits<double>::infinity()) {
        *first++ = '0';
        return first;
    }

    // Handle sign
    bool negative = (value < 0.0);
    if (negative) {
        value = -value;
    }

    // Handle zero early (treat -0.0 as "0")
    if (value == 0.0) {
        if (negative) {
            *first++ = '-';
        }
        *first++ = '0';
        return first;
    }

    // Clamp precision to something reasonable for double: 1..17 significant digits
    int prec = 1;
    if (decimals == 0) {
        prec = 1;
    } else if (decimals > 17) {
        prec = 17;
    } else {
        prec = static_cast<int>(decimals);
    }

    // Write sign now (so all we do below just writes magnitude)
    if (negative) {
        *first++ = '-';
    }

    double v = static_cast<double>(value);

    // Normalize v to [1, 10) and compute decimal exponent: value = v * 10^exp10
    int32_t exp10 = 0;
    
    // Optimized normalization: scale by large powers first (much faster than single-step loop)
    if (v >= 10.0L) {
        // Scale down by powers of 10^16, 10^8, 10^4, 10^2, 10^1
        constexpr double p16 = 1e16L;
        constexpr double p8  = 1e8L;
        constexpr double p4  = 1e4L;
        constexpr double p2  = 1e2L;
        
        while (v >= p16) { v /= p16; exp10 += 16; }
        while (v >= p8)  { v /= p8;  exp10 += 8;  }
        while (v >= p4)  { v /= p4;  exp10 += 4;  }
        while (v >= p2)  { v /= p2;  exp10 += 2;  }
        while (v >= 10.0L) { v /= 10.0L; ++exp10; }
    } else if (v < 1.0L) {
        // Scale up by powers of 10^16, 10^8, 10^4, 10^2, 10^1
        constexpr double p16 = 1e16L;
        constexpr double p8  = 1e8L;
        constexpr double p4  = 1e4L;
        constexpr double p2  = 1e2L;
        
        while (v > 0.0L && v < 1e-15L) { v *= p16; exp10 -= 16; }
        while (v > 0.0L && v < 1e-7L)  { v *= p8;  exp10 -= 8;  }
        while (v > 0.0L && v < 1e-3L)  { v *= p4;  exp10 -= 4;  }
        while (v > 0.0L && v < 1e-1L)  { v *= p2;  exp10 -= 2;  }
        while (v > 0.0L && v < 1.0L)   { v *= 10.0L; --exp10; }
    }

    // Generate digits: we produce prec+1 digits, the last is a guard for rounding
    int digits[18]; // up to 17 significant digits + 1 guard
    for (int i = 0; i < prec + 1; ++i) {
        int d = static_cast<int>(v);
        if (d > 9) d = 9;  // safety clamp in case of tiny FP error
        digits[i] = d;
        v = (v - static_cast<double>(d)) * 10.0L;
    }

    // Round using the guard digit (simple round-half-up)
    if (digits[prec] >= 5) {
        int i = prec - 1;
        for (;;) {
            if (digits[i] < 9) {
                ++digits[i];
                break;
            }
            digits[i] = 0;
            if (i == 0) {
                // 9.999... rounded -> 10.000... => adjust exponent
                digits[0] = 1;
                ++exp10;
                break;
            }
            --i;
        }
    }
    // Now digits[0..prec-1] are the final significant digits.

    // Decide between fixed vs exponential notation, like %g:
    // fixed if -4 <= exp10 < prec, otherwise exponential
    int k = exp10;
    bool use_exp = (k < -4 || k >= prec);

    char* p = first;

    if (!use_exp) {
        // ---------------- Fixed-point notation (like %g) ----------------

        if (k >= 0) {
            // k >= 0: digits before decimal: k + 1
            int int_digits = k + 1;

            // Write integer part
            int i = 0;
            for (; i < int_digits && i < prec; ++i) {
                *p++ = static_cast<char>('0' + digits[i]);
            }

            if (i >= prec) {
                // All significant digits went into the integer part, done
                return p;
            }

            // Fractional part exists
            *p++ = '.';
            char* frac_start_ptr = p;

            for (; i < prec; ++i) {
                *p++ = static_cast<char>('0' + digits[i]);
            }

            // Trim trailing zeros in fractional part
            char* end = p;
            while (end > frac_start_ptr && end[-1] == '0') {
                --end;
            }
            if (end == frac_start_ptr) {
                // No digits after '.', remove it
                return frac_start_ptr - 1;
            }
            return end;

        } else {
            // k < 0: 0.xxx form
            *p++ = '0';
            *p++ = '.';

            int zeros = -k - 1;  // number of leading zeros after the decimal
            for (int i = 0; i < zeros; ++i) {
                *p++ = '0';
            }

            char* frac_start_ptr = p;

            for (int i = 0; i < prec; ++i) {
                *p++ = static_cast<char>('0' + digits[i]);
            }

            // Trim trailing zeros
            char* end = p;
            while (end > frac_start_ptr && end[-1] == '0') {
                --end;
            }
            if (end == frac_start_ptr) {
                // We ended up with "0." -> just "0"
                return frac_start_ptr - 2; // points to the '0'
            }
            return end;
        }

    } else {
        // ---------------- Exponential notation: d.dddde±xx ----------------

        // First digit
        *p++ = static_cast<char>('0' + digits[0]);

        if (prec > 1) {
            *p++ = '.';
            char* frac_start_ptr = p;

            for (int i = 1; i < prec; ++i) {
                *p++ = static_cast<char>('0' + digits[i]);
            }

            // Trim trailing zeros
            char* end = p;
            while (end > frac_start_ptr && end[-1] == '0') {
                --end;
            }
            if (end == frac_start_ptr) {
                // Remove the '.'
                p = frac_start_ptr - 1;
            } else {
                p = end;
            }
        }

        // Append exponent
        *p++ = 'e';
        if (k >= 0) {
            *p++ = '+';
        } else {
            *p++ = '-';
            k = -k;
        }

        // Convert exponent to decimal (no leading-zero padding – simpler than %g)
        char exp_buf[8]; // enough for signed 3-digit exponents
        int idx = 0;
        do {
            int digit = k % 10;
            exp_buf[idx++] = static_cast<char>('0' + digit);
            k /= 10;
        } while (k > 0);

        // Reverse exponent digits
        while (idx-- > 0) {
            *p++ = exp_buf[idx];
        }

        return p;
    }
}
#endif


#if JSONFUSION_FP_BACKEND == 1

constexpr inline bool parse_number_to_double(const char * buf, double& out) {
    const char* endp = fast_double_parser::parse_number(buf, &out);
    if (!endp) return false;
    return true;
}

constexpr  inline char* format_double_to_chars(char* first, double value, std::size_t decimals) {
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
}

#endif

#if JSONFUSION_FP_BACKEND != 0 && JSONFUSION_FP_BACKEND != 1
//For custom backends

inline bool parse_number_to_double(const char * buf, double& out);
inline char* format_double_to_chars(char* first, double value, std::size_t decimals);

#endif


} // namespace JsonFusion::fp_to_str_detail
