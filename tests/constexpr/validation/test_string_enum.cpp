#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/validators.hpp>
#include <array>

using namespace JsonFusion;
using namespace JsonFusion::validators;

// ============================================================================
// Test: enum_values - Valid Enum Values
// ============================================================================

constexpr bool test_enum_valid_single() {
    Annotated<
        std::array<char, 32>,
        enum_values<"red", "green", "blue">
    > color;
    
    std::string_view json = R"("red")";
    auto result = Parse(color, json);
    
    return result;
}
static_assert(test_enum_valid_single());

constexpr bool test_enum_valid_all_values() {
    // Test all three enum values work
    Annotated<
        std::array<char, 32>,
        enum_values<"alpha", "beta", "gamma">
    > value1;
    
    Annotated<
        std::array<char, 32>,
        enum_values<"alpha", "beta", "gamma">
    > value2;
    
    Annotated<
        std::array<char, 32>,
        enum_values<"alpha", "beta", "gamma">
    > value3;
    
    auto r1 = Parse(value1, std::string_view(R"("alpha")"));
    auto r2 = Parse(value2, std::string_view(R"("beta")"));
    auto r3 = Parse(value3, std::string_view(R"("gamma")"));
    
    return r1 && r2 && r3;
}
static_assert(test_enum_valid_all_values());

// ============================================================================
// Test: enum_values - Invalid Value
// ============================================================================

constexpr bool test_enum_invalid() {
    Annotated<
        std::array<char, 32>,
        enum_values<"small", "medium", "large">
    > size;
    
    std::string_view json = R"("extra-large")";
    auto result = Parse(size, json);
    
    return !result;  // Should fail
}
static_assert(test_enum_invalid());

constexpr bool test_enum_empty_string() {
    Annotated<
        std::array<char, 32>,
        enum_values<"a", "b", "c">
    > value;
    
    std::string_view json = R"("")";
    auto result = Parse(value, json);
    
    return !result;  // Empty string should fail
}
static_assert(test_enum_empty_string());

// ============================================================================
// Test: enum_values - Early Rejection (Incremental)
// ============================================================================

constexpr bool test_enum_early_rejection() {
    Annotated<
        std::array<char, 32>,
        enum_values<"apple", "apricot", "banana">
    > fruit;
    
    // "cherry" should be rejected early when we hit 'c'
    std::string_view json = R"("cherry")";
    auto result = Parse(fruit, json);
    
    return !result;
}
static_assert(test_enum_early_rejection());

constexpr bool test_enum_prefix_not_match() {
    Annotated<
        std::array<char, 32>,
        enum_values<"test">
    > value;
    
    // "testing" is longer than "test"
    std::string_view json = R"("testing")";
    auto result = Parse(value, json);
    
    return !result;  // Should fail - not exact match
}
static_assert(test_enum_prefix_not_match());

constexpr bool test_enum_partial_match() {
    Annotated<
        std::array<char, 32>,
        enum_values<"hello">
    > value;
    
    // "hell" is shorter than "hello"
    std::string_view json = R"("hell")";
    auto result = Parse(value, json);
    
    return !result;  // Should fail - not complete
}
static_assert(test_enum_partial_match());

// ============================================================================
// Test: enum_values - Case Sensitivity
// ============================================================================

constexpr bool test_enum_case_sensitive() {
    Annotated<
        std::array<char, 32>,
        enum_values<"Active", "Inactive">
    > status;
    
    std::string_view json = R"("active")";  // lowercase
    auto result = Parse(status, std::string_view(json));
    
    return !result;  // Should fail - case sensitive
}
static_assert(test_enum_case_sensitive());

// ============================================================================
// Test: enum_values - Many Values (Binary Search)
// ============================================================================

constexpr bool test_enum_many_values() {
    Annotated<
        std::array<char, 32>,
        enum_values<
            "jan", "feb", "mar", "apr", "may", "jun",
            "jul", "aug", "sep", "oct", "nov", "dec"
        >
    > month;
    
    auto r1 = Parse(month, std::string_view(R"("jan")"));
    
    Annotated<
        std::array<char, 32>,
        enum_values<
            "jan", "feb", "mar", "apr", "may", "jun",
            "jul", "aug", "sep", "oct", "nov", "dec"
        >
    > month2;
    auto r2 = Parse(month2, std::string_view(R"("dec")"));
    
    Annotated<
        std::array<char, 32>,
        enum_values<
            "jan", "feb", "mar", "apr", "may", "jun",
            "jul", "aug", "sep", "oct", "nov", "dec"
        >
    > month3;
    auto r3 = Parse(month3, std::string_view(R"("xyz")"));
    
    return r1 && r2 && !r3;
}
static_assert(test_enum_many_values());

// ============================================================================
// Test: enum_values - Single Value
// ============================================================================

