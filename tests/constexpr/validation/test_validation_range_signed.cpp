#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/validators.hpp>
#include <cstdint>
#include <climits>

using namespace JsonFusion;
using namespace JsonFusion::validators;

// ============================================================================
// Test: range<> - int8_t
// ============================================================================

// Test: range<INT8_MIN, INT8_MAX> accepts min boundary
constexpr bool test_range_int8_min() {
    struct Test {
        Annotated<int8_t, range<INT8_MIN, INT8_MAX>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": -128})";  // INT8_MIN
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == INT8_MIN;
}
static_assert(test_range_int8_min(), "range<INT8_MIN, INT8_MAX> accepts INT8_MIN");

// Test: range<INT8_MIN, INT8_MAX> accepts max boundary
constexpr bool test_range_int8_max() {
    struct Test {
        Annotated<int8_t, range<INT8_MIN, INT8_MAX>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 127})";  // INT8_MAX
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == INT8_MAX;
}
static_assert(test_range_int8_max(), "range<INT8_MIN, INT8_MAX> accepts INT8_MAX");

// Test: range<-10, 10> with int8_t accepts valid values
constexpr bool test_range_int8_valid() {
    struct Test {
        Annotated<int8_t, range<-10, 10>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 0})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == 0;
}
static_assert(test_range_int8_valid(), "range<-10, 10> with int8_t accepts valid values");

// Test: range<-10, 10> with int8_t rejects below min
constexpr bool test_range_int8_below_min() {
    struct Test {
        Annotated<int8_t, range<-10, 10>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": -11})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_int8_below_min(), "range<-10, 10> with int8_t rejects below min");

// Test: range<-10, 10> with int8_t rejects above max
constexpr bool test_range_int8_above_max() {
    struct Test {
        Annotated<int8_t, range<-10, 10>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 11})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_int8_above_max(), "range<-10, 10> with int8_t rejects above max");

// ============================================================================
// Test: range<> - int16_t
// ============================================================================

// Test: range<INT16_MIN, INT16_MAX> accepts min boundary
constexpr bool test_range_int16_min() {
    struct Test {
        Annotated<int16_t, range<INT16_MIN, INT16_MAX>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": -32768})";  // INT16_MIN
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == INT16_MIN;
}
static_assert(test_range_int16_min(), "range<INT16_MIN, INT16_MAX> accepts INT16_MIN");

// Test: range<INT16_MIN, INT16_MAX> accepts max boundary
constexpr bool test_range_int16_max() {
    struct Test {
        Annotated<int16_t, range<INT16_MIN, INT16_MAX>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 32767})";  // INT16_MAX
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == INT16_MAX;
}
static_assert(test_range_int16_max(), "range<INT16_MIN, INT16_MAX> accepts INT16_MAX");

// Test: range<-1000, 1000> with int16_t accepts valid values
constexpr bool test_range_int16_valid() {
    struct Test {
        Annotated<int16_t, range<-1000, 1000>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 500})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == 500;
}
static_assert(test_range_int16_valid(), "range<-1000, 1000> with int16_t accepts valid values");

// Test: range<-1000, 1000> with int16_t rejects below min
constexpr bool test_range_int16_below_min() {
    struct Test {
        Annotated<int16_t, range<-1000, 1000>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": -1001})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_int16_below_min(), "range<-1000, 1000> with int16_t rejects below min");

// Test: range<-1000, 1000> with int16_t rejects above max
constexpr bool test_range_int16_above_max() {
    struct Test {
        Annotated<int16_t, range<-1000, 1000>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 1001})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_int16_above_max(), "range<-1000, 1000> with int16_t rejects above max");

// ============================================================================
// Test: range<> - int32_t
// ============================================================================

// Test: range<INT32_MIN, INT32_MAX> accepts min boundary
constexpr bool test_range_int32_min() {
    struct Test {
        Annotated<int32_t, range<INT32_MIN, INT32_MAX>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": -2147483648})";  // INT32_MIN
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == INT32_MIN;
}
static_assert(test_range_int32_min(), "range<INT32_MIN, INT32_MAX> accepts INT32_MIN");

// Test: range<INT32_MIN, INT32_MAX> accepts max boundary
constexpr bool test_range_int32_max() {
    struct Test {
        Annotated<int32_t, range<INT32_MIN, INT32_MAX>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 2147483647})";  // INT32_MAX
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == INT32_MAX;
}
static_assert(test_range_int32_max(), "range<INT32_MIN, INT32_MAX> accepts INT32_MAX");

// Test: range<-100000, 100000> with int32_t accepts valid values
constexpr bool test_range_int32_valid() {
    struct Test {
        Annotated<int32_t, range<-100000, 100000>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 50000})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == 50000;
}
static_assert(test_range_int32_valid(), "range<-100000, 100000> with int32_t accepts valid values");

