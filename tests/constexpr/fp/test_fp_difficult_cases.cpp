#include "../test_helpers.hpp"
#include <limits>

using namespace TestHelpers;

struct Config {
    double value;
};

// ============================================================================
// Test: Famous Difficult Cases from David Gay's Test Suite
// ============================================================================

// These are known problematic decimal-to-binary conversions that have
// historically caused issues in floating-point parsers

// Test: 1e23 - Common difficult case
static_assert(TestParse(R"({"value":1e23})", Config{1e23}));

// Test: 2.2250738585072014e-308 - Near DBL_MIN (smallest normal)
static_assert(TestParse(R"({"value":2.2250738585072014e-308})", Config{2.2250738585072014e-308}));

// ============================================================================
// Test: Rounding Boundary Cases
// ============================================================================

// Numbers that test rounding behavior

// Test: 1.0000000000000002 (1 + epsilon)
static_assert(TestParse(R"({"value":1.0000000000000002})", Config{1.0000000000000002}));

// Test: 2.0000000000000004 (2 + epsilon)
static_assert(TestParse(R"({"value":2.0000000000000004})", Config{2.0000000000000004}));

// ============================================================================
// Test: Powers of 10 Near Boundaries
// ============================================================================

// Test: 1e308 (very large, near overflow)
static_assert(TestParse(R"({"value":1e308})", Config{1e308}));

// ============================================================================
// Test: Long Decimal Mantissas
// ============================================================================

// Test: Long mantissa that requires careful parsing
static_assert(TestParse(R"({"value":1.2345678901234567})", Config{1.2345678901234567}));

// Test: Many significant digits
static_assert(TestParse(R"({"value":9.8765432109876543})", Config{9.8765432109876543}));

// Test: Long mantissa with small value
static_assert(TestParse(R"({"value":0.00000000123456789})", Config{0.00000000123456789}));

// ============================================================================
// Test: Combinations of Features
// ============================================================================

// Test: Negative with long mantissa
static_assert(TestParse(R"({"value":-1.2345678901234567})", Config{-1.2345678901234567}));

// Test: Large exponent
static_assert(TestParse(R"({"value":1.23e100})", Config{1.23e100}));

// Test: Small exponent
static_assert(TestParse(R"({"value":1.23e-100})", Config{1.23e-100}));

// ============================================================================
// Test: Powers of 2 Boundaries (2^53 region)
// ============================================================================

// Test: 2^53 (max exact integer in double)
static_assert(TestParse(R"({"value":9007199254740992})", Config{9007199254740992.0}));

// Test: 2^53 + 2 (next representable after 2^53)
static_assert(TestParse(R"({"value":9007199254740994})", Config{9007199254740994.0}));

// Test: 2^53 - 1 (largest integer < 2^53)
static_assert(TestParse(R"({"value":9007199254740991})", Config{9007199254740991.0}));

// ============================================================================
// Test: Negative Difficult Cases
// ============================================================================

// Test: Negative near DBL_MIN
static_assert(TestParse(R"({"value":-2.2250738585072014e-308})", Config{-2.2250738585072014e-308}));

// Test: Negative 1 + epsilon
static_assert(TestParse(R"({"value":-1.0000000000000002})", Config{-1.0000000000000002}));

// Test: Negative large value
static_assert(TestParse(R"({"value":-1e308})", Config{-1e308}));

// ============================================================================
// Test: Mixed Magnitude Cases
// ============================================================================

// Test: Very small with precise fractional part
static_assert(TestParse(R"({"value":1e-307})", Config{1e-307}));

// Test: Medium range
static_assert(TestParse(R"({"value":123456.789})", Config{123456.789}));


