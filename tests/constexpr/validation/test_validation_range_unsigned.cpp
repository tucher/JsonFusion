#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/validators.hpp>
#include <cstdint>
#include <climits>

using namespace JsonFusion;
using namespace JsonFusion::validators;

// ============================================================================
// Test: range<> - uint8_t
// ============================================================================

// Test: range<0, UINT8_MAX> accepts zero boundary
constexpr bool test_range_uint8_zero() {
    struct Test {
        Annotated<uint8_t, range<0, UINT8_MAX>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 0})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == 0;
}
static_assert(test_range_uint8_zero(), "range<0, UINT8_MAX> accepts zero");

// Test: range<0, UINT8_MAX> accepts max boundary
constexpr bool test_range_uint8_max() {
    struct Test {
        Annotated<uint8_t, range<0, UINT8_MAX>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 255})";  // UINT8_MAX
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == UINT8_MAX;
}
static_assert(test_range_uint8_max(), "range<0, UINT8_MAX> accepts UINT8_MAX");

// Test: range<10, 100> with uint8_t accepts valid values
constexpr bool test_range_uint8_valid() {
    struct Test {
        Annotated<uint8_t, range<10, 100>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 50})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == 50;
}
static_assert(test_range_uint8_valid(), "range<10, 100> with uint8_t accepts valid values");

// Test: range<10, 100> with uint8_t rejects below min
constexpr bool test_range_uint8_below_min() {
    struct Test {
        Annotated<uint8_t, range<10, 100>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 9})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_uint8_below_min(), "range<10, 100> with uint8_t rejects below min");

// Test: range<10, 100> with uint8_t rejects above max
constexpr bool test_range_uint8_above_max() {
    struct Test {
        Annotated<uint8_t, range<10, 100>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 101})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_uint8_above_max(), "range<10, 100> with uint8_t rejects above max");

// ============================================================================
// Test: range<> - uint16_t
// ============================================================================

// Test: range<0, UINT16_MAX> accepts zero boundary
constexpr bool test_range_uint16_zero() {
    struct Test {
        Annotated<uint16_t, range<0, UINT16_MAX>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 0})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == 0;
}
static_assert(test_range_uint16_zero(), "range<0, UINT16_MAX> accepts zero");

// Test: range<0, UINT16_MAX> accepts max boundary
constexpr bool test_range_uint16_max() {
    struct Test {
        Annotated<uint16_t, range<0, UINT16_MAX>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 65535})";  // UINT16_MAX
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == UINT16_MAX;
}
static_assert(test_range_uint16_max(), "range<0, UINT16_MAX> accepts UINT16_MAX");

// Test: range<100, 1000> with uint16_t accepts valid values
constexpr bool test_range_uint16_valid() {
    struct Test {
        Annotated<uint16_t, range<100, 1000>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 500})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == 500;
}
static_assert(test_range_uint16_valid(), "range<100, 1000> with uint16_t accepts valid values");

// Test: range<100, 1000> with uint16_t rejects below min
constexpr bool test_range_uint16_below_min() {
    struct Test {
        Annotated<uint16_t, range<100, 1000>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 99})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_uint16_below_min(), "range<100, 1000> with uint16_t rejects below min");

// Test: range<100, 1000> with uint16_t rejects above max
constexpr bool test_range_uint16_above_max() {
    struct Test {
        Annotated<uint16_t, range<100, 1000>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 1001})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_uint16_above_max(), "range<100, 1000> with uint16_t rejects above max");

// ============================================================================
// Test: range<> - uint32_t
// ============================================================================

// Test: range<0, UINT32_MAX> accepts zero boundary
constexpr bool test_range_uint32_zero() {
    struct Test {
        Annotated<uint32_t, range<0, UINT32_MAX>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 0})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == 0;
}
static_assert(test_range_uint32_zero(), "range<0, UINT32_MAX> accepts zero");

// Test: range<0, UINT32_MAX> accepts max boundary
constexpr bool test_range_uint32_max() {
    struct Test {
        Annotated<uint32_t, range<0, UINT32_MAX>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 4294967295})";  // UINT32_MAX
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == UINT32_MAX;
}
static_assert(test_range_uint32_max(), "range<0, UINT32_MAX> accepts UINT32_MAX");

// Test: range<1000, 100000> with uint32_t accepts valid values
constexpr bool test_range_uint32_valid() {
    struct Test {
        Annotated<uint32_t, range<1000, 100000>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 50000})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == 50000;
}
static_assert(test_range_uint32_valid(), "range<1000, 100000> with uint32_t accepts valid values");

// Test: range<1000, 100000> with uint32_t rejects below min
constexpr bool test_range_uint32_below_min() {
    struct Test {
        Annotated<uint32_t, range<1000, 100000>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 999})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_uint32_below_min(), "range<1000, 100000> with uint32_t rejects below min");

// Test: range<1000, 100000> with uint32_t rejects above max
constexpr bool test_range_uint32_above_max() {
    struct Test {
        Annotated<uint32_t, range<1000, 100000>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 100001})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_uint32_above_max(), "range<1000, 100000> with uint32_t rejects above max");

