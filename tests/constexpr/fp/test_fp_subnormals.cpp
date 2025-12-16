#include "../test_helpers.hpp"
#include <limits>

using namespace TestHelpers;

struct Config {
    double value;
};

// ============================================================================
// Test: Subnormal (Denormalized) Numbers
// ============================================================================

// Subnormal numbers are those smaller than DBL_MIN (2.2250738585072014e-308)
// They use gradual underflow to represent very small values

// Test: DBL_MIN (smallest normal double)
static_assert(TestParse(R"({"value":2.2250738585072014e-308})", Config{2.2250738585072014e-308}));

// Test: Very small positive (deep subnormal range)
static_assert(TestParse(R"({"value":1e-320})", Config{1e-320}));

// Test: Very small positive (deep subnormal range)
static_assert(TestParse(R"({"value":1e-322})", Config{1e-322}));

// Test: Near smallest positive double (5e-324 is the smallest positive)
static_assert(TestParse(R"({"value":1e-323})", Config{1e-323}));

// ============================================================================
// Test: Negative Subnormals
// ============================================================================

// Test: Negative DBL_MIN
static_assert(TestParse(R"({"value":-2.2250738585072014e-308})", Config{-2.2250738585072014e-308}));

// Test: Negative subnormal
static_assert(TestParse(R"({"value":-1e-320})", Config{-1e-320}));

// Test: Negative deep subnormal
static_assert(TestParse(R"({"value":-1e-323})", Config{-1e-323}));

// ============================================================================
// Test: Transition Between Normal and Subnormal
// ============================================================================

// Test: Just above DBL_MIN (normal)
static_assert(TestParse(R"({"value":3e-308})", Config{3e-308}));

// Test: Deep in subnormal range
static_assert(TestParse(R"({"value":1e-310})", Config{1e-310}));

// Test: Very deep in subnormal range
static_assert(TestParse(R"({"value":1e-315})", Config{1e-315}));

// ============================================================================
// Test: Zero and Near-Zero
// ============================================================================

// Test: Positive zero
static_assert(TestParse(R"({"value":0.0})", Config{0.0}));

// Test: Negative zero
static_assert(TestParse(R"({"value":-0.0})", Config{-0.0}));

// Test: Very small that underflows to zero (beyond smallest double)
static_assert(TestParse(R"({"value":1e-325})", Config{0.0}));

// Test: Negative very small that underflows to zero
static_assert(TestParse(R"({"value":-1e-325})", Config{-0.0}));


