#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/validators.hpp>
#include <array>

using namespace JsonFusion;
using namespace JsonFusion::validators;

// ============================================================================
// Test: Combined String Validators - min_length + max_length
// ============================================================================

// Test: min_length<5> + max_length<10> - all constraints pass
constexpr bool test_combined_length_valid() {
    struct Test {
        Annotated<std::array<char, 32>, min_length<5>, max_length<10>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": "hello"})";  // 5 chars, within range
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_combined_length_valid(), "min_length<5> + max_length<10> - all constraints pass");

// Test: min_length<5> + max_length<10> - fails min_length
constexpr bool test_combined_length_fails_min() {
    struct Test {
        Annotated<std::array<char, 32>, min_length<5>, max_length<10>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": "hi"})";  // 2 chars < 5
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::string_length_out_of_range;
}
static_assert(test_combined_length_fails_min(), "min_length<5> + max_length<10> - fails min_length");

// Test: min_length<5> + max_length<10> - fails max_length
constexpr bool test_combined_length_fails_max() {
    struct Test {
        Annotated<std::array<char, 32>, min_length<5>, max_length<10>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": "12345678901"})";  // 11 chars > 10
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::string_length_out_of_range;
}
static_assert(test_combined_length_fails_max(), "min_length<5> + max_length<10> - fails max_length");

// ============================================================================
// Test: Combined String Validators - min_length + max_length + enum_values
// ============================================================================

// Test: min_length<3> + max_length<10> + enum_values - all constraints pass
constexpr bool test_combined_length_enum_valid() {
    struct Test {
        Annotated<std::array<char, 32>, min_length<3>, max_length<10>, enum_values<"red", "green", "blue">> value;
    };
    
    Test obj{};
    std::string json = R"({"value": "red"})";  // 3 chars, in enum
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_combined_length_enum_valid(), "min_length<3> + max_length<10> + enum_values - all constraints pass");

// Test: min_length<3> + max_length<10> + enum_values - fails enum (valid length)
constexpr bool test_combined_length_enum_fails_enum() {
    struct Test {
        Annotated<std::array<char, 32>, min_length<3>, max_length<10>, enum_values<"red", "green", "blue">> value;
    };
    
    Test obj{};
    std::string json = R"({"value": "yellow"})";  // 6 chars (valid length), but not in enum
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::wrong_constant_value;
}
static_assert(test_combined_length_enum_fails_enum(), "min_length<3> + max_length<10> + enum_values - fails enum");

// Test: min_length<3> + max_length<10> + enum_values - fails min_length (before enum check)
constexpr bool test_combined_length_enum_fails_min() {
    struct Test {
        Annotated<std::array<char, 32>, min_length<3>, max_length<10>, enum_values<"red", "green", "blue">> value;
    };
    
    Test obj{};
    std::string json = R"({"value": "hi"})";  // 2 chars < 3
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::string_length_out_of_range;
}
static_assert(test_combined_length_enum_fails_min(), "min_length<3> + max_length<10> + enum_values - fails min_length");

// Test: min_length<3> + max_length<10> + enum_values - fails max_length (before enum check)
constexpr bool test_combined_length_enum_fails_max() {
    struct Test {
        Annotated<std::array<char, 32>, min_length<3>, max_length<10>, enum_values<"red", "green", "blue">> value;
    };
    
    Test obj{};
    std::string json = R"({"value": "12345678901"})";  // 11 chars > 10
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::string_length_out_of_range;
}
static_assert(test_combined_length_enum_fails_max(), "min_length<3> + max_length<10> + enum_values - fails max_length");

// ============================================================================
// Test: Combined String Validators - enum_values + min_length
// ============================================================================

// Test: enum_values + min_length - all constraints pass
constexpr bool test_combined_enum_length_valid() {
    struct Test {
        Annotated<std::array<char, 32>, enum_values<"small", "medium", "large">, min_length<4>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": "small"})";  // In enum, >= 4 chars
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_combined_enum_length_valid(), "enum_values + min_length - all constraints pass");

// Test: enum_values + min_length - fails enum (valid length)
constexpr bool test_combined_enum_length_fails_enum() {
    struct Test {
        Annotated<std::array<char, 32>, enum_values<"small", "medium", "large">, min_length<4>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": "tiny"})";  // 4 chars (valid length), but not in enum
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::wrong_constant_value;
}
static_assert(test_combined_enum_length_fails_enum(), "enum_values + min_length - fails enum");

// ============================================================================
// Test: Combined String Validators - Multiple Fields
// ============================================================================

constexpr bool test_combined_string_multiple_fields() {
    struct Test {
        Annotated<std::array<char, 32>, min_length<3>, max_length<10>> field1;
        Annotated<std::array<char, 32>, enum_values<"yes", "no">> field2;
        Annotated<std::array<char, 32>, min_length<5>, enum_values<"hello", "world">> field3;
    };
    
    Test obj{};
    std::string json = R"({"field1": "test", "field2": "yes", "field3": "hello"})";
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_combined_string_multiple_fields(), "Multiple fields with combined string validators");

constexpr bool test_combined_string_multiple_fields_one_fails() {
    struct Test {
        Annotated<std::array<char, 32>, min_length<3>, max_length<10>> field1;
        Annotated<std::array<char, 32>, enum_values<"yes", "no">> field2;
    };
    
    Test obj{};
    std::string json = R"({"field1": "test", "field2": "maybe"})";  // field2 not in enum
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::wrong_constant_value;
}
static_assert(test_combined_string_multiple_fields_one_fails(), "Multiple fields - one fails");