// ============================================================================
// Test: range<> - uint64_t
// ============================================================================

// Test: range<0, UINT64_MAX> accepts zero boundary
constexpr bool test_range_uint64_zero() {
    struct Test {
        Annotated<uint64_t, range<0, UINT64_MAX>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 0})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == 0;
}
static_assert(test_range_uint64_zero(), "range<0, UINT64_MAX> accepts zero");

// Test: range<0, UINT64_MAX> accepts max boundary
constexpr bool test_range_uint64_max() {
    struct Test {
        Annotated<uint64_t, range<0, UINT64_MAX>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 18446744073709551615})";  // UINT64_MAX
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == UINT64_MAX;
}
static_assert(test_range_uint64_max(), "range<0, UINT64_MAX> accepts UINT64_MAX");

// Test: range<10000, 1000000> with uint64_t accepts valid values
constexpr bool test_range_uint64_valid() {
    struct Test {
        Annotated<uint64_t, range<10000, 1000000>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 500000})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == 500000;
}
static_assert(test_range_uint64_valid(), "range<10000, 1000000> with uint64_t accepts valid values");

// Test: range<10000, 1000000> with uint64_t rejects below min
constexpr bool test_range_uint64_below_min() {
    struct Test {
        Annotated<uint64_t, range<10000, 1000000>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 9999})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_uint64_below_min(), "range<10000, 1000000> with uint64_t rejects below min");

// Test: range<10000, 1000000> with uint64_t rejects above max
constexpr bool test_range_uint64_above_max() {
    struct Test {
        Annotated<uint64_t, range<10000, 1000000>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 1000001})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_uint64_above_max(), "range<10000, 1000000> with uint64_t rejects above max");

// ============================================================================
// Test: range<> - Zero Boundaries for Unsigned Types
// ============================================================================

// Test: range<0, 0> with uint8_t accepts only zero
constexpr bool test_range_uint8_zero_only() {
    struct Test {
        Annotated<uint8_t, range<0, 0>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 0})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == 0;
}
static_assert(test_range_uint8_zero_only(), "range<0, 0> with uint8_t accepts only zero");

// Test: range<0, 0> with uint8_t rejects positive
constexpr bool test_range_uint8_zero_rejects_positive() {
    struct Test {
        Annotated<uint8_t, range<0, 0>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 1})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_uint8_zero_rejects_positive(), "range<0, 0> with uint8_t rejects positive");

// ============================================================================
// Test: range<> - Type-Specific Full Range
// ============================================================================

// Test: range<0, 255> with uint8_t (full type range)
constexpr bool test_range_uint8_full_range() {
    struct Test {
        Annotated<uint8_t, range<0, 255>> value;
    };
    
    Test obj1{};
    std::string json1 = R"({"value": 0})";
    auto result1 = Parse(obj1, json1);
    
    Test obj2{};
    std::string json2 = R"({"value": 255})";
    auto result2 = Parse(obj2, json2);
    
    return result1 && obj1.value.get() == 0
        && result2 && obj2.value.get() == 255;
}
static_assert(test_range_uint8_full_range(), "range<0, 255> with uint8_t (full type range)");

// Test: range<0, 65535> with uint16_t (full type range)
constexpr bool test_range_uint16_full_range() {
    struct Test {
        Annotated<uint16_t, range<0, 65535>> value;
    };
    
    Test obj1{};
    std::string json1 = R"({"value": 0})";
    auto result1 = Parse(obj1, json1);
    
    Test obj2{};
    std::string json2 = R"({"value": 65535})";
    auto result2 = Parse(obj2, json2);
    
    return result1 && obj1.value.get() == 0
        && result2 && obj2.value.get() == 65535;
}
static_assert(test_range_uint16_full_range(), "range<0, 65535> with uint16_t (full type range)");

// ============================================================================
// Test: range<> - Multiple Unsigned Types in Same Struct
// ============================================================================

constexpr bool test_range_multiple_unsigned_types() {
    struct Test {
        Annotated<uint8_t, range<10, 100>> value8;
        Annotated<uint16_t, range<100, 1000>> value16;
        Annotated<uint32_t, range<1000, 10000>> value32;
        Annotated<uint64_t, range<10000, 100000>> value64;
    };
    
    Test obj{};
    std::string json = R"({"value8": 50, "value16": 500, "value32": 5000, "value64": 50000})";
    auto result = Parse(obj, json);
    
    return result 
        && obj.value8.get() == 50
        && obj.value16.get() == 500
        && obj.value32.get() == 5000
        && obj.value64.get() == 50000;
}
static_assert(test_range_multiple_unsigned_types(), "Multiple unsigned types with range in same struct");

constexpr bool test_range_multiple_unsigned_types_one_fails() {
    struct Test {
        Annotated<uint8_t, range<10, 100>> value8;
        Annotated<uint16_t, range<100, 1000>> value16;
    };
    
    Test obj{};
    std::string json = R"({"value8": 50, "value16": 1001})";  // value16 out of range
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_multiple_unsigned_types_one_fails(), "Multiple unsigned types - one fails");

