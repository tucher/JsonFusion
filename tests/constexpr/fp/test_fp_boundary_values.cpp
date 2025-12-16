#include "../test_helpers.hpp"
#include <limits>

using namespace TestHelpers;

struct Config {
    double value;
};

// Constexpr-compatible absolute value
template<typename T>
constexpr T test_abs(T value) {
    return value < T(0) ? -value : value;
}

// Constexpr-compatible comparison with tolerance
constexpr bool close_enough(double a, double b, double tolerance = 1e-15) {
    return test_abs(a - b) <= tolerance * (test_abs(a) + test_abs(b) + 1.0);
}

// ============================================================================
// Test: Powers of 2 - Exact Values
// ============================================================================

// Test: 2^0 = 1
static_assert(TestParse(R"({"value":1.0})", Config{1.0}));

// Test: 2^1 = 2
static_assert(TestParse(R"({"value":2.0})", Config{2.0}));

// Test: 2^10 = 1024
static_assert(TestParse(R"({"value":1024.0})", Config{1024.0}));

// Test: 2^20 = 1048576
static_assert(TestParse(R"({"value":1048576.0})", Config{1048576.0}));

// Test: 2^53 = 9007199254740992 (largest exact integer in double)
static_assert(TestParse(R"({"value":9007199254740992.0})", Config{9007199254740992.0}));

// Test: 2^-1 = 0.5
static_assert(TestParse(R"({"value":0.5})", Config{0.5}));

// Test: 2^-2 = 0.25
static_assert(TestParse(R"({"value":0.25})", Config{0.25}));

// Test: 2^-10 = 0.0009765625
static_assert(TestParse(R"({"value":0.0009765625})", Config{0.0009765625}));

// ============================================================================
// Test: Powers of 10 - Common Values
// ============================================================================

// Test: 10^0 = 1
static_assert(TestParse(R"({"value":1.0})", Config{1.0}));

// Test: 10^1 = 10
static_assert(TestParse(R"({"value":10.0})", Config{10.0}));

// Test: 10^2 = 100
static_assert(TestParse(R"({"value":100.0})", Config{100.0}));

// Test: 10^3 = 1000
static_assert(TestParse(R"({"value":1000.0})", Config{1000.0}));

// Test: 10^6 = 1000000
static_assert(TestParse(R"({"value":1000000.0})", Config{1000000.0}));

// Test: 10^10
static_assert(TestParse(R"({"value":10000000000.0})", Config{10000000000.0}));

// ============================================================================
// Test: Scientific Notation - Powers of 10
// ============================================================================

// Test: 1e10
static_assert(TestParse(R"({"value":1e10})", Config{1e10}));

// Test: 1e20
static_assert(TestParse(R"({"value":1e20})", Config{1e20}));

// Test: 1e50
static_assert(TestParse(R"({"value":1e50})", Config{1e50}));

// Test: 1e100
static_assert(TestParse(R"({"value":1e100})", Config{1e100}));

// Test: 1e200
static_assert(TestParse(R"({"value":1e200})", Config{1e200}));

// Test: 1e-10 (exact representation)
static_assert(TestParse(R"({"value":1e-10})", Config{1e-10}));

// Test: 1e-100 (exact representation)  
static_assert(TestParse(R"({"value":1e-100})", Config{1e-100}));

// Test: 1e-200 (exact representation)
static_assert(TestParse(R"({"value":1e-200})", Config{1e-200}));

// ============================================================================
// Test: Common Decimal Fractions
// ============================================================================

// Note: 0.1, 0.2, 0.3 etc. are not exactly representable in binary
// These tests verify that parsing produces values close to the expected result

// Test: 0.1 (not exactly representable, but should roundtrip correctly)
static_assert(TestParse(R"({"value":0.1})", Config{0.1}));

// Test: 0.2
static_assert(TestParse(R"({"value":0.2})", Config{0.2}));

// Test: 0.5 (exactly representable as 2^-1)
static_assert(TestParse(R"({"value":0.5})", Config{0.5}));

// Test: 0.125 (exactly representable as 2^-3)
static_assert(TestParse(R"({"value":0.125})", Config{0.125}));

// Test: 3.14 (π approximation)
static_assert(TestParse(R"({"value":3.14})", Config{3.14}));

// Test: 2.5 (exactly representable)
static_assert(TestParse(R"({"value":2.5})", Config{2.5}));


