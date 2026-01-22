#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/options.hpp>
#include <string>
#include <array>

using namespace JsonFusion;
using namespace JsonFusion::options;
using namespace TestHelpers;

// ============================================================================
// Test: exclude - Field excluded from JSON entirely
// ============================================================================

// Test 1: Basic exclude - field not serialized, not expected in JSON
struct BasicExclude {
    int visible;
    Annotated<int, exclude> hidden;
};

constexpr bool test_exclude_basic_parse() {
    BasicExclude obj{};
    obj.visible = 0;
    obj.hidden.get() = 999;  // Should remain unchanged
    
    std::string json = R"({"visible": 42})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    if (obj.visible != 42) return false;
    if (obj.hidden.get() != 999) return false;  // Hidden field unchanged
    
    return true;
}
static_assert(test_exclude_basic_parse(), "exclude - basic parse without hidden field");

constexpr bool test_exclude_basic_serialize() {
    BasicExclude obj{};
    obj.visible = 42;
    obj.hidden.get() = 999;
    
    std::string out;
    auto result = Serialize(obj, out);
    
    if (!result) return false;
    
    // Output should NOT contain "hidden"
    if (out.find("hidden") != std::string::npos) return false;
    // Output should contain "visible"
    if (out.find("visible") == std::string::npos) return false;
    
    return true;
}
static_assert(test_exclude_basic_serialize(), "exclude - basic serialize excludes hidden field");

// ============================================================================
// Test 2: Excluded field key in JSON should be treated as excess field
// ============================================================================

constexpr bool test_exclude_excess_field_rejected() {
    BasicExclude obj{};
    
    // JSON contains "hidden" key - should be rejected as excess field
    std::string json = R"({"visible": 42, "hidden": 100})";
    auto result = Parse(obj, json);
    
    // Should fail because "hidden" is not a valid JSON field (it's excluded)
    // and by default excess fields are not allowed
    return !result;
}
static_assert(test_exclude_excess_field_rejected(), 
    "exclude - excluded field key in JSON rejected as excess field");

// Test with allow_excess_fields - should succeed and ignore the excluded field
struct BasicExcludeAllowExcess {
    int visible;
    Annotated<int, exclude> hidden;
};

constexpr bool test_exclude_with_allow_excess_fields() {
    Annotated<BasicExcludeAllowExcess, allow_excess_fields> obj{};
    obj.get().visible = 0;
    obj.get().hidden.get() = 999;
    
    // JSON contains "hidden" key - should be ignored with allow_excess_fields
    std::string json = R"({"visible": 42, "hidden": 100})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    if (obj.get().visible != 42) return false;
    // hidden should remain unchanged (ignored, not parsed)
    if (obj.get().hidden.get() != 999) return false;
    
    return true;
}
static_assert(test_exclude_with_allow_excess_fields(), 
    "exclude - with allow_excess_fields, excluded field key is ignored");

// ============================================================================
// Test 3: as_array with excluded field
// ============================================================================

struct PointWithExclude {
    double x;
    Annotated<double, exclude> y;  // Excluded - not in JSON array
    double z;
};

constexpr bool test_exclude_as_array_parse() {
    Annotated<PointWithExclude, as_array> obj{};
    obj.get().x = 0;
    obj.get().y.get() = 999;  // Should remain unchanged
    obj.get().z = 0;
    
    // Only 2 elements: x and z (y is excluded)
    std::string json = R"([1.0, 3.0])";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    if (obj.get().x != 1.0) return false;
    if (obj.get().y.get() != 999) return false;  // Excluded, unchanged
    if (obj.get().z != 3.0) return false;
    
    return true;
}
static_assert(test_exclude_as_array_parse(), "exclude - as_array parse with 2 elements");

constexpr bool test_exclude_as_array_serialize() {
    Annotated<PointWithExclude, as_array> obj{};
    obj.get().x = 1.0;
    obj.get().y.get() = 2.0;  // Should be excluded from output
    obj.get().z = 3.0;
    
    std::string out;
    auto result = Serialize(obj, out);
    
    if (!result) return false;
    
    // Should serialize as [1.0, 3.0] - only 2 elements
    // Parse it back to verify
    Annotated<PointWithExclude, as_array> obj2{};
    obj2.get().y.get() = 999;  // Different value to verify it's not parsed
    
    if (!Parse(obj2, out)) return false;
    if (obj2.get().x != 1.0) return false;
    if (obj2.get().y.get() != 999) return false;  // Unchanged (excluded)
    if (obj2.get().z != 3.0) return false;
    
    return true;
}
static_assert(test_exclude_as_array_serialize(), "exclude - as_array serialize excludes field");