constexpr bool test_enum_single_value() {
    Annotated<
        std::array<char, 32>,
        enum_values<"only_this">
    > value;
    
    auto r1 = Parse(value, std::string_view(R"("only_this")"));
    
    Annotated<
        std::array<char, 32>,
        enum_values<"only_this">
    > value2;
    auto r2 = Parse(value2, std::string_view(R"("something_else")"));
    
    return r1 && !r2;
}
static_assert(test_enum_single_value());

// ============================================================================
// Test: enum_values - Similar Values
// ============================================================================

constexpr bool test_enum_similar_values() {
    Annotated<
        std::array<char, 32>,
        enum_values<"read", "readwrite", "readonly">
    > permission;
    
    auto r1 = Parse(permission, std::string_view(R"("read")"));
    
    Annotated<
        std::array<char, 32>,
        enum_values<"read", "readwrite", "readonly">
    > permission2;
    auto r2 = Parse(permission2, std::string_view(R"("readonly")"));
    
    Annotated<
        std::array<char, 32>,
        enum_values<"read", "readwrite", "readonly">
    > permission3;
    auto r3 = Parse(permission3, std::string_view(R"("readwrite")"));
    
    return r1 && r2 && r3;
}
static_assert(test_enum_similar_values());

// ============================================================================
// Test: enum_values - With Other Validators (in structs)
// ============================================================================

struct Config {
    Annotated<
        std::array<char, 32>,
        enum_values<"development", "staging", "production">
    > environment;
    
    Annotated<
        std::array<char, 32>,
        enum_values<"debug", "info", "warning", "error">
    > log_level;
};

constexpr bool test_enum_in_struct() {
    Config config;
    std::string_view json = R"({"environment": "production", "log_level": "error"})";
    
    auto result = Parse(config, json);
    return result;
}
static_assert(test_enum_in_struct());

constexpr bool test_enum_in_struct_invalid_value() {
    Config config;
    std::string_view json = R"({"environment": "testing", "log_level": "error"})";
    
    auto result = Parse(config, json);
    return !result;  // "testing" is not valid for environment
}
static_assert(test_enum_in_struct_invalid_value());

// ============================================================================
// Test: enum_values - In Arrays
// ============================================================================

constexpr bool test_enum_in_array() {
    std::array<
        Annotated<
            std::array<char, 16>,
            enum_values<"GET", "POST", "PUT", "DELETE">
        >,
        3
    > methods;
    
    std::string_view json = R"(["GET", "POST", "DELETE"])";
    
    auto result = Parse(methods, json);
    return result;
}
static_assert(test_enum_in_array());

constexpr bool test_enum_in_array_invalid() {
    std::array<
        Annotated<
            std::array<char, 16>,
            enum_values<"GET", "POST", "PUT", "DELETE">
        >,
        3
    > methods;
    
    std::string_view json = R"(["GET", "PATCH", "DELETE"])";
    
    auto result = Parse(methods, json);
    return !result;  // "PATCH" is not in the enum
}
static_assert(test_enum_in_array_invalid());

// ============================================================================
// Test: enum_values - With std::string (dynamic)
// ============================================================================

constexpr bool test_enum_with_string() {
    Annotated<
        std::string,
        enum_values<"active", "inactive", "pending">
    > status;
    
    std::string_view json = R"("active")";
    auto result = Parse(status, json);
    
    return result;
}
static_assert(test_enum_with_string());

// ============================================================================
// Test: enum_values - Special Characters
// ============================================================================

constexpr bool test_enum_special_chars() {
    Annotated<
        std::array<char, 32>,
        enum_values<"status-ok", "status-error", "status-pending">
    > status;
    
    auto r1 = Parse(status, std::string_view(R"("status-ok")"));
    
    Annotated<
        std::array<char, 32>,
        enum_values<"status-ok", "status-error", "status-pending">
    > status2;
    auto r2 = Parse(status2, std::string_view(R"("status-unknown")"));
    
    return r1 && !r2;
}
static_assert(test_enum_special_chars());

// ============================================================================
// All Tests Summary
// ============================================================================

constexpr bool all_enum_tests_pass() {
    return 
        test_enum_valid_single() &&
        test_enum_valid_all_values() &&
        test_enum_invalid() &&
        test_enum_empty_string() &&
        test_enum_early_rejection() &&
        test_enum_prefix_not_match() &&
        test_enum_partial_match() &&
        test_enum_case_sensitive() &&
        test_enum_many_values() &&
        test_enum_single_value() &&
        test_enum_similar_values() &&
        test_enum_in_struct() &&
        test_enum_in_struct_invalid_value() &&
        test_enum_in_array() &&
        test_enum_in_array_invalid() &&
        test_enum_with_string() &&
        test_enum_special_chars();
}

static_assert(all_enum_tests_pass(), 
    "[[[ All enum_values tests must pass at compile time ]]]");

int main() {
    // All tests verified at compile time
    return 0;
}

