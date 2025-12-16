#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/validators.hpp>
#include <array>

using namespace JsonFusion;
using namespace JsonFusion::validators;

// Constexpr-compatible absolute value for testing
template<typename T>
constexpr T test_abs(T value) {
    return value < T(0) ? -value : value;
}

// ============================================================================
// Test: constant<> - Boolean Constants
// ============================================================================

// Test: constant<true> accepts true
constexpr bool test_constant_bool_true_valid() {
    struct Test {
        Annotated<bool, constant<true>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": true})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == true;
}
static_assert(test_constant_bool_true_valid(), "constant<true> accepts true");

// Test: constant<true> rejects false
constexpr bool test_constant_bool_true_invalid() {
    struct Test {
        Annotated<bool, constant<true>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": false})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::wrong_constant_value;
}
static_assert(test_constant_bool_true_invalid(), "constant<true> rejects false");

// Test: constant<false> accepts false
constexpr bool test_constant_bool_false_valid() {
    struct Test {
        Annotated<bool, constant<false>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": false})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == false;
}
static_assert(test_constant_bool_false_valid(), "constant<false> accepts false");

// Test: constant<false> rejects true
constexpr bool test_constant_bool_false_invalid() {
    struct Test {
        Annotated<bool, constant<false>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": true})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::wrong_constant_value;
}
static_assert(test_constant_bool_false_invalid(), "constant<false> rejects true");

// ============================================================================
// Test: constant<> - Integer Constants
// ============================================================================

// Test: constant<42> accepts 42
constexpr bool test_constant_int_valid() {
    struct Test {
        Annotated<int, constant<42>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 42})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == 42;
}
static_assert(test_constant_int_valid(), "constant<42> accepts 42");

// Test: constant<42> rejects different value
constexpr bool test_constant_int_invalid() {
    struct Test {
        Annotated<int, constant<42>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 43})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::wrong_constant_value;
}
static_assert(test_constant_int_invalid(), "constant<42> rejects 43");

// Test: constant<0> accepts zero
constexpr bool test_constant_zero_valid() {
    struct Test {
        Annotated<int, constant<0>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 0})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == 0;
}
static_assert(test_constant_zero_valid(), "constant<0> accepts zero");

// Test: constant<-100> accepts negative
constexpr bool test_constant_negative_valid() {
    struct Test {
        Annotated<int, constant<-100>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": -100})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == -100;
}
static_assert(test_constant_negative_valid(), "constant<-100> accepts negative");

// Test: constant<-100> rejects positive
constexpr bool test_constant_negative_invalid() {
    struct Test {
        Annotated<int, constant<-100>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 100})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::wrong_constant_value;
}
static_assert(test_constant_negative_invalid(), "constant<-100> rejects positive");

// ============================================================================
// Test: string_constant<> - String Constants
// ============================================================================

// Test: string_constant<"hello"> accepts "hello"
constexpr bool test_string_constant_valid() {
    struct Test {
        Annotated<std::array<char, 32>, string_constant<"hello">> value;
    };
    
    Test obj{};
    std::string json = R"({"value": "hello"})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    
    // Check the value matches
    std::string_view expected = "hello";
    std::string_view actual = obj.value.get().data();
    return actual.substr(0, expected.size()) == expected;
}
static_assert(test_string_constant_valid(), "string_constant<\"hello\"> accepts \"hello\"");

// Test: string_constant<"hello"> rejects different string
constexpr bool test_string_constant_invalid() {
    struct Test {
        Annotated<std::array<char, 32>, string_constant<"hello">> value;
    };
    
    Test obj{};
    std::string json = R"({"value": "world"})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::wrong_constant_value;
}
static_assert(test_string_constant_invalid(), "string_constant<\"hello\"> rejects \"world\"");

// Test: string_constant<""> accepts empty string
constexpr bool test_string_constant_empty_valid() {
    struct Test {
        Annotated<std::array<char, 32>, string_constant<"">> value;
    };
    
    Test obj{};
    std::string json = R"({"value": ""})";
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_string_constant_empty_valid(), "string_constant<\"\"> accepts empty string");

// Test: string_constant<"test"> rejects empty string
constexpr bool test_string_constant_empty_invalid() {
    struct Test {
        Annotated<std::array<char, 32>, string_constant<"test">> value;
    };
    
    Test obj{};
    std::string json = R"({"value": ""})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::wrong_constant_value;
}
static_assert(test_string_constant_empty_invalid(), "string_constant<\"test\"> rejects empty string");

// Test: string_constant is case-sensitive
constexpr bool test_string_constant_case_sensitive() {
    struct Test {
        Annotated<std::array<char, 32>, string_constant<"Hello">> value;
    };
    
    Test obj{};
    std::string json = R"({"value": "hello"})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::wrong_constant_value;
}
static_assert(test_string_constant_case_sensitive(), "string_constant is case-sensitive");

// Test: string_constant with special characters
constexpr bool test_string_constant_special_chars() {
    struct Test {
        Annotated<std::array<char, 64>, string_constant<"test-value_123">> value;
    };
    
    Test obj{};
    std::string json = R"({"value": "test-value_123"})";
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_string_constant_special_chars(), "string_constant with special characters");

// ============================================================================
// Test: Multiple constant fields in same struct
// ============================================================================

constexpr bool test_multiple_constants() {
    struct Test {
        Annotated<bool, constant<true>> flag;
        Annotated<int, constant<42>> number;
        Annotated<std::array<char, 32>, string_constant<"test">> text;
    };
    
    Test obj{};
    std::string json = R"({"flag": true, "number": 42, "text": "test"})";
    auto result = Parse(obj, json);
    
    return result 
        && obj.flag.get() == true
        && obj.number.get() == 42;
}
static_assert(test_multiple_constants(), "Multiple constant fields in same struct");

constexpr bool test_multiple_constants_one_fails() {
    struct Test {
        Annotated<bool, constant<true>> flag;
        Annotated<int, constant<42>> number;
    };
    
    Test obj{};
    std::string json = R"({"flag": true, "number": 43})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::wrong_constant_value;
}
static_assert(test_multiple_constants_one_fails(), "Multiple constants - one fails");

// ============================================================================
// Test: constant<> - Float Constants
// ============================================================================

// Test: constant<3.14f> accepts 3.14 (float)
constexpr bool test_constant_float_valid() {
    struct Test {
        Annotated<float, constant<3.14f>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 3.14})";
    auto result = Parse(obj, json);
    
    return result && test_abs(obj.value.get() - 3.14f) < 0.001f;
}
static_assert(test_constant_float_valid(), "constant<3.14f> accepts 3.14");

