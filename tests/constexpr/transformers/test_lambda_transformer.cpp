#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/generic_transformers.hpp>
#include <string>
#include <charconv>

using namespace JsonFusion;
using namespace JsonFusion::transformers;

// ============================================================================
// Helper: Simple constexpr string to int conversion
// ============================================================================

constexpr bool parse_int(int& result, std::string_view sv) {
    if (sv.empty()) return false;
    
    result = 0;
    bool negative = false;
    std::size_t i = 0;
    
    if (sv[0] == '-') {
        negative = true;
        i = 1;
        if (sv.size() == 1) return false;
    }
    
    for (; i < sv.size(); ++i) {
        char c = sv[i];
        if (c < '0' || c > '9') return false;
        result = result * 10 + (c - '0');
    }
    
    if (negative) result = -result;
    return true;
}

constexpr bool int_to_string(const int& value, std::string& result) {
    result.clear();
    
    if (value == 0) {
        result = "0";
        return true;
    }
    
    int v = value;
    bool negative = v < 0;
    if (negative) v = -v;
    
    // Build string in reverse
    std::string temp;
    while (v > 0) {
        temp.push_back('0' + (v % 10));
        v /= 10;
    }
    
    if (negative) temp.push_back('-');
    
    // Reverse
    for (auto it = temp.rbegin(); it != temp.rend(); ++it) {
        result.push_back(*it);
    }
    
    return true;
}

// ============================================================================
// Test: Basic roundtrip with Transformed (int <-> string)
// ============================================================================

// Lambdas for transformation
constexpr auto string_to_int = [](int& stored, const std::string& wire) -> bool {
    return parse_int(stored, wire);
};

constexpr auto int_to_string_lambda = [](const int& stored, std::string& wire) -> bool {
    return int_to_string(stored, wire);
};

// Transformed type: stores int, wire is string
using IntAsString = Transformed<int, std::string, string_to_int, int_to_string_lambda>;

constexpr bool test_int_as_string_basic_roundtrip() {
    // Create model with int value
    struct Model {
        IntAsString number;
    };
    
    Model original;
    original.number = 42;
    
    // Serialize to JSON
    std::string json;
    auto serialize_result = Serialize(original, json);
    if (!serialize_result) return false;
    
    // JSON should have "42" as a string
    // Expected: {"number":"42"}
    if (json.find("\"42\"") == std::string::npos) return false;
    
    // Parse back
    Model parsed;
    auto parse_result = Parse(parsed, json);
    if (!parse_result) return false;
    
    // Should get back 42
    if (parsed.number.get() != 42) return false;
    
    return true;
}
static_assert(test_int_as_string_basic_roundtrip(), 
              "IntAsString should roundtrip correctly");

// ============================================================================
// Test: Negative numbers
// ============================================================================

constexpr bool test_int_as_string_negative() {
    struct Model {
        IntAsString value;
    };
    
    Model original;
    original.value = -123;
    
    std::string json;
    if (!Serialize(original, json)) return false;
    
    Model parsed;
    if (!Parse(parsed, json)) return false;
    
    if (parsed.value.get() != -123) return false;
    
    return true;
}
static_assert(test_int_as_string_negative(),
              "IntAsString should handle negative numbers");

// ============================================================================
// Test: Zero
// ============================================================================

constexpr bool test_int_as_string_zero() {
    struct Model {
        IntAsString value;
    };
    
    Model original;
    original.value = 0;
    
    std::string json;
    if (!Serialize(original, json)) return false;
    if (json.find("\"0\"") == std::string::npos) return false;
    
    Model parsed;
    if (!Parse(parsed, json)) return false;
    if (parsed.value.get() != 0) return false;
    
    return true;
}
static_assert(test_int_as_string_zero(),
              "IntAsString should handle zero");

// ============================================================================
// Test: Multiple fields with transformations
// ============================================================================