// Test: range<-100000, 100000> with int32_t rejects below min
constexpr bool test_range_int32_below_min() {
    struct Test {
        Annotated<int32_t, range<-100000, 100000>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": -100001})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_int32_below_min(), "range<-100000, 100000> with int32_t rejects below min");

// Test: range<-100000, 100000> with int32_t rejects above max
constexpr bool test_range_int32_above_max() {
    struct Test {
        Annotated<int32_t, range<-100000, 100000>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 100001})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_int32_above_max(), "range<-100000, 100000> with int32_t rejects above max");

// ============================================================================
// Test: range<> - int64_t
// ============================================================================

// Test: range<INT64_MIN, INT64_MAX> accepts min boundary
constexpr bool test_range_int64_min() {
    struct Test {
        Annotated<int64_t, range<INT64_MIN, INT64_MAX>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": -9223372036854775808})";  // INT64_MIN
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == INT64_MIN;
}
static_assert(test_range_int64_min(), "range<INT64_MIN, INT64_MAX> accepts INT64_MIN");

// Test: range<INT64_MIN, INT64_MAX> accepts max boundary
constexpr bool test_range_int64_max() {
    struct Test {
        Annotated<int64_t, range<INT64_MIN, INT64_MAX>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 9223372036854775807})";  // INT64_MAX
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == INT64_MAX;
}
static_assert(test_range_int64_max(), "range<INT64_MIN, INT64_MAX> accepts INT64_MAX");

// Test: range<-1000000, 1000000> with int64_t accepts valid values
constexpr bool test_range_int64_valid() {
    struct Test {
        Annotated<int64_t, range<-1000000, 1000000>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 500000})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == 500000;
}
static_assert(test_range_int64_valid(), "range<-1000000, 1000000> with int64_t accepts valid values");

// Test: range<-1000000, 1000000> with int64_t rejects below min
constexpr bool test_range_int64_below_min() {
    struct Test {
        Annotated<int64_t, range<-1000000, 1000000>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": -1000001})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_int64_below_min(), "range<-1000000, 1000000> with int64_t rejects below min");

// Test: range<-1000000, 1000000> with int64_t rejects above max
constexpr bool test_range_int64_above_max() {
    struct Test {
        Annotated<int64_t, range<-1000000, 1000000>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 1000001})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_int64_above_max(), "range<-1000000, 1000000> with int64_t rejects above max");

// ============================================================================
// Test: range<> - Type-Specific Boundaries
// ============================================================================

// Test: range<-128, 127> with int8_t (full type range)
constexpr bool test_range_int8_full_range() {
    struct Test {
        Annotated<int8_t, range<-128, 127>> value;
    };
    
    Test obj1{};
    std::string json1 = R"({"value": -128})";
    auto result1 = Parse(obj1, json1);
    
    Test obj2{};
    std::string json2 = R"({"value": 127})";
    auto result2 = Parse(obj2, json2);
    
    return result1 && obj1.value.get() == -128
        && result2 && obj2.value.get() == 127;
}
static_assert(test_range_int8_full_range(), "range<-128, 127> with int8_t (full type range)");

// Test: range<-32768, 32767> with int16_t (full type range)
constexpr bool test_range_int16_full_range() {
    struct Test {
        Annotated<int16_t, range<-32768, 32767>> value;
    };
    
    Test obj1{};
    std::string json1 = R"({"value": -32768})";
    auto result1 = Parse(obj1, json1);
    
    Test obj2{};
    std::string json2 = R"({"value": 32767})";
    auto result2 = Parse(obj2, json2);
    
    return result1 && obj1.value.get() == -32768
        && result2 && obj2.value.get() == 32767;
}
static_assert(test_range_int16_full_range(), "range<-32768, 32767> with int16_t (full type range)");

// ============================================================================
// Test: range<> - Multiple Signed Types in Same Struct
// ============================================================================

constexpr bool test_range_multiple_signed_types() {
    struct Test {
        Annotated<int8_t, range<-10, 10>> value8;
        Annotated<int16_t, range<-100, 100>> value16;
        Annotated<int32_t, range<-1000, 1000>> value32;
        Annotated<int64_t, range<-10000, 10000>> value64;
    };
    
    Test obj{};
    std::string json = R"({"value8": 5, "value16": 50, "value32": 500, "value64": 5000})";
    auto result = Parse(obj, json);
    
    return result 
        && obj.value8.get() == 5
        && obj.value16.get() == 50
        && obj.value32.get() == 500
        && obj.value64.get() == 5000;
}
static_assert(test_range_multiple_signed_types(), "Multiple signed types with range in same struct");

constexpr bool test_range_multiple_signed_types_one_fails() {
    struct Test {
        Annotated<int8_t, range<-10, 10>> value8;
        Annotated<int16_t, range<-100, 100>> value16;
    };
    
    Test obj{};
    std::string json = R"({"value8": 5, "value16": 101})";  // value16 out of range
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_multiple_signed_types_one_fails(), "Multiple signed types - one fails");

