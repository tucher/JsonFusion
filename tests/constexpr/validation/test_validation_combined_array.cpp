#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/validators.hpp>
#include <array>
#include <vector>

using namespace JsonFusion;
using namespace JsonFusion::validators;

// ============================================================================
// Test: Combined Array Validators - min_items + max_items
// ============================================================================

// Test: min_items<3> + max_items<5> - all constraints pass
constexpr bool test_combined_items_valid() {
    struct Test {
        Annotated<std::array<int, 10>, min_items<3>, max_items<5>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": [1, 2, 3, 4]})";  // 4 items, within range
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_combined_items_valid(), "min_items<3> + max_items<5> - all constraints pass");

// Test: min_items<3> + max_items<5> - fails min_items
constexpr bool test_combined_items_fails_min() {
    struct Test {
        Annotated<std::array<int, 10>, min_items<3>, max_items<5>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": [1, 2]})";  // 2 items < 3
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::array_items_count_out_of_range;
}
static_assert(test_combined_items_fails_min(), "min_items<3> + max_items<5> - fails min_items");

// Test: min_items<3> + max_items<5> - fails max_items
constexpr bool test_combined_items_fails_max() {
    struct Test {
        Annotated<std::array<int, 10>, min_items<3>, max_items<5>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": [1, 2, 3, 4, 5, 6]})";  // 6 items > 5
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::array_items_count_out_of_range;
}
static_assert(test_combined_items_fails_max(), "min_items<3> + max_items<5> - fails max_items");

// ============================================================================
// Test: Combined Array Validators - min_items + max_items + Element Validators
// ============================================================================

// Test: min_items<2> + max_items<4> with range on elements - all constraints pass
constexpr bool test_combined_items_element_range_valid() {
    struct Test {
        Annotated<std::array<int, 10>, min_items<2>, max_items<4>> value;
    };
    
    struct Element {
        Annotated<int, range<0, 100>> item;
    };
    
    struct TestWithElements {
        Annotated<std::array<Element, 10>, min_items<2>, max_items<4>> value;
    };
    
    TestWithElements obj{};
    std::string json = R"({"value": [{"item": 10}, {"item": 20}, {"item": 30}]})";  // 3 items, all elements valid
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_combined_items_element_range_valid(), "min_items<2> + max_items<4> with range on elements - all pass");

// Test: min_items<2> + max_items<4> with range on elements - element fails range
constexpr bool test_combined_items_element_range_fails() {
    struct Element {
        Annotated<int, range<0, 100>> item;
    };
    
    struct TestWithElements {
        Annotated<std::array<Element, 10>, min_items<2>, max_items<4>> value;
    };
    
    TestWithElements obj{};
    std::string json = R"({"value": [{"item": 10}, {"item": 150}]})";  // 2 items (valid count), but element out of range
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_combined_items_element_range_fails(), "min_items<2> + max_items<4> with range on elements - element fails");

// ============================================================================
// Test: Combined Array Validators - Nested Validation
// ============================================================================

// Test: Array of structs with validators - all constraints pass
constexpr bool test_combined_array_nested_valid() {
    struct Inner {
        Annotated<int, range<0, 100>> value;
    };
    
    struct Test {
        Annotated<std::array<Inner, 10>, min_items<2>, max_items<5>> items;
    };
    
    Test obj{};
    std::string json = R"({"items": [{"value": 10}, {"value": 20}, {"value": 30}]})";
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_combined_array_nested_valid(), "Array of structs with validators - all constraints pass");

// Test: Array of structs with validators - inner validator fails
constexpr bool test_combined_array_nested_inner_fails() {
    struct Inner {
        Annotated<int, range<0, 100>> value;
    };
    
    struct Test {
        Annotated<std::array<Inner, 10>, min_items<2>, max_items<5>> items;
    };
    
    Test obj{};
    std::string json = R"({"items": [{"value": 10}, {"value": 200}]})";  // Valid count, but inner value out of range
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_combined_array_nested_inner_fails(), "Array of structs - inner validator fails");

// Test: Array of structs with validators - array count fails
constexpr bool test_combined_array_nested_count_fails() {
    struct Inner {
        Annotated<int, range<0, 100>> value;
    };
    
    struct Test {
        Annotated<std::array<Inner, 10>, min_items<2>, max_items<5>> items;
    };
    
    Test obj{};
    std::string json = R"({"items": [{"value": 10}]})";  // Only 1 item < 2
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::array_items_count_out_of_range;
}
static_assert(test_combined_array_nested_count_fails(), "Array of structs - array count fails");

// ============================================================================
// Test: Combined Array Validators - Multiple Array Fields
// ============================================================================

constexpr bool test_combined_array_multiple_fields() {
    struct Test {
        Annotated<std::array<int, 10>, min_items<2>, max_items<5>> array1;
        Annotated<std::vector<int>, min_items<1>, max_items<3>> array2;
    };
    
    Test obj{};
    std::string json = R"({"array1": [1, 2, 3], "array2": [10, 20]})";
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_combined_array_multiple_fields(), "Multiple array fields with combined validators");

constexpr bool test_combined_array_multiple_fields_one_fails() {
    struct Test {
        Annotated<std::array<int, 10>, min_items<2>, max_items<5>> array1;
        Annotated<std::vector<int>, min_items<1>, max_items<3>> array2;
    };
    
    Test obj{};
    std::string json = R"({"array1": [1, 2, 3], "array2": [10, 20, 30, 40]})";  // array2 has 4 items > 3
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::array_items_count_out_of_range;
}
static_assert(test_combined_array_multiple_fields_one_fails(), "Multiple array fields - one fails");

