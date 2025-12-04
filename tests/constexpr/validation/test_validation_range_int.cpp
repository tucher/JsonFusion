#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/validators.hpp>

using namespace JsonFusion;
using namespace JsonFusion::validators;

// ============================================================================
// Test: range<> - Valid Values at Boundaries
// ============================================================================

// Test: range<0, 100> accepts value at min boundary
constexpr bool test_range_min_boundary_valid() {
    struct Test {
        Annotated<int, range<0, 100>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 0})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == 0;
}
static_assert(test_range_min_boundary_valid(), "range<0, 100> accepts min boundary (0)");

// Test: range<0, 100> accepts value at max boundary
constexpr bool test_range_max_boundary_valid() {
    struct Test {
        Annotated<int, range<0, 100>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 100})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == 100;
}
static_assert(test_range_max_boundary_valid(), "range<0, 100> accepts max boundary (100)");

// Test: range<0, 100> accepts value in middle
constexpr bool test_range_middle_valid() {
    struct Test {
        Annotated<int, range<0, 100>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 50})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == 50;
}
static_assert(test_range_middle_valid(), "range<0, 100> accepts middle value (50)");

// ============================================================================
// Test: range<> - Invalid Values (Below Min, Above Max)
// ============================================================================

// Test: range<0, 100> rejects value below min
constexpr bool test_range_below_min() {
    struct Test {
        Annotated<int, range<0, 100>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": -1})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_below_min(), "range<0, 100> rejects value below min (-1)");

// Test: range<0, 100> rejects value above max
constexpr bool test_range_above_max() {
    struct Test {
        Annotated<int, range<0, 100>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 101})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_above_max(), "range<0, 100> rejects value above max (101)");

// ============================================================================
// Test: range<> - Negative Ranges
// ============================================================================

// Test: range<-100, -10> accepts value at min boundary
constexpr bool test_range_negative_min_valid() {
    struct Test {
        Annotated<int, range<-100, -10>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": -100})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == -100;
}
static_assert(test_range_negative_min_valid(), "range<-100, -10> accepts min boundary (-100)");

// Test: range<-100, -10> accepts value at max boundary
constexpr bool test_range_negative_max_valid() {
    struct Test {
        Annotated<int, range<-100, -10>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": -10})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == -10;
}
static_assert(test_range_negative_max_valid(), "range<-100, -10> accepts max boundary (-10)");

// Test: range<-100, -10> accepts value in middle
constexpr bool test_range_negative_middle_valid() {
    struct Test {
        Annotated<int, range<-100, -10>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": -50})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == -50;
}
static_assert(test_range_negative_middle_valid(), "range<-100, -10> accepts middle value (-50)");

// Test: range<-100, -10> rejects value below min
constexpr bool test_range_negative_below_min() {
    struct Test {
        Annotated<int, range<-100, -10>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": -101})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_negative_below_min(), "range<-100, -10> rejects value below min (-101)");

// Test: range<-100, -10> rejects value above max
constexpr bool test_range_negative_above_max() {
    struct Test {
        Annotated<int, range<-100, -10>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": -9})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_negative_above_max(), "range<-100, -10> rejects value above max (-9)");

// Test: range<-100, -10> rejects positive value
constexpr bool test_range_negative_rejects_positive() {
    struct Test {
        Annotated<int, range<-100, -10>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 0})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_negative_rejects_positive(), "range<-100, -10> rejects positive value (0)");

// ============================================================================
// Test: range<> - Single Value Range
// ============================================================================

// Test: range<42, 42> accepts exactly 42
constexpr bool test_range_single_value_valid() {
    struct Test {
        Annotated<int, range<42, 42>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 42})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == 42;
}
static_assert(test_range_single_value_valid(), "range<42, 42> accepts exactly 42");

// Test: range<42, 42> rejects 41
constexpr bool test_range_single_value_below() {
    struct Test {
        Annotated<int, range<42, 42>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 41})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_single_value_below(), "range<42, 42> rejects 41");

// Test: range<42, 42> rejects 43
constexpr bool test_range_single_value_above() {
    struct Test {
        Annotated<int, range<42, 42>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 43})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_single_value_above(), "range<42, 42> rejects 43");

// ============================================================================
// Test: range<> - Zero Boundaries
// ============================================================================

// Test: range<0, 0> accepts zero
constexpr bool test_range_zero_valid() {
    struct Test {
        Annotated<int, range<0, 0>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 0})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == 0;
}
static_assert(test_range_zero_valid(), "range<0, 0> accepts zero");

// Test: range<0, 0> rejects positive
constexpr bool test_range_zero_rejects_positive() {
    struct Test {
        Annotated<int, range<0, 0>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 1})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_zero_rejects_positive(), "range<0, 0> rejects positive (1)");

// Test: range<0, 0> rejects negative
constexpr bool test_range_zero_rejects_negative() {
    struct Test {
        Annotated<int, range<0, 0>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": -1})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_zero_rejects_negative(), "range<0, 0> rejects negative (-1)");

// ============================================================================
// Test: range<> - Large Ranges
// ============================================================================

// Test: range<-1000, 1000> accepts values at boundaries
constexpr bool test_range_large_boundaries() {
    struct Test {
        Annotated<int, range<-1000, 1000>> value;
    };
    
    Test obj1{};
    std::string json1 = R"({"value": -1000})";
    auto result1 = Parse(obj1, json1);
    
    Test obj2{};
    std::string json2 = R"({"value": 1000})";
    auto result2 = Parse(obj2, json2);
    
    return result1 && obj1.value.get() == -1000
        && result2 && obj2.value.get() == 1000;
}
static_assert(test_range_large_boundaries(), "range<-1000, 1000> accepts boundary values");

// Test: range<-1000, 1000> rejects values outside
constexpr bool test_range_large_outside() {
    struct Test {
        Annotated<int, range<-1000, 1000>> value;
    };
    
    Test obj1{};
    std::string json1 = R"({"value": -1001})";
    auto result1 = Parse(obj1, json1);
    
    Test obj2{};
    std::string json2 = R"({"value": 1001})";
    auto result2 = Parse(obj2, json2);
    
    return !result1 && result1.validationErrors().error() == SchemaError::number_out_of_range
        && !result2 && result2.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_large_outside(), "range<-1000, 1000> rejects values outside range");

// ============================================================================
// Test: Multiple range fields in same struct
// ============================================================================

constexpr bool test_multiple_ranges() {
    struct Test {
        Annotated<int, range<0, 10>> small;
        Annotated<int, range<100, 200>> medium;
        Annotated<int, range<-50, 50>> centered;
    };
    
    Test obj{};
    std::string json = R"({"small": 5, "medium": 150, "centered": 0})";
    auto result = Parse(obj, json);
    
    return result 
        && obj.small.get() == 5
        && obj.medium.get() == 150
        && obj.centered.get() == 0;
}
static_assert(test_multiple_ranges(), "Multiple range fields in same struct");

constexpr bool test_multiple_ranges_one_fails() {
    struct Test {
        Annotated<int, range<0, 10>> small;
        Annotated<int, range<100, 200>> medium;
    };
    
    Test obj{};
    std::string json = R"({"small": 5, "medium": 250})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_multiple_ranges_one_fails(), "Multiple ranges - one fails");

