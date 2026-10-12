#include "../test_helpers.hpp"
#include <cmath>

using namespace TestHelpers;

struct Config {
    double value;
};

// ============================================================================
// Test: Short Decimals Parse to the Nearest Double
// ============================================================================

// Decimals with few significant digits and a small exponent must parse to the
// same double as the compiler's literal (correctly rounded), not one ULP off.

static_assert(TestParse(R"({"value":0.3})", Config{0.3}));
static_assert(TestParse(R"({"value":-0.3})", Config{-0.3}));
static_assert(TestParse(R"({"value":0.003})", Config{0.003}));
static_assert(TestParse(R"({"value":0.007})", Config{0.007}));
static_assert(TestParse(R"({"value":0.009})", Config{0.009}));
static_assert(TestParse(R"({"value":-0.0078})", Config{-0.0078}));
static_assert(TestParse(R"({"value":0.1248})", Config{0.1248}));
static_assert(TestParse(R"({"value":-0.2057})", Config{-0.2057}));
static_assert(TestParse(R"({"value":0.2367})", Config{0.2367}));
static_assert(TestParse(R"({"value":0.22423})", Config{0.22423}));
static_assert(TestParse(R"({"value":0.01177746578})", Config{0.01177746578}));
static_assert(TestParse(R"({"value":3.1415926535898})", Config{3.1415926535898}));
static_assert(TestParse(R"({"value":123456789.12346})", Config{123456789.12346}));
static_assert(TestParse(R"({"value":3e-5})", Config{3e-5}));
static_assert(TestParse(R"({"value":7.0e-3})", Config{7.0e-3}));

// ============================================================================
// Test: Exact Power-of-Ten Range Boundaries
// ============================================================================

// 10^22 is the largest power of ten exactly representable in a double.
static_assert(TestParse(R"({"value":1e22})", Config{1e22}));
static_assert(TestParse(R"({"value":1e-22})", Config{1e-22}));
static_assert(TestParse(R"({"value":3e-22})", Config{3e-22}));
static_assert(TestParse(R"({"value":4.5e21})", Config{4.5e21}));

// 2^53 is the largest mantissa converted exactly; 2^53 + 1 rounds to even.
static_assert(TestParse(R"({"value":9007199254740992e-10})", Config{9007199254740992e-10}));
static_assert(TestParse(R"({"value":9007199254740993})", Config{9007199254740992.0}));

// ============================================================================
// Test: Leading Zeros Do Not Consume Significant Digits
// ============================================================================

// Fractional leading zeros only shift the scale; the digits after them keep
// their full precision.
static_assert(TestParse(R"({"value":0.000000000000000001})", Config{1e-18}));
static_assert(TestParse(R"({"value":0.0000000000123456789})", Config{0.0000000000123456789}));
static_assert(TestParse(R"({"value":-0.00000000000000000025})", Config{-2.5e-19}));
static_assert(TestParse(R"({"value":0.000000000000000000000000000001})", Config{1e-30}));

// ============================================================================
// Test: Zero Shapes
// ============================================================================

static_assert(TestParse(R"({"value":0})", Config{0.0}));
static_assert(TestParse(R"({"value":0.0})", Config{0.0}));
static_assert(TestParse(R"({"value":0.000})", Config{0.0}));
static_assert(TestParse(R"({"value":0e10})", Config{0.0}));
static_assert(TestParse(R"({"value":0.0e-5})", Config{0.0}));
static_assert(TestParse<Config>(R"({"value":-0.0})", [](const Config& c) {
    return c.value == 0.0 && std::signbit(c.value);
}));
static_assert(TestParse<Config>(R"({"value":-0})", [](const Config& c) {
    return c.value == 0.0 && std::signbit(c.value);
}));
