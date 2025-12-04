#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/validators.hpp>
#include <array>
#include <vector>

using namespace JsonFusion;
using namespace JsonFusion::validators;

// ============================================================================
// Test: min_items<> - Valid Values
// ============================================================================

// Test: min_items<3> accepts array with exactly 3 items
constexpr bool test_min_items_exact_valid() {
    struct Test {
        Annotated<std::array<int, 10>, min_items<3>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": [1, 2, 3]})";
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_min_items_exact_valid(), "min_items<3> accepts array with exactly 3 items");

// Test: min_items<3> accepts array with more than 3 items
constexpr bool test_min_items_more_valid() {
    struct Test {
        Annotated<std::array<int, 10>, min_items<3>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": [1, 2, 3, 4, 5]})";
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_min_items_more_valid(), "min_items<3> accepts array with more than 3 items");

// Test: min_items<1> accepts array with single item
constexpr bool test_min_items_one_valid() {
    struct Test {
        Annotated<std::array<int, 10>, min_items<1>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": [42]})";
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_min_items_one_valid(), "min_items<1> accepts array with single item");

// ============================================================================
// Test: min_items<> - Invalid Values (Too Few Items)
// ============================================================================

// Test: min_items<3> rejects array with fewer than 3 items
constexpr bool test_min_items_too_few() {
    struct Test {
        Annotated<std::array<int, 10>, min_items<3>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": [1, 2]})";  // 2 items < 3
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::array_items_count_out_of_range;
}
static_assert(test_min_items_too_few(), "min_items<3> rejects array with fewer than 3 items");

// Test: min_items<3> rejects empty array
constexpr bool test_min_items_empty_rejected() {
    struct Test {
        Annotated<std::array<int, 10>, min_items<3>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": []})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::array_items_count_out_of_range;
}
static_assert(test_min_items_empty_rejected(), "min_items<3> rejects empty array");

// Test: min_items<1> rejects empty array
constexpr bool test_min_items_one_rejects_empty() {
    struct Test {
        Annotated<std::array<int, 10>, min_items<1>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": []})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::array_items_count_out_of_range;
}
static_assert(test_min_items_one_rejects_empty(), "min_items<1> rejects empty array");

// ============================================================================
// Test: max_items<> - Valid Values
// ============================================================================

// Test: max_items<5> accepts array with exactly 5 items
constexpr bool test_max_items_exact_valid() {
    struct Test {
        Annotated<std::array<int, 10>, max_items<5>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": [1, 2, 3, 4, 5]})";
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_max_items_exact_valid(), "max_items<5> accepts array with exactly 5 items");

// Test: max_items<5> accepts array with fewer than 5 items
constexpr bool test_max_items_fewer_valid() {
    struct Test {
        Annotated<std::array<int, 10>, max_items<5>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": [1, 2, 3]})";
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_max_items_fewer_valid(), "max_items<5> accepts array with fewer than 5 items");

// Test: max_items<5> accepts empty array
constexpr bool test_max_items_empty_valid() {
    struct Test {
        Annotated<std::array<int, 10>, max_items<5>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": []})";
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_max_items_empty_valid(), "max_items<5> accepts empty array");

// ============================================================================
// Test: max_items<> - Invalid Values (Too Many Items)
// ============================================================================

// Test: max_items<5> rejects array with more than 5 items
constexpr bool test_max_items_too_many() {
    struct Test {
        Annotated<std::array<int, 10>, max_items<5>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": [1, 2, 3, 4, 5, 6]})";  // 6 items > 5
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::array_items_count_out_of_range;
}
static_assert(test_max_items_too_many(), "max_items<5> rejects array with more than 5 items");

// Test: max_items<1> rejects array with 2 items
constexpr bool test_max_items_one_rejects_two() {
    struct Test {
        Annotated<std::array<int, 10>, max_items<1>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": [1, 2]})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::array_items_count_out_of_range;
}
static_assert(test_max_items_one_rejects_two(), "max_items<1> rejects array with 2 items");

// ============================================================================
// Test: min_items<> + max_items<> - Combined Constraints
// ============================================================================

// Test: min_items<3>, max_items<5> accepts array at min boundary
constexpr bool test_items_range_min_valid() {
    struct Test {
        Annotated<std::array<int, 10>, min_items<3>, max_items<5>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": [1, 2, 3]})";  // 3 items
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_items_range_min_valid(), "min_items<3>, max_items<5> accepts array at min boundary");

// Test: min_items<3>, max_items<5> accepts array at max boundary
constexpr bool test_items_range_max_valid() {
    struct Test {
        Annotated<std::array<int, 10>, min_items<3>, max_items<5>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": [1, 2, 3, 4, 5]})";  // 5 items
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_items_range_max_valid(), "min_items<3>, max_items<5> accepts array at max boundary");

// Test: min_items<3>, max_items<5> accepts array in middle
constexpr bool test_items_range_middle_valid() {
    struct Test {
        Annotated<std::array<int, 10>, min_items<3>, max_items<5>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": [1, 2, 3, 4]})";  // 4 items
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_items_range_middle_valid(), "min_items<3>, max_items<5> accepts array in middle");

// Test: min_items<3>, max_items<5> rejects array too few items
constexpr bool test_items_range_too_few() {
    struct Test {
        Annotated<std::array<int, 10>, min_items<3>, max_items<5>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": [1, 2]})";  // 2 items < 3
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::array_items_count_out_of_range;
}
static_assert(test_items_range_too_few(), "min_items<3>, max_items<5> rejects array too few items");

// Test: min_items<3>, max_items<5> rejects array too many items
constexpr bool test_items_range_too_many() {
    struct Test {
        Annotated<std::array<int, 10>, min_items<3>, max_items<5>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": [1, 2, 3, 4, 5, 6]})";  // 6 items > 5
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::array_items_count_out_of_range;
}
static_assert(test_items_range_too_many(), "min_items<3>, max_items<5> rejects array too many items");

// Test: min_items<3>, max_items<5> rejects empty array
constexpr bool test_items_range_empty_rejected() {
    struct Test {
        Annotated<std::array<int, 10>, min_items<3>, max_items<5>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": []})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::array_items_count_out_of_range;
}
static_assert(test_items_range_empty_rejected(), "min_items<3>, max_items<5> rejects empty array");

// ============================================================================
// Test: Single item constraint
// ============================================================================

// Test: min_items<1>, max_items<1> accepts array with exactly one item
constexpr bool test_items_exactly_one() {
    struct Test {
        Annotated<std::array<int, 10>, min_items<1>, max_items<1>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": [42]})";
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_items_exactly_one(), "min_items<1>, max_items<1> accepts array with exactly one item");

// Test: min_items<1>, max_items<1> rejects empty array
constexpr bool test_items_exactly_one_rejects_empty() {
    struct Test {
        Annotated<std::array<int, 10>, min_items<1>, max_items<1>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": []})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::array_items_count_out_of_range;
}
static_assert(test_items_exactly_one_rejects_empty(), "min_items<1>, max_items<1> rejects empty array");

// Test: min_items<1>, max_items<1> rejects array with two items
constexpr bool test_items_exactly_one_rejects_two() {
    struct Test {
        Annotated<std::array<int, 10>, min_items<1>, max_items<1>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": [1, 2]})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::array_items_count_out_of_range;
}
static_assert(test_items_exactly_one_rejects_two(), "min_items<1>, max_items<1> rejects array with two items");

// ============================================================================
// Test: With std::vector (dynamic arrays)
// ============================================================================

// Test: min_items<2>, max_items<4> works with std::vector
constexpr bool test_items_vector_valid() {
    struct Test {
        Annotated<std::vector<int>, min_items<2>, max_items<4>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": [1, 2, 3]})";
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_items_vector_valid(), "min_items<2>, max_items<4> works with std::vector");

// Test: min_items<2> rejects std::vector with too few items
constexpr bool test_items_vector_too_few() {
    struct Test {
        Annotated<std::vector<int>, min_items<2>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": [1]})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::array_items_count_out_of_range;
}
static_assert(test_items_vector_too_few(), "min_items<2> rejects std::vector with too few items");

// ============================================================================
// Test: Multiple item-constrained fields in same struct
// ============================================================================

constexpr bool test_multiple_items_constraints() {
    struct Test {
        Annotated<std::array<int, 10>, min_items<2>> small_array;
        Annotated<std::array<int, 10>, max_items<5>> large_array;
        Annotated<std::array<int, 10>, min_items<3>, max_items<7>> range_array;
    };
    
    Test obj{};
    std::string json = R"({"small_array": [1, 2], "large_array": [1, 2, 3], "range_array": [1, 2, 3, 4]})";
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_multiple_items_constraints(), "Multiple item-constrained fields in same struct");

constexpr bool test_multiple_items_constraints_one_fails() {
    struct Test {
        Annotated<std::array<int, 10>, min_items<2>> small_array;
        Annotated<std::array<int, 10>, max_items<3>> large_array;
    };
    
    Test obj{};
    std::string json = R"({"small_array": [1, 2], "large_array": [1, 2, 3, 4]})";  // 4 items > 3
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::array_items_count_out_of_range;
}
static_assert(test_multiple_items_constraints_one_fails(), "Multiple items constraints - one fails");

