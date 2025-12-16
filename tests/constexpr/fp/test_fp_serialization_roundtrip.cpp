#include "../test_helpers.hpp"
#include <limits>

using namespace TestHelpers;

struct Config {
    double value;
};

// Helper: Serialize value, parse back, compare semantically
template<typename T>
constexpr bool RoundTripValue(const T& original) {
    // Serialize
    std::string serialized;
    if (!JsonFusion::Serialize(original, serialized)) {
        return false;
    }
    
    // Parse back
    T parsed{};
    if (!JsonFusion::Parse(parsed, serialized)) {
        return false;
    }
    
    // Compare semantically
    return DeepEqual(original, parsed);
}

// ============================================================================
// Test: Basic Roundtrip - Exact Values
// ============================================================================

// Test: Roundtrip 1.0
static_assert(RoundTripValue(Config{1.0}));

// Test: Roundtrip 2.0
static_assert(RoundTripValue(Config{2.0}));

// Test: Roundtrip 0.5
static_assert(RoundTripValue(Config{0.5}));

// Test: Roundtrip 0.0
static_assert(RoundTripValue(Config{0.0}));

// Test: Roundtrip -0.0
static_assert(RoundTripValue(Config{-0.0}));

// ============================================================================
// Test: Roundtrip - Powers of 2
// ============================================================================

// Test: Roundtrip 2^10 = 1024
static_assert(RoundTripValue(Config{1024.0}));

// Test: Roundtrip 2^20 = 1048576
static_assert(RoundTripValue(Config{1048576.0}));

// Test: Roundtrip 2^-1 = 0.5
static_assert(RoundTripValue(Config{0.5}));

// Test: Roundtrip 2^-2 = 0.25
static_assert(RoundTripValue(Config{0.25}));

// Test: Roundtrip 2^-10 = 0.0009765625
static_assert(RoundTripValue(Config{0.0009765625}));

// ============================================================================
// Test: Roundtrip - Powers of 10
// ============================================================================

// Test: Roundtrip 10^1 = 10
static_assert(RoundTripValue(Config{10.0}));

// Test: Roundtrip 10^2 = 100
static_assert(RoundTripValue(Config{100.0}));

// Test: Roundtrip 10^3 = 1000
static_assert(RoundTripValue(Config{1000.0}));

// Test: Roundtrip 10^6 = 1000000
static_assert(RoundTripValue(Config{1000000.0}));

// Test: Roundtrip 10^10
static_assert(RoundTripValue(Config{10000000000.0}));

// ============================================================================
// Test: Roundtrip - Scientific Notation
// ============================================================================

// Test: Roundtrip 1e10
static_assert(RoundTripValue(Config{1e10}));

// Test: Roundtrip 1e20
static_assert(RoundTripValue(Config{1e20}));

// Test: Roundtrip 1e100
static_assert(RoundTripValue(Config{1e100}));

// Test: Roundtrip 1e200
static_assert(RoundTripValue(Config{1e200}));

// Test: Roundtrip 1e-10
static_assert(RoundTripValue(Config{1e-10}));

// Test: Roundtrip 1e-100
static_assert(RoundTripValue(Config{1e-100}));

// Test: Roundtrip 1e-200
static_assert(RoundTripValue(Config{1e-200}));

// ============================================================================
// Test: Roundtrip - Subnormal Numbers
// ============================================================================

// Test: Roundtrip very small (subnormal range)
static_assert(RoundTripValue(Config{1e-320}));

// Test: Roundtrip very small (subnormal range)
static_assert(RoundTripValue(Config{1e-322}));

// Test: Roundtrip near smallest positive
static_assert(RoundTripValue(Config{1e-323}));

// Test: Roundtrip negative subnormal
static_assert(RoundTripValue(Config{-1e-320}));

// ============================================================================
// Test: Roundtrip - Extreme Values
// ============================================================================

// Test: Roundtrip large value
static_assert(RoundTripValue(Config{1e307}));

// Test: Roundtrip very large value
static_assert(RoundTripValue(Config{1e308}));

// Test: Roundtrip negative large value
static_assert(RoundTripValue(Config{-1e308}));

// ============================================================================
// Test: Serialization Format Verification
// ============================================================================

// Test: 0.0 serializes as "0" or "0.0"
constexpr bool test_serialize_zero() {
    Config obj{0.0};
    std::string output;
    if (!JsonFusion::Serialize(obj, output)) return false;
    // Should be "0" or "0.0" (both valid)
    return output == R"({"value":0})" || output == R"({"value":0.0})";
}
static_assert(test_serialize_zero(), "Serialize 0.0");

// Test: 1.0 serializes as "1" or "1.0"
constexpr bool test_serialize_one() {
    Config obj{1.0};
    std::string output;
    if (!JsonFusion::Serialize(obj, output)) return false;
    return output == R"({"value":1})" || output == R"({"value":1.0})";
}
static_assert(test_serialize_one(), "Serialize 1.0");

// Test: 0.5 serializes correctly
constexpr bool test_serialize_half() {
    Config obj{0.5};
    std::string output;
    if (!JsonFusion::Serialize(obj, output)) return false;
    return output == R"({"value":0.5})";
}
static_assert(test_serialize_half(), "Serialize 0.5");

// Test: 3.14 serializes correctly
constexpr bool test_serialize_pi_approx() {
    Config obj{3.14};
    std::string output;
    if (!JsonFusion::Serialize(obj, output)) return false;
    return output.find(R"("value":3.14)") != std::string::npos;
}
static_assert(test_serialize_pi_approx(), "Serialize 3.14");

// Test: Large value serializes correctly (format may vary)
constexpr bool test_serialize_large() {
    Config obj{1e10};
    std::string output;
    if (!JsonFusion::Serialize(obj, output)) return false;
    // Parse back and verify value is preserved
    Config parsed{};
    return JsonFusion::Parse(parsed, output) && parsed.value == 1e10;
}
static_assert(test_serialize_large(), "Serialize 1e10");

// Test: Small value serializes correctly (format may vary)
constexpr bool test_serialize_small() {
    Config obj{1e-10};
    std::string output;
    if (!JsonFusion::Serialize(obj, output)) return false;
    // Parse back and verify value is preserved
    Config parsed{};
    return JsonFusion::Parse(parsed, output) && parsed.value == 1e-10;
}
static_assert(test_serialize_small(), "Serialize 1e-10");

// ============================================================================
// Test: Roundtrip - Common Decimal Fractions
// ============================================================================

// Test: Roundtrip 0.1 (not exactly representable, but should roundtrip)
static_assert(RoundTripValue(Config{0.1}));

// Test: Roundtrip 0.2
static_assert(RoundTripValue(Config{0.2}));

// Test: Roundtrip 0.125 (exactly representable as 2^-3)
static_assert(RoundTripValue(Config{0.125}));

// Test: Roundtrip 2.5 (exactly representable)
static_assert(RoundTripValue(Config{2.5}));

// ============================================================================
// Test: Roundtrip - Negative Values
// ============================================================================

// Test: Roundtrip -1.0
static_assert(RoundTripValue(Config{-1.0}));

// Test: Roundtrip -0.5
static_assert(RoundTripValue(Config{-0.5}));

// Test: Roundtrip -3.14
static_assert(RoundTripValue(Config{-3.14}));

// Test: Roundtrip -1e10
static_assert(RoundTripValue(Config{-1e10}));

// Test: Roundtrip -1e-10
static_assert(RoundTripValue(Config{-1e-10}));