constexpr bool test_multiple_transformed_fields() {
    struct Model {
        IntAsString field1;
        IntAsString field2;
        int regular_int;
    };
    
    Model original;
    original.field1 = 100;
    original.field2 = 200;
    original.regular_int = 300;
    
    std::string json;
    if (!Serialize(original, json)) return false;
    
    // field1 and field2 should be strings, regular_int should be a number
    if (json.find("\"100\"") == std::string::npos) return false;
    if (json.find("\"200\"") == std::string::npos) return false;
    // regular_int should NOT have quotes
    if (json.find("300") == std::string::npos) return false;
    
    Model parsed;
    if (!Parse(parsed, json)) return false;
    
    if (parsed.field1.get() != 100) return false;
    if (parsed.field2.get() != 200) return false;
    if (parsed.regular_int != 300) return false;
    
    return true;
}
static_assert(test_multiple_transformed_fields(),
              "Multiple transformed fields should work together");

// ============================================================================
// Test: Transformed type satisfies transformer concepts
// ============================================================================

static_assert(static_schema::ParseTransformer<IntAsString>,
              "Transformed should satisfy ParseTransformer concept");

static_assert(static_schema::SerializeTransformer<IntAsString>,
              "Transformed should satisfy SerializeTransformer concept");

static_assert(std::is_same_v<
                  static_schema::parse_transform_traits<IntAsString>::wire_type,
                  std::string>,
              "IntAsString wire_type should be std::string");

static_assert(static_schema::is_parse_transformer_v<IntAsString>,
              "is_parse_transformer_v should detect Transformed");

static_assert(static_schema::is_serialize_transformer_v<IntAsString>,
              "is_serialize_transformer_v should detect Transformed");

// ============================================================================
// Test: Parse failure handling
// ============================================================================

constexpr bool test_parse_failure_invalid_string() {
    struct Model {
        IntAsString value;
    };
    
    // Try to parse invalid JSON (string that's not a valid number)
    std::string json = R"({"value":"not_a_number"})";
    
    Model parsed;
    auto result = Parse(parsed, json);
    
    // Should fail because "not_a_number" can't be parsed as int
    if (result) return false;
    
    return true;
}
static_assert(test_parse_failure_invalid_string(),
              "Should fail gracefully on invalid input");

// ============================================================================
// Test: Direct value access and manipulation
// ============================================================================

constexpr bool test_direct_value_access() {
    IntAsString wrapper;
    
    // Direct assignment
    wrapper = 99;
    if (wrapper.get() != 99) return false;
    
    // Implicit conversion
    int extracted = wrapper;
    if (extracted != 99) return false;
    
    // Modification
    wrapper.value = 88;
    if (wrapper.get() != 88) return false;
    
    return true;
}
static_assert(test_direct_value_access(),
              "Transformed should allow direct value access");

// ============================================================================
// Test: Comparison operators
// ============================================================================

constexpr bool test_comparison_operators() {
    IntAsString a, b;
    a = 10;
    b = 10;
    
    if (!(a == b)) return false;
    if (a != b) return false;
    
    b = 20;
    if (a == b) return false;
    if (!(a != b)) return false;
    
    // Compare with raw int
    if (!(a == 10)) return false;
    if (!(10 == a)) return false;
    
    return true;
}
static_assert(test_comparison_operators(),
              "Transformed comparison operators should work");

// ============================================================================
// Test: Nested structure with transformation
// ============================================================================

constexpr bool test_nested_with_transformation() {
    struct Inner {
        IntAsString id;
    };
    
    struct Outer {
        Inner inner;
        IntAsString outer_id;
    };
    
    Outer original;
    original.inner.id = 111;
    original.outer_id = 222;
    
    std::string json;
    if (!Serialize(original, json)) return false;
    
    Outer parsed;
    if (!Parse(parsed, json)) return false;
    
    if (parsed.inner.id.get() != 111) return false;
    if (parsed.outer_id.get() != 222) return false;
    
    return true;
}
static_assert(test_nested_with_transformation(),
              "Transformed should work in nested structures");


