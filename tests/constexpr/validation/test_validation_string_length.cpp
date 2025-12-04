#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/validators.hpp>
#include <array>

using namespace JsonFusion;
using namespace JsonFusion::validators;

// ============================================================================
// Test: min_length<> - Valid Values
// ============================================================================

// Test: min_length<5> accepts string with exactly 5 characters
constexpr bool test_min_length_exact_valid() {
    struct Test {
        Annotated<std::array<char, 32>, min_length<5>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": "hello"})";  // 5 chars
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_min_length_exact_valid(), "min_length<5> accepts string with exactly 5 characters");

// Test: min_length<5> accepts string longer than 5 characters
constexpr bool test_min_length_longer_valid() {
    struct Test {
        Annotated<std::array<char, 32>, min_length<5>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": "hello world"})";  // 11 chars
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_min_length_longer_valid(), "min_length<5> accepts string longer than 5 characters");

// Test: min_length<1> accepts single character
constexpr bool test_min_length_one_valid() {
    struct Test {
        Annotated<std::array<char, 32>, min_length<1>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": "a"})";
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_min_length_one_valid(), "min_length<1> accepts single character");

// ============================================================================
// Test: min_length<> - Invalid Values (Too Short)
// ============================================================================

// Test: min_length<5> rejects string shorter than 5 characters
constexpr bool test_min_length_too_short() {
    struct Test {
        Annotated<std::array<char, 32>, min_length<5>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": "hi"})";  // 2 chars < 5
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::string_length_out_of_range;
}
static_assert(test_min_length_too_short(), "min_length<5> rejects string shorter than 5 characters");

// Test: min_length<5> rejects empty string
constexpr bool test_min_length_empty_rejected() {
    struct Test {
        Annotated<std::array<char, 32>, min_length<5>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": ""})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::string_length_out_of_range;
}
static_assert(test_min_length_empty_rejected(), "min_length<5> rejects empty string");

// Test: min_length<1> rejects empty string
constexpr bool test_min_length_one_rejects_empty() {
    struct Test {
        Annotated<std::array<char, 32>, min_length<1>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": ""})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::string_length_out_of_range;
}
static_assert(test_min_length_one_rejects_empty(), "min_length<1> rejects empty string");

// ============================================================================
// Test: max_length<> - Valid Values
// ============================================================================

// Test: max_length<10> accepts string with exactly 10 characters
constexpr bool test_max_length_exact_valid() {
    struct Test {
        Annotated<std::array<char, 32>, max_length<10>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": "1234567890"})";  // 10 chars
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_max_length_exact_valid(), "max_length<10> accepts string with exactly 10 characters");

// Test: max_length<10> accepts string shorter than 10 characters
constexpr bool test_max_length_shorter_valid() {
    struct Test {
        Annotated<std::array<char, 32>, max_length<10>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": "hello"})";  // 5 chars < 10
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_max_length_shorter_valid(), "max_length<10> accepts string shorter than 10 characters");

// Test: max_length<10> accepts empty string
constexpr bool test_max_length_empty_valid() {
    struct Test {
        Annotated<std::array<char, 32>, max_length<10>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": ""})";
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_max_length_empty_valid(), "max_length<10> accepts empty string");

// ============================================================================
// Test: max_length<> - Invalid Values (Too Long)
// ============================================================================

// Test: max_length<10> rejects string longer than 10 characters
constexpr bool test_max_length_too_long() {
    struct Test {
        Annotated<std::array<char, 32>, max_length<10>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": "12345678901"})";  // 11 chars > 10
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::string_length_out_of_range;
}
static_assert(test_max_length_too_long(), "max_length<10> rejects string longer than 10 characters");

// Test: max_length<1> rejects string with 2 characters
constexpr bool test_max_length_one_rejects_two() {
    struct Test {
        Annotated<std::array<char, 32>, max_length<1>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": "ab"})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::string_length_out_of_range;
}
static_assert(test_max_length_one_rejects_two(), "max_length<1> rejects string with 2 characters");

// ============================================================================
// Test: min_length<> + max_length<> - Combined Constraints
// ============================================================================

// Test: min_length<5>, max_length<10> accepts string at min boundary
constexpr bool test_length_range_min_valid() {
    struct Test {
        Annotated<std::array<char, 32>, min_length<5>, max_length<10>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": "hello"})";  // 5 chars
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_length_range_min_valid(), "min_length<5>, max_length<10> accepts string at min boundary");

// Test: min_length<5>, max_length<10> accepts string at max boundary
constexpr bool test_length_range_max_valid() {
    struct Test {
        Annotated<std::array<char, 32>, min_length<5>, max_length<10>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": "1234567890"})";  // 10 chars
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_length_range_max_valid(), "min_length<5>, max_length<10> accepts string at max boundary");

// Test: min_length<5>, max_length<10> accepts string in middle
constexpr bool test_length_range_middle_valid() {
    struct Test {
        Annotated<std::array<char, 32>, min_length<5>, max_length<10>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": "test123"})";  // 7 chars
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_length_range_middle_valid(), "min_length<5>, max_length<10> accepts string in middle");

// Test: min_length<5>, max_length<10> rejects string too short
constexpr bool test_length_range_too_short() {
    struct Test {
        Annotated<std::array<char, 32>, min_length<5>, max_length<10>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": "test"})";  // 4 chars < 5
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::string_length_out_of_range;
}
static_assert(test_length_range_too_short(), "min_length<5>, max_length<10> rejects string too short");

// Test: min_length<5>, max_length<10> rejects string too long
constexpr bool test_length_range_too_long() {
    struct Test {
        Annotated<std::array<char, 32>, min_length<5>, max_length<10>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": "12345678901"})";  // 11 chars > 10
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::string_length_out_of_range;
}
static_assert(test_length_range_too_long(), "min_length<5>, max_length<10> rejects string too long");

// Test: min_length<5>, max_length<10> rejects empty string
constexpr bool test_length_range_empty_rejected() {
    struct Test {
        Annotated<std::array<char, 32>, min_length<5>, max_length<10>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": ""})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::string_length_out_of_range;
}
static_assert(test_length_range_empty_rejected(), "min_length<5>, max_length<10> rejects empty string");

// ============================================================================
// Test: Single character constraints
// ============================================================================

// Test: min_length<1>, max_length<1> accepts single character
constexpr bool test_length_exactly_one() {
    struct Test {
        Annotated<std::array<char, 32>, min_length<1>, max_length<1>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": "a"})";
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_length_exactly_one(), "min_length<1>, max_length<1> accepts single character");

// Test: min_length<1>, max_length<1> rejects empty
constexpr bool test_length_exactly_one_rejects_empty() {
    struct Test {
        Annotated<std::array<char, 32>, min_length<1>, max_length<1>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": ""})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::string_length_out_of_range;
}
static_assert(test_length_exactly_one_rejects_empty(), "min_length<1>, max_length<1> rejects empty");

// Test: min_length<1>, max_length<1> rejects two characters
constexpr bool test_length_exactly_one_rejects_two() {
    struct Test {
        Annotated<std::array<char, 32>, min_length<1>, max_length<1>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": "ab"})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::string_length_out_of_range;
}
static_assert(test_length_exactly_one_rejects_two(), "min_length<1>, max_length<1> rejects two characters");

// ============================================================================
// Test: Multiple length-constrained fields in same struct
// ============================================================================

constexpr bool test_multiple_length_constraints() {
    struct Test {
        Annotated<std::array<char, 32>, min_length<3>> short_field;
        Annotated<std::array<char, 64>, max_length<20>> long_field;
        Annotated<std::array<char, 32>, min_length<5>, max_length<10>> range_field;
    };
    
    Test obj{};
    std::string json = R"({"short_field": "abc", "long_field": "short", "range_field": "middle"})";
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_multiple_length_constraints(), "Multiple length-constrained fields in same struct");

constexpr bool test_multiple_length_constraints_one_fails() {
    struct Test {
        Annotated<std::array<char, 32>, min_length<3>> short_field;
        Annotated<std::array<char, 32>, max_length<5>> long_field;
    };
    
    Test obj{};
    std::string json = R"({"short_field": "abc", "long_field": "toolong"})";  // 7 chars > 5
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::string_length_out_of_range;
}
static_assert(test_multiple_length_constraints_one_fails(), "Multiple length constraints - one fails");

