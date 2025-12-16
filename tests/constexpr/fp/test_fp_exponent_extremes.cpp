#include "../test_helpers.hpp"
#include <limits>

using namespace TestHelpers;

struct Config {
    double value;
};

// ============================================================================
// Test: Near-Overflow Values
// ============================================================================

// Test: Approaching overflow from below
static_assert(TestParse(R"({"value":1.5e308})", Config{1.5e308}));

// Test: Large but well below overflow
static_assert(TestParse(R"({"value":1e307})", Config{1e307}));

// Test: 1e308 (maximum decade)
static_assert(TestParse(R"({"value":1e308})", Config{1e308}));

// ============================================================================
// Test: Near-Underflow Values
// ============================================================================

// Test: Minimum positive normal double (DBL_MIN)
static_assert(TestParse(R"({"value":2.2250738585072014e-308})", Config{2.2250738585072014e-308}));

// Test: Just above DBL_MIN
static_assert(TestParse(R"({"value":1e-307})", Config{1e-307}));

// ============================================================================
// Test: Negative Extremes
// ============================================================================

// Test: Negative large value
static_assert(TestParse(R"({"value":-1e308})", Config{-1e308}));

// Test: Negative DBL_MIN
static_assert(TestParse(R"({"value":-2.2250738585072014e-308})", Config{-2.2250738585072014e-308}));

// Test: Negative near DBL_MIN
static_assert(TestParse(R"({"value":-1e-307})", Config{-1e-307}));

// ============================================================================
// Test: Large Exponents (but not overflow)
// ============================================================================

// Test: 1e300
static_assert(TestParse(R"({"value":1e300})", Config{1e300}));

// Test: 1e200
static_assert(TestParse(R"({"value":1e200})", Config{1e200}));

// ============================================================================
// Test: Small Exponents (but not underflow to subnormal)
// ============================================================================

// Test: 1e-300
static_assert(TestParse(R"({"value":1e-300})", Config{1e-300}));

// Test: 1e-250
static_assert(TestParse(R"({"value":1e-250})", Config{1e-250}));

// Test: 1e-200
static_assert(TestParse(R"({"value":1e-200})", Config{1e-200}));

// ============================================================================
// Test: Negative Large/Small Exponents
// ============================================================================

// Test: -1e300
static_assert(TestParse(R"({"value":-1e300})", Config{-1e300}));

// Test: -1e-300
static_assert(TestParse(R"({"value":-1e-300})", Config{-1e-300}));

// ============================================================================
// Test: Fractional Coefficients with Extreme Exponents
// ============================================================================

// Test: 9.9e307 (near max)
static_assert(TestParse(R"({"value":9.9e307})", Config{9.9e307}));

// Test: 2.5e-307 (near min normal)
static_assert(TestParse(R"({"value":2.5e-307})", Config{2.5e-307}));

// Test: 0.5e308 = 5e307
static_assert(TestParse(R"({"value":0.5e308})", Config{0.5e308}));


