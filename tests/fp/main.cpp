#include <cstdint>
#include <cstring>
#include <limits>
#include <cmath>
#include <random>
#include <iostream>
#include <iomanip>
// #define JSONFUSION_FP_BACKEND 1  // on-house is the default, no need to switch anything

#include <JsonFusion/fp_to_str.hpp>

char* format_double_to_chars(char* first, double value, std::size_t decimals) {
    return JsonFusion::fp_to_str_detail::format_double_to_chars(first, value, decimals);
}
bool parse_number_to_double(const char* buf, double& out) {
    return JsonFusion::fp_to_str_detail::parse_number_to_double(buf, out);
}



double allowed_for_prec(int prec) {
    // Max relative diff between adjacent prec-digit decimals is 10^(1-prec)
    // Add a small safety factor, e.g. 2x or 5x.
    return 5.0 * std::pow(10.0, 1 - prec);
}

void fuzz_roundtrip_self(std::size_t iterations, double max_rel_error) {
    std::mt19937_64 rng(123456);
    std::uniform_int_distribution<uint64_t> dist;

    for (std::size_t i = 0; i < iterations; ++i) {
        uint64_t bits = dist(rng);
        double d;
        std::memcpy(&d, &bits, sizeof(d));

        // Skip NaN/Inf because JSON doesn’t support them anyway.
        if (std::isnan(d) || std::isinf(d)) {
            continue;
        }

        // Try several precisions
        for (int prec : {1, 2, 6, 10, 14, 15, 16, 17}) {
            char buf[JsonFusion::fp_to_str_detail::NumberBufSize];
            char* end = format_double_to_chars(buf, d, prec);
            *end = '\0';

            double parsed{};
            if (!parse_number_to_double(buf, parsed)) {
                std::cerr << "Parse failed on self-format: " << buf << "\n";
                std::abort();
            }
            if (prec >= 15) {
                double rel = std::abs(parsed - d) / std::max(std::abs(parsed), std::abs(d));
                if (rel > max_rel_error) { // or 1e-12 if you want to be very safe
                    std::cerr << "Self roundtrip mismatch rel error:\n"
                              << "  original: " << std::setprecision(17) << d << "\n"
                              << "  text    : " << buf << "\n"
                              << "  parsed  : " << parsed << "\n"
                              << "  rel  : " << rel << "\n"
                              << "  prec  : " << prec << "\n";
                    std::abort();
                }
            }
        }
    }
}

void fuzz_format_vs_snprintf(std::size_t iterations, double max_rel_error) {
    std::mt19937_64 rng(7891011);
    std::uniform_int_distribution<uint64_t> dist;

    for (std::size_t i = 0; i < iterations; ++i) {
        uint64_t bits = dist(rng);
        double d;
        std::memcpy(&d, &bits, sizeof(d));

        if (std::isnan(d) || std::isinf(d)) {
            continue;
        }

        for (int prec : {1, 2, 6, 10, 14}) {
            char our_buf[JsonFusion::fp_to_str_detail::NumberBufSize];
            char* our_end = format_double_to_chars(our_buf, d, prec);
            *our_end = '\0';

            char ref_buf[JsonFusion::fp_to_str_detail::NumberBufSize];
            int n = std::snprintf(ref_buf, sizeof(ref_buf), "%.*g", prec, d);
            if (n <= 0 || static_cast<std::size_t>(n) >= sizeof(ref_buf)) {
                continue; // weird snprintf failure, ignore
            }

            // Now parse both with strtod and compare doubles
            char* endp1 = nullptr;
            char* endp2 = nullptr;
            double d1 = std::strtod(our_buf, &endp1);
            double d2 = std::strtod(ref_buf, &endp2);

            double rel = std::abs(d1 - d2) / std::max(std::abs(d1), std::abs(d2));

            if (rel > std::max(max_rel_error, allowed_for_prec(prec))) {
                std::cerr << "Format mismatch vs snprintf:\n"
                          << "  value   : " << std::setprecision(17) << d << "\n"
                          << "  our str : " << our_buf << "\n"
                          << "  ref str : " << ref_buf << "\n"
                          << "  our d   : " << d1 << "\n"
                          << "  ref d   : " << d2 << "\n"
                          << "  rel:      " << rel << "\n"
                          << "  prec:      " << prec << "\n";
                std::abort();
            }
        }
    }
}

std::string random_json_number(std::mt19937_64& rng) {
    std::uniform_int_distribution<int> choose(0, 9);

    std::string s;

    // optional sign

    if (choose(rng) < 2) { // 20% chance
        s.push_back('-');
    }

    // integer part (no leading zeros unless single 0)
    int first_digit = choose(rng);
    if (first_digit == 0) {
        s.push_back('0');
    } else {
        s.push_back(static_cast<char>('0' + first_digit));
        int extra = choose(rng); // 0..9 extra digits
        for (int i = 0; i < extra; ++i) {
            s.push_back(static_cast<char>('0' + choose(rng)));
        }
    }

    // optional fractional part
    if (choose(rng) < 5) { // 50% chance
        s.push_back('.');
        int frac_len = 1 + choose(rng); // at least 1 digit
        for (int i = 0; i < frac_len; ++i) {
            s.push_back(static_cast<char>('0' + choose(rng)));
        }
    }

    // optional exponent
    if (choose(rng) < 4) { // 40% chance
        s.push_back(choose(rng) < 5 ? 'e' : 'E');
        if (choose(rng) < 4) { // 40% chance of sign
            s.push_back(choose(rng) < 5 ? '-' : '+');
        }
        int exp_len = 1 + choose(rng); // 1..10 digits
        for (int i = 0; i < exp_len; ++i) {
            s.push_back(static_cast<char>('0' + choose(rng)));
        }
    }

    return s;
}

void fuzz_parse_vs_strtod(std::size_t iterations, double max_rel_error) {
    std::mt19937_64 rng(555666);
    for (std::size_t i = 0; i < iterations; ++i) {
        std::string s = random_json_number(rng);

        double our{};
        if (!parse_number_to_double(s.c_str(), our)) {
            // For well-formed JSON numbers our parser should accept; if not
            // you can log and break.
            std::cerr << "Our parser rejected: " << s << "\n";
            std::abort();
        }

        char* endp = nullptr;
        double ref = std::strtod(s.c_str(), &endp);

        if (!std::isfinite(ref) || !std::isfinite(our)) {
            continue; // or test separately if you care about extreme exponents
        }


        double rel = std::abs(ref - our) / std::max(std::abs(ref), std::abs(our));

        if (rel > max_rel_error) {
            std::cerr << "Parse mismatch vs strtod:\n"
                      << "  text : " << s << "\n"
                      << "  our  : " << std::setprecision(17) << our << "\n"
                      << "  ref  : " << ref << "\n"
                      << "  rel  : " << rel << "\n";
            std::abort();
        }
    }
}

int main() {
    int iters = 100000000;
    double rel_error = 1e-11;
    for(int i = 0; i < 10; i ++) {
        std::cout << "\nREL ERROR THRESHOLD: " << rel_error << std::endl;
        fuzz_parse_vs_strtod(iters, rel_error);      // parser vs strtod
        std::cout << "fuzz_parse_vs_strtod done" << std::endl;
        fuzz_roundtrip_self(iters, rel_error);       // dtoa + parser
        std::cout << "fuzz_roundtrip_self done" << std::endl;
        fuzz_format_vs_snprintf(iters, rel_error);   // dtoa vs snprintf
        std::cout << "fuzz_format_vs_snprintf done" << std::endl;

        rel_error /= 10;
    }
    std::cout << "All tests passed.\n";
    return 0;
}
