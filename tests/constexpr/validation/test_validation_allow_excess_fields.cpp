#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/options.hpp>
#include <JsonFusion/validators.hpp>

using namespace JsonFusion;
using namespace JsonFusion::options;
using namespace JsonFusion::validators;

// ============================================================================
// Test: allow_excess_fields<> - Unknown Fields Without Validator
// ============================================================================

// Test: Unknown field without allow_excess_fields - should fail
constexpr bool test_allow_excess_fields_unknown_field_fails() {
    struct Test {
        int required;
    };
    
    Test obj{};
    std::string json = R"({"required": 42, "unknown": 100})";  // unknown field present
    auto result = Parse(obj, json);
    
    return !result && result.error() == ParseError::EXCESS_FIELD;
}
static_assert(test_allow_excess_fields_unknown_field_fails(), "Unknown field without allow_excess_fields fails");

// Test: Only unknown fields without allow_excess_fields - should fail
constexpr bool test_allow_excess_fields_only_unknown_fails() {
    struct Test {
        int required;
    };
    
    Test obj{};
    std::string json = R"({"unknown1": 1, "unknown2": 2})";  // only unknown fields
    auto result = Parse(obj, json);
    
    return !result && result.error() == ParseError::EXCESS_FIELD;
}
static_assert(test_allow_excess_fields_only_unknown_fails(), "Only unknown fields without allow_excess_fields fails");

// ============================================================================
// Test: allow_excess_fields<> - Unknown Fields With Validator
// ============================================================================

// Test: Unknown field with allow_excess_fields - should succeed
constexpr bool test_allow_excess_fields_unknown_field_allowed() {
    struct Test {
        int required;
    };
    
    Annotated<Test, allow_excess_fields<>> obj{};
    std::string json = R"({"required": 42, "unknown": 100})";  // unknown field present
    auto result = Parse(obj, json);
    
    return result && obj.get().required == 42;
}
static_assert(test_allow_excess_fields_unknown_field_allowed(), "Unknown field with allow_excess_fields succeeds");

// Test: Multiple unknown fields with allow_excess_fields - should succeed
constexpr bool test_allow_excess_fields_multiple_unknown_allowed() {
    struct Test {
        int required;
    };
    
    Annotated<Test, allow_excess_fields<>> obj{};
    std::string json = R"({"required": 42, "unknown1": 100, "unknown2": 200, "unknown3": 300})";
    auto result = Parse(obj, json);
    
    return result && obj.get().required == 42;
}
static_assert(test_allow_excess_fields_multiple_unknown_allowed(), "Multiple unknown fields with allow_excess_fields succeed");

// Test: Only unknown fields with allow_excess_fields - should fail because required field missing
constexpr bool test_allow_excess_fields_only_unknown_allowed() {
    struct Test {
        int required;
    };
    
    Annotated<Test, allow_excess_fields<>> obj{};
    std::string json = R"({"unknown1": 1, "unknown2": 2})";  // only unknown fields, required missing
    auto result = Parse(obj, json);
    
    return !result;
}
static_assert(test_allow_excess_fields_only_unknown_allowed(), "Only unknown fields with allow_excess_fields - fails on missing required");

// Test: All known fields present, no unknown fields - should succeed
constexpr bool test_allow_excess_fields_all_known_succeeds() {
    struct Test {
        int field1;
        int field2;
    };
    
    Annotated<Test, allow_excess_fields<>> obj{};
    std::string json = R"({"field1": 10, "field2": 20})";  // all known fields
    auto result = Parse(obj, json);
    
    return result && obj.get().field1 == 10 && obj.get().field2 == 20;
}
static_assert(test_allow_excess_fields_all_known_succeeds(), "All known fields with allow_excess_fields succeeds");

// ============================================================================
// Test: allow_excess_fields<> - Mixed Known and Unknown Fields
// ============================================================================

// Test: Mix of known and unknown fields - should succeed
constexpr bool test_allow_excess_fields_mixed_fields() {
    struct Test {
        int known1;
        int known2;
    };
    
    Annotated<Test, allow_excess_fields<>> obj{};
    std::string json = R"({"known1": 1, "known2": 2, "unknown1": 100})";
    auto result = Parse(obj, json);
    
    return result && obj.get().known1 == 1 && obj.get().known2 == 2;
}
static_assert(test_allow_excess_fields_mixed_fields(), "Mix of known and unknown fields succeeds");

// ============================================================================
// Test: allow_excess_fields<> - Nested Objects
// ============================================================================

// Test: Nested object - allow_excess_fields only applies to the struct it's on
constexpr bool test_allow_excess_fields_nested() {
    struct Inner {
        int inner_required;
    };
    
    struct Outer {
        int outer_required;
        Inner inner;
    };
    
    Annotated<Outer, allow_excess_fields<>> obj{};
    std::string json = R"({"outer_required": 1, "outer_unknown": 50, "inner": {"inner_required": 2}})";
    auto result = Parse(obj, json);
    
    return result 
        && obj.get().outer_required == 1
        && obj.get().inner.inner_required == 2;
}
static_assert(test_allow_excess_fields_nested(), "Nested object - allow_excess_fields allows unknown fields in outer struct");

// Test: Both outer and inner with allow_excess_fields - all unknown fields allowed
constexpr bool test_allow_excess_fields_nested_both_allowed() {
    struct Inner {
        int inner_required;
    };
    
    struct Outer {
        int outer_required;
        Annotated<Inner, allow_excess_fields<>> inner;
    };
    
    Annotated<Outer, allow_excess_fields<>> obj{};
    std::string json = R"({"outer_required": 1, "outer_unknown": 50, "inner": {"inner_required": 2, "inner_unknown": 100}})";
    auto result = Parse(obj, json);
    
    return result 
        && obj.get().outer_required == 1
        && obj.get().inner.get().inner_required == 2;
}
static_assert(test_allow_excess_fields_nested_both_allowed(), "Both outer and inner with allow_excess_fields - all unknown fields allowed");

// ============================================================================
// Test: allow_excess_fields<> - Interaction with not_required
// ============================================================================

// Test: allow_excess_fields + not_required - both work together
constexpr bool test_allow_excess_fields_with_not_required() {
    struct Test {
        int required;
        int optional;
    };
    
    Annotated<Test, allow_excess_fields<>, not_required<"optional">> obj{};
    std::string json = R"({"required": 42, "unknown": 100})";  // optional absent, unknown present
    auto result = Parse(obj, json);
    
    return result && obj.get().required == 42;
}
static_assert(test_allow_excess_fields_with_not_required(), "allow_excess_fields + not_required work together");

// ============================================================================
// Test: allow_excess_fields<> - Empty Object
// ============================================================================

// Test: Empty object with allow_excess_fields - should fail if required fields missing
constexpr bool test_allow_excess_fields_empty_object() {
    struct Test {
        int required;
    };
    
    Annotated<Test, allow_excess_fields<>> obj{};
    std::string json = R"({})";  // empty object, required field missing
    auto result = Parse(obj, json);
    
    return !result;
}
static_assert(test_allow_excess_fields_empty_object(), "Empty object with allow_excess_fields - fails on missing required");

// Test: Empty object with all fields not_required and allow_excess_fields - should succeed
constexpr bool test_allow_excess_fields_empty_object_all_optional() {
    struct Test {
        int optional1;
        int optional2;
    };
    
    Annotated<Test, allow_excess_fields<>, not_required<"optional1", "optional2">> obj{};
    std::string json = R"({})";  // empty object, all fields optional
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_allow_excess_fields_empty_object_all_optional(), "Empty object with all fields optional and allow_excess_fields succeeds");

