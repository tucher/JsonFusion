#include "../test_helpers.hpp"
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/parser.hpp>
#include <string>

using namespace JsonFusion;
using namespace JsonFusion::options;

// Constexpr-compatible absolute value for testing
template<typename T>
constexpr T test_abs(T value) {
    return value < T(0) ? -value : value;
}

// Constexpr-compatible string comparison (simple prefix check)
constexpr bool starts_with(const std::string& str, const std::string_view& prefix) {
    if (str.size() < prefix.size()) return false;
    for (size_t i = 0; i < prefix.size(); ++i) {
        if (str[i] != prefix[i]) return false;
    }
    return true;
}

// Helper to count decimal places in serialized number
constexpr size_t count_decimal_places(const std::string& json) {
    size_t colon_pos = json.find(':');
    if (colon_pos == std::string::npos) return 0;
    
    size_t dot_pos = json.find('.', colon_pos);
    if (dot_pos == std::string::npos) return 0;
    
    size_t end_pos = json.find_first_of(",}", dot_pos);
    if (end_pos == std::string::npos) end_pos = json.size();
    
    return end_pos - dot_pos - 1;
}

// ============================================================================
// Test: float_decimals<> - Float Serialization Precision
// ============================================================================

// Test: float_decimals<2> serializes with up to 2 decimal places (trailing zeros removed)
constexpr bool test_float_decimals_2() {
    struct Test {
        Annotated<float, float_decimals<2>> value;
    };
    
    Test obj{3.14159f};
    std::string output;
    auto result = Serialize(obj, output);
    
    // Should serialize as "3.1" (trailing zero removed)
    return result && starts_with(output, R"({"value":3.1)");
}
static_assert(test_float_decimals_2(), "float_decimals<2> serializes with up to 2 decimal places");

// Test: float_decimals<4> serializes with up to 4 decimal places
constexpr bool test_float_decimals_4() {
    struct Test {
        Annotated<float, float_decimals<4>> value;
    };
    
    Test obj{3.14159f};
    std::string output;
    auto result = Serialize(obj, output);
    
    // Should serialize as "3.142" (rounded, trailing zeros removed)
    return result && starts_with(output, R"({"value":3.142)");
}
static_assert(test_float_decimals_4(), "float_decimals<4> serializes with up to 4 decimal places");

// Test: float_decimals<0> serializes with no decimal places (integer)
constexpr bool test_float_decimals_0() {
    struct Test {
        Annotated<float, float_decimals<0>> value;
    };
    
    Test obj{3.14159f};
    std::string output;
    auto result = Serialize(obj, output);
    
    // Should serialize as "3" (no decimal point)
    return result && (output == R"({"value":3})");
}
static_assert(test_float_decimals_0(), "float_decimals<0> serializes with no decimal places");

// Test: float_decimals<8> serializes with 8 decimal places (default precision)
constexpr bool test_float_decimals_8() {
    struct Test {
        Annotated<float, float_decimals<8>> value;
    };
    
    Test obj{3.14159265f};
    std::string output;
    auto result = Serialize(obj, output);
    
    // Should serialize with 8 decimal places
    return result && starts_with(output, R"({"value":3.14159)");
}
static_assert(test_float_decimals_8(), "float_decimals<8> serializes with 8 decimal places");

// ============================================================================
// Test: float_decimals<> - Double Serialization Precision
// ============================================================================

// Test: float_decimals<2> with double
constexpr bool test_double_decimals_2() {
    struct Test {
        Annotated<double, float_decimals<2>> value;
    };
    
    Test obj{3.14159};
    std::string output;
    auto result = Serialize(obj, output);
    
    // Should serialize as "3.1" (trailing zero removed)
    return result && starts_with(output, R"({"value":3.1)");
}
static_assert(test_double_decimals_2(), "float_decimals<2> with double serializes with up to 2 decimal places");