// Test: as_array with wrong number of elements should fail
constexpr bool test_exclude_as_array_too_many_elements() {
    Annotated<PointWithExclude, as_array> obj{};
    
    // 3 elements but only 2 expected (y is excluded)
    std::string json = R"([1.0, 2.0, 3.0])";
    auto result = Parse(obj, json);
    
    // Should fail - too many elements
    return !result;
}
static_assert(test_exclude_as_array_too_many_elements(), 
    "exclude - as_array rejects extra elements");

constexpr bool test_exclude_as_array_too_few_elements() {
    Annotated<PointWithExclude, as_array> obj{};
    
    // Only 1 element but 2 expected (x and z)
    std::string json = R"([1.0])";
    auto result = Parse(obj, json);
    
    // Should fail - not enough elements
    return !result;
}
static_assert(test_exclude_as_array_too_few_elements(), 
    "exclude - as_array rejects too few elements");

// ============================================================================
// Test 4: Multiple excluded fields
// ============================================================================

struct MultipleExclude {
    int a;
    Annotated<int, exclude> b;
    int c;
    Annotated<int, exclude> d;
    int e;
};

constexpr bool test_exclude_multiple_fields_parse() {
    MultipleExclude obj{};
    obj.a = 0;
    obj.b.get() = 100;
    obj.c = 0;
    obj.d.get() = 200;
    obj.e = 0;
    
    std::string json = R"({"a": 1, "c": 3, "e": 5})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    if (obj.a != 1) return false;
    if (obj.b.get() != 100) return false;  // Unchanged
    if (obj.c != 3) return false;
    if (obj.d.get() != 200) return false;  // Unchanged
    if (obj.e != 5) return false;
    
    return true;
}
static_assert(test_exclude_multiple_fields_parse(), "exclude - multiple excluded fields");

constexpr bool test_exclude_multiple_as_array() {
    Annotated<MultipleExclude, as_array> obj{};
    obj.get().b.get() = 100;
    obj.get().d.get() = 200;
    
    // Only a, c, e in array (b, d excluded)
    std::string json = R"([1, 3, 5])";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    if (obj.get().a != 1) return false;
    if (obj.get().b.get() != 100) return false;
    if (obj.get().c != 3) return false;
    if (obj.get().d.get() != 200) return false;
    if (obj.get().e != 5) return false;
    
    return true;
}
static_assert(test_exclude_multiple_as_array(), "exclude - multiple excluded fields as_array");

// ============================================================================
// Test 5: Struct with only excluded fields (edge case)
// ============================================================================

struct AllExcluded {
    Annotated<int, exclude> a;
    Annotated<int, exclude> b;
};

constexpr bool test_exclude_all_fields() {
    AllExcluded obj{};
    obj.a.get() = 100;
    obj.b.get() = 200;
    
    // Empty object expected
    std::string json = R"({})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    if (obj.a.get() != 100) return false;
    if (obj.b.get() != 200) return false;
    
    return true;
}
static_assert(test_exclude_all_fields(), "exclude - all fields excluded parses empty object");

constexpr bool test_exclude_all_fields_serialize() {
    AllExcluded obj{};
    obj.a.get() = 100;
    obj.b.get() = 200;
    
    std::string out;
    auto result = Serialize(obj, out);
    
    if (!result) return false;
    // Should serialize to "{}"
    if (out != "{}") return false;
    
    return true;
}
static_assert(test_exclude_all_fields_serialize(), "exclude - all fields excluded serializes to {}");

// ============================================================================
// Test 6: Nested struct with excluded fields
// ============================================================================

struct Inner {
    int val;
    Annotated<int, exclude> hidden;
};

struct Outer {
    Inner inner;
    Annotated<int, exclude> outer_hidden;
};

constexpr bool test_exclude_nested() {
    Outer obj{};
    obj.inner.val = 0;
    obj.inner.hidden.get() = 100;
    obj.outer_hidden.get() = 200;
    
    std::string json = R"({"inner": {"val": 42}})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    if (obj.inner.val != 42) return false;
    if (obj.inner.hidden.get() != 100) return false;
    if (obj.outer_hidden.get() != 200) return false;
    
    return true;
}
static_assert(test_exclude_nested(), "exclude - nested struct with excluded fields");

// ============================================================================
// All tests passed marker
// ============================================================================

static_assert(true, "All exclude annotation tests passed");