// Test: constant<3.14f> rejects different value
constexpr bool test_constant_float_invalid() {
    struct Test {
        Annotated<float, constant<3.14f>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 2.71})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::wrong_constant_value;
}
static_assert(test_constant_float_invalid(), "constant<3.14f> rejects 2.71");

// Test: constant<0.0f> accepts zero (float)
constexpr bool test_constant_float_zero_valid() {
    struct Test {
        Annotated<float, constant<0.0f>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 0.0})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == 0.0f;
}
static_assert(test_constant_float_zero_valid(), "constant<0.0f> accepts zero");

// Test: constant<-2.5f> accepts negative (float)
constexpr bool test_constant_float_negative_valid() {
    struct Test {
        Annotated<float, constant<-2.5f>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": -2.5})";
    auto result = Parse(obj, json);
    
    return result && test_abs(obj.value.get() - (-2.5f)) < 0.001f;
}
static_assert(test_constant_float_negative_valid(), "constant<-2.5f> accepts negative");

// ============================================================================
// Test: constant<> - Double Constants
// ============================================================================

// Test: constant<3.14> accepts 3.14 (double)
constexpr bool test_constant_double_valid() {
    struct Test {
        Annotated<double, constant<3.14>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 3.14})";
    auto result = Parse(obj, json);
    
    return result && test_abs(obj.value.get() - 3.14) < 0.0001;
}
static_assert(test_constant_double_valid(), "constant<3.14> accepts 3.14");

// Test: constant<3.14> rejects different value
constexpr bool test_constant_double_invalid() {
    struct Test {
        Annotated<double, constant<3.14>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 2.71})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::wrong_constant_value;
}
static_assert(test_constant_double_invalid(), "constant<3.14> rejects 2.71");

// Test: constant<0.0> accepts zero (double)
constexpr bool test_constant_double_zero_valid() {
    struct Test {
        Annotated<double, constant<0.0>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 0.0})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == 0.0;
}
static_assert(test_constant_double_zero_valid(), "constant<0.0> accepts zero");

// ============================================================================
// Test: Type Mismatch (compile-time rejection)
// ============================================================================

// Note: The following WOULD fail at compile time (static_assert in validator):
// struct BadNarrowing { A<float, constant<3.14>> value; };   // double→float narrowing
// struct BadWidening  { A<double, constant<3.14f>> value; };  // float→double widening causes precision mismatch
// Both require exact type match: use 3.14f for float, 3.14 for double