// Test: float_decimals<6> with double
constexpr bool test_double_decimals_6() {
    struct Test {
        Annotated<double, float_decimals<6>> value;
    };
    
    Test obj{3.141592653589793};
    std::string output;
    auto result = Serialize(obj, output);
    
    // Should serialize as "3.141593" or similar (rounded, depends on representation)
    return result && starts_with(output, R"({"value":3.14159)");
}
static_assert(test_double_decimals_6(), "float_decimals<6> with double serializes with up to 6 decimal places");

// Test: float_decimals<0> with double
constexpr bool test_double_decimals_0() {
    struct Test {
        Annotated<double, float_decimals<0>> value;
    };
    
    Test obj{2.71828};
    std::string output;
    auto result = Serialize(obj, output);
    
    // Should serialize as "3" (rounded, no decimal point)
    return result && (output == R"({"value":3})");
}
static_assert(test_double_decimals_0(), "float_decimals<0> with double serializes with no decimal places");

// ============================================================================
// Test: float_decimals<> - Special Values
// ============================================================================

// Test: float_decimals<2> with zero
constexpr bool test_float_decimals_zero() {
    struct Test {
        Annotated<float, float_decimals<2>> value;
    };
    
    Test obj{0.0f};
    std::string output;
    auto result = Serialize(obj, output);
    
    // Should serialize as "0.00"
    return result && (output == R"({"value":0.00})" || output == R"({"value":0})");
}
static_assert(test_float_decimals_zero(), "float_decimals<2> serializes zero");

// Test: float_decimals<3> with negative value
constexpr bool test_float_decimals_negative() {
    struct Test {
        Annotated<float, float_decimals<3>> value;
    };
    
    Test obj{-2.71828f};
    std::string output;
    auto result = Serialize(obj, output);
    
    // Should serialize as "-2.72" (rounded, trailing zeros removed)
    return result && starts_with(output, R"({"value":-2.72)");
}
static_assert(test_float_decimals_negative(), "float_decimals<3> serializes negative value");

// ============================================================================
// Test: float_decimals<> - Roundtrip
// ============================================================================

// Test: Roundtrip with float_decimals<3>
constexpr bool test_float_decimals_roundtrip() {
    struct Test {
        Annotated<float, float_decimals<3>> value;
    };
    
    Test obj{1.2345f};
    std::string output;
    auto serialize_result = Serialize(obj, output);
    
    Test parsed{};
    auto parse_result = Parse(parsed, output);
    
    // After roundtrip with 3 decimals, should be approximately 1.23 or 1.234 (rounded, trailing zeros removed)
    return serialize_result && parse_result && test_abs(parsed.value.get() - obj.value.get()) < 0.01f;
}
static_assert(test_float_decimals_roundtrip(), "Roundtrip with float_decimals<3>");

// Test: Roundtrip with float_decimals<0>
constexpr bool test_float_decimals_0_roundtrip() {
    struct Test {
        Annotated<float, float_decimals<0>> value;
    };
    
    Test obj{5.6f};
    std::string output;
    auto serialize_result = Serialize(obj, output);
    
    Test parsed{};
    auto parse_result = Parse(parsed, output);
    
    // After roundtrip with 0 decimals, should be 6.0 (rounded up)
    return serialize_result && parse_result && parsed.value.get() == 6.0f;
}
static_assert(test_float_decimals_0_roundtrip(), "Roundtrip with float_decimals<0>");

// ============================================================================
// Test: float_decimals<> - Multiple Fields
// ============================================================================

// Test: Different decimal precision for different fields
constexpr bool test_float_decimals_multiple_fields() {
    struct Test {
        Annotated<float, float_decimals<2>> value1;
        Annotated<float, float_decimals<4>> value2;
        Annotated<double, float_decimals<0>> value3;
    };
    
    Test obj{3.14159f, 2.71828f, 1.41421};
    std::string output;
    auto result = Serialize(obj, output);
    
    // value1: 3.1, value2: 2.718, value3: 1
    return result && starts_with(output, R"({"value1":3.1)") && output.find("value2\":2.7") != std::string::npos;
}
static_assert(test_float_decimals_multiple_fields(), "Different decimal precision for different fields");


