#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/validators.hpp>
#include <JsonFusion/options.hpp>
#include <optional>

using namespace JsonFusion;
using namespace JsonFusion::validators;
using namespace JsonFusion::options;

// ============================================================================
// Test: Default Behavior - All Fields Not Required
// ============================================================================

// Test: By default, all fields are optional (no annotation)
constexpr bool test_default_all_fields_optional() {
    struct Test {
        int field1;
        int field2;
    };
    
    Test obj{};
    std::string json = R"({})";  // all fields absent
    auto result = Parse(obj, json);
    
    return result;  // Should succeed - all fields optional by default
}
static_assert(test_default_all_fields_optional(), "By default, all fields are optional");

// Test: By default, fields can be present
constexpr bool test_default_fields_can_be_present() {
    struct Test {
        int field1;
        int field2;
    };
    
    Test obj{};
    std::string json = R"({"field1": 10, "field2": 20})";
    auto result = Parse(obj, json);
    
    return result && obj.field1 == 10 && obj.field2 == 20;
}
static_assert(test_default_fields_can_be_present(), "By default, fields can be present");

// Test: By default, some fields can be present, others absent
constexpr bool test_default_some_present() {
    struct Test {
        int field1;
        int field2;
    };
    
    Test obj{};
    std::string json = R"({"field1": 10})";  // field2 absent
    auto result = Parse(obj, json);
    
    return result && obj.field1 == 10;
}
static_assert(test_default_some_present(), "By default, some fields can be present, others absent");

// ============================================================================
// Test: required<> - Specific Fields Are Required
// ============================================================================

// Test: required<"field1"> - field1 must be present
constexpr bool test_required_field_present() {
    struct Test {
        int field1;
        int field2;
    };
    
    Annotated<Test, required<"field1">> obj{};
    std::string json = R"({"field1": 42})";  // field1 present, field2 absent (optional)
    auto result = Parse(obj, json);
    
    return result && obj.get().field1 == 42;
}
static_assert(test_required_field_present(), "required field can be present");

// Test: required<"field1"> - field1 missing causes error
constexpr bool test_required_field_missing() {
    struct Test {
        int field1;
        int field2;
    };
    
    Annotated<Test, required<"field1">> obj{};
    std::string json = R"({"field2": 100})";  // field1 missing
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::missing_required_fields;
}
static_assert(test_required_field_missing(), "required field missing causes error");

// Test: required<"field1", "field2"> - both must be present
constexpr bool test_required_multiple_all_present() {
    struct Test {
        int field1;
        int field2;
        int field3;
    };
    
    Annotated<Test, required<"field1", "field2">> obj{};
    std::string json = R"({"field1": 10, "field2": 20})";  // field3 absent (optional)
    auto result = Parse(obj, json);
    
    return result && obj.get().field1 == 10 && obj.get().field2 == 20;
}
static_assert(test_required_multiple_all_present(), "Multiple required fields - all present");

// Test: required<"field1", "field2"> - one missing causes error
constexpr bool test_required_multiple_one_missing() {
    struct Test {
        int field1;
        int field2;
        int field3;
    };
    
    Annotated<Test, required<"field1", "field2">> obj{};
    std::string json = R"({"field1": 10})";  // field2 missing
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::missing_required_fields;
}
static_assert(test_required_multiple_one_missing(), "Multiple required fields - one missing causes error");

// Test: required<"field1"> - other fields are optional
constexpr bool test_required_others_optional() {
    struct Test {
        int field1;
        int field2;
        int field3;
    };
    
    Annotated<Test, required<"field1">> obj{};
    std::string json = R"({"field1": 42})";  // field2 and field3 absent (both optional)
    auto result = Parse(obj, json);
    
    return result && obj.get().field1 == 42;
}
static_assert(test_required_others_optional(), "required - other fields are optional");

// Test: required<"field1"> - all fields can be present
constexpr bool test_required_all_can_be_present() {
    struct Test {
        int field1;
        int field2;
    };
    
    Annotated<Test, required<"field1">> obj{};
    std::string json = R"({"field1": 10, "field2": 20})";
    auto result = Parse(obj, json);
    
    return result && obj.get().field1 == 10 && obj.get().field2 == 20;
}
static_assert(test_required_all_can_be_present(), "required - all fields can be present");

// ============================================================================
// Test: not_required<> - Specific Fields Are Not Required, Others Are
// ============================================================================

// Test: not_required<"optional"> - optional field can be absent
constexpr bool test_not_required_field_absent() {
    struct Test {
        int required;
        int optional;
    };
    
    Annotated<Test, not_required<"optional">> obj{};
    std::string json = R"({"required": 42})";  // optional field absent
    auto result = Parse(obj, json);
    
    return result && obj.get().required == 42;
}
static_assert(test_not_required_field_absent(), "not_required field can be absent");

// Test: not_required<"optional"> - optional field can be present
constexpr bool test_not_required_field_present() {
    struct Test {
        int required;
        int optional;
    };
    
    Annotated<Test, not_required<"optional">> obj{};
    std::string json = R"({"required": 42, "optional": 100})";
    auto result = Parse(obj, json);
    
    return result && obj.get().required == 42 && obj.get().optional == 100;
}
static_assert(test_not_required_field_present(), "not_required field can be present");

// Test: not_required<"optional"> - required field missing causes error
constexpr bool test_not_required_required_missing() {
    struct Test {
        int required;
        int optional;
    };
    
    Annotated<Test, not_required<"optional">> obj{};
    std::string json = R"({"optional": 100})";  // required field missing
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::missing_required_fields;
}
static_assert(test_not_required_required_missing(), "not_required - required field missing causes error");

// Test: not_required<"field1", "field2"> - multiple fields can be absent
constexpr bool test_not_required_multiple_absent() {
    struct Test {
        int required;
        int field1;
        int field2;
    };
    
    Annotated<Test, not_required<"field1", "field2">> obj{};
    std::string json = R"({"required": 42})";  // both optional fields absent
    auto result = Parse(obj, json);
    
    return result && obj.get().required == 42;
}
static_assert(test_not_required_multiple_absent(), "Multiple not_required fields can be absent");

// Test: not_required<"field1", "field2"> - some present, some absent
constexpr bool test_not_required_some_present() {
    struct Test {
        int required;
        int field1;
        int field2;
    };
    
    Annotated<Test, not_required<"field1", "field2">> obj{};
    std::string json = R"({"required": 42, "field1": 10})";  // field1 present, field2 absent
    auto result = Parse(obj, json);
    
    return result && obj.get().required == 42 && obj.get().field1 == 10;
}
static_assert(test_not_required_some_present(), "Some not_required fields can be present, others absent");

// Test: not_required<"field1"> - other required field missing
constexpr bool test_not_required_other_required_missing() {
    struct Test {
        int required1;
        int required2;
        int field1;
    };
    
    Annotated<Test, not_required<"field1">> obj{};
    std::string json = R"({"required1": 42})";  // required2 missing
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::missing_required_fields;
}
static_assert(test_not_required_other_required_missing(), "not_required - other required field missing causes error");

// Test: All fields not_required - all can be absent
constexpr bool test_not_required_all_absent() {
    struct Test {
        int field1;
        int field2;
    };
    
    Annotated<Test, not_required<"field1", "field2">> obj{};
    std::string json = R"({})";  // all fields absent
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_not_required_all_absent(), "All fields not_required - all can be absent");

// Test: All fields not_required - all can be present
constexpr bool test_not_required_all_present() {
    struct Test {
        int field1;
        int field2;
    };
    
    Annotated<Test, not_required<"field1", "field2">> obj{};
    std::string json = R"({"field1": 10, "field2": 20})";
    auto result = Parse(obj, json);
    
    return result && obj.get().field1 == 10 && obj.get().field2 == 20;
}
static_assert(test_not_required_all_present(), "All fields not_required - all can be present");

// ============================================================================
// Test: Mix of required<> and not_required<> - Complex Scenarios
// ============================================================================

// Test: Mix - some required, some not_required
constexpr bool test_mix_required_and_not_required_all_present() {
    struct Test {
        int required1;
        int required2;
        int optional1;
        int optional2;
    };
    
    Annotated<Test, not_required<"optional1", "optional2">> obj{};
    std::string json = R"({"required1": 1, "required2": 2, "optional1": 10, "optional2": 20})";
    auto result = Parse(obj, json);
    
    return result 
        && obj.get().required1 == 1
        && obj.get().required2 == 2
        && obj.get().optional1 == 10
        && obj.get().optional2 == 20;
}
static_assert(test_mix_required_and_not_required_all_present(), "Mix of required and not_required - all present");

// Test: Mix - required present, optional absent
constexpr bool test_mix_required_present_optional_absent() {
    struct Test {
        int required1;
        int required2;
        int optional1;
        int optional2;
    };
    
    Annotated<Test, not_required<"optional1", "optional2">> obj{};
    std::string json = R"({"required1": 1, "required2": 2})";  // optional fields absent
    auto result = Parse(obj, json);
    
    return result 
        && obj.get().required1 == 1
        && obj.get().required2 == 2;
}
static_assert(test_mix_required_present_optional_absent(), "Mix - required present, optional absent");

// Test: Mix - one required missing
constexpr bool test_mix_required_missing() {
    struct Test {
        int required1;
        int required2;
        int optional1;
    };
    
    Annotated<Test, not_required<"optional1">> obj{};
    std::string json = R"({"required1": 1, "optional1": 10})";  // required2 missing
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::missing_required_fields;
}
static_assert(test_mix_required_missing(), "Mix - one required missing causes error");

// ============================================================================
// Test: Interaction with std::optional
// ============================================================================

// Note: std::optional provides field-level nullability (can be null, but field must be present in JSON)
// required/not_required provides object-level optionality (field can be absent from JSON)
// They work independently - a field can be both std::optional AND required/not_required

// Test: required works with std::optional field
constexpr bool test_required_with_optional_type() {
    struct Test {
        std::optional<int> required_field;
        int optional_field;
    };
    
    Annotated<Test, required<"required_field">> obj{};
    std::string json = R"({"required_field": 42})";  // required_field present (not null)
    auto result = Parse(obj, json);
    
    return result && obj.get().required_field.has_value() && obj.get().required_field.value() == 42;
}
static_assert(test_required_with_optional_type(), "required works with std::optional field");

// Test: required with std::optional - field can be null but must be present
constexpr bool test_required_with_optional_null() {
    struct Test {
        std::optional<int> required_field;
        int optional_field;
    };
    
    Annotated<Test, required<"required_field">> obj{};
    std::string json = R"({"required_field": null})";  // required_field present but null
    auto result = Parse(obj, json);
    
    return result && !obj.get().required_field.has_value();
}
static_assert(test_required_with_optional_null(), "required with std::optional - field can be null but must be present");

// Test: not_required works with regular fields (not std::optional)
constexpr bool test_not_required_regular_field() {
    struct Test {
        int required;
        int not_required_field;
    };
    
    Annotated<Test, not_required<"not_required_field">> obj{};
    std::string json = R"({"required": 42})";  // not_required_field absent
    auto result = Parse(obj, json);
    
    return result && obj.get().required == 42;
}
static_assert(test_not_required_regular_field(), "not_required works with regular fields");

// ============================================================================
// Test: Nested Objects
// ============================================================================

// Test: Nested object with required
constexpr bool test_required_nested() {
    struct Inner {
        int inner_required;
        int inner_optional;
    };
    
    struct Outer {
        int outer_required;
        Annotated<Inner, required<"inner_required">> inner;
    };
    
    Outer obj{};
    std::string json = R"({"outer_required": 1, "inner": {"inner_required": 2}})";  // inner_optional absent
    auto result = Parse(obj, json);
    
    return result 
        && obj.outer_required == 1
        && obj.inner.get().inner_required == 2;
}
static_assert(test_required_nested(), "Nested object with required");

// Test: Nested object - required field missing in inner
constexpr bool test_required_nested_required_missing() {
    struct Inner {
        int inner_required;
        int inner_optional;
    };
    
    struct Outer {
        int outer_required;
        Annotated<Inner, required<"inner_required">> inner;
    };
    
    Outer obj{};
    std::string json = R"({"outer_required": 1, "inner": {"inner_optional": 10}})";  // inner_required missing
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::missing_required_fields;
}
static_assert(test_required_nested_required_missing(), "Nested object - required field missing");

// Test: Nested object with not_required
constexpr bool test_not_required_nested() {
    struct Inner {
        int inner_required;
        int inner_optional;
    };
    
    struct Outer {
        int outer_required;
        Annotated<Inner, not_required<"inner_optional">> inner;
    };
    
    Outer obj{};
    std::string json = R"({"outer_required": 1, "inner": {"inner_required": 2}})";  // inner_optional absent
    auto result = Parse(obj, json);
    
    return result 
        && obj.outer_required == 1
        && obj.inner.get().inner_required == 2;
}
static_assert(test_not_required_nested(), "Nested object with not_required");

// Test: Nested object - required field missing in inner
constexpr bool test_not_required_nested_required_missing() {
    struct Inner {
        int inner_required;
        int inner_optional;
    };
    
    struct Outer {
        int outer_required;
        Annotated<Inner, not_required<"inner_optional">> inner;
    };
    
    Outer obj{};
    std::string json = R"({"outer_required": 1, "inner": {"inner_optional": 10}})";  // inner_required missing
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::missing_required_fields;
}
static_assert(test_not_required_nested_required_missing(), "Nested object - required field missing");

// Test: Different annotations at different levels
constexpr bool test_different_annotations_different_levels() {
    struct Inner {
        int inner_required;
        int inner_optional;
    };
    
    struct Outer {
        int outer_required;
        int outer_optional;
        Annotated<Inner, required<"inner_required">> inner;
    };
    
    Annotated<Outer, not_required<"outer_optional">> obj{};
    std::string json = R"({"outer_required": 1, "inner": {"inner_required": 2}})";  // Both optional fields absent
    auto result = Parse(obj, json);
    
    return result 
        && obj.get().outer_required == 1
        && obj.get().inner.get().inner_required == 2;
}
static_assert(test_different_annotations_different_levels(), "Different annotations at different levels");

// ============================================================================
// Test: With key<> Annotation
// ============================================================================

// Test: required uses JSON key names when key<> annotation is present
constexpr bool test_required_with_key_annotation() {
    struct Test {
        Annotated<int, key<"json_required">> required;
        Annotated<int, key<"json_optional">> optional;
    };
    
    Annotated<Test, required<"json_required">> obj{};  // Use JSON key name from key<> annotation
    std::string json = R"({"json_required": 42})";  // json_optional absent
    auto result = Parse(obj, json);
    
    return result && obj.get().required.get() == 42;
}
static_assert(test_required_with_key_annotation(), "required uses JSON key names when key<> is present");

// Test: required with key<> - required field missing
constexpr bool test_required_key_required_missing() {
    struct Test {
        Annotated<int, key<"json_required">> required;
        Annotated<int, key<"json_optional">> optional;
    };
    
    Annotated<Test, required<"json_required">> obj{};
    std::string json = R"({"json_optional": 100})";  // json_required missing
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::missing_required_fields;
}
static_assert(test_required_key_required_missing(), "required with key<> - required field missing");

// Test: required with key<> - uses JSON key, not C++ field name
constexpr bool test_required_key_vs_field_name() {
    struct Test {
        Annotated<int, key<"json_name">> cpp_field_name;  // C++ name: cpp_field_name, JSON name: json_name
    };
    
    Annotated<Test, required<"json_name">> obj{};  // Must use JSON key name
    std::string json = R"({"json_name": 42})";
    auto result = Parse(obj, json);
    
    return result && obj.get().cpp_field_name.get() == 42;
}
static_assert(test_required_key_vs_field_name(), "required uses JSON key name, not C++ field name when key<> is present");

// Test: not_required uses JSON key names when key<> annotation is present
constexpr bool test_not_required_with_key_annotation() {
    struct Test {
        Annotated<int, key<"json_required">> required;
        Annotated<int, key<"json_optional">> optional;
    };
    
    Annotated<Test, not_required<"json_optional">> obj{};  // Use JSON key name from key<> annotation
    std::string json = R"({"json_required": 42})";  // json_optional absent
    auto result = Parse(obj, json);
    
    return result && obj.get().required.get() == 42;
}
static_assert(test_not_required_with_key_annotation(), "not_required uses JSON key names when key<> is present");

// Test: not_required with key<> - required field missing
constexpr bool test_not_required_key_required_missing() {
    struct Test {
        Annotated<int, key<"json_required">> required;
        Annotated<int, key<"json_optional">> optional;
    };
    
    Annotated<Test, not_required<"json_optional">> obj{};
    std::string json = R"({"json_optional": 100})";  // json_required missing
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::missing_required_fields;
}
static_assert(test_not_required_key_required_missing(), "not_required with key<> - required field missing");

// Test: not_required with key<> - uses JSON key, not C++ field name
constexpr bool test_not_required_key_vs_field_name() {
    struct Test {
        Annotated<int, key<"json_name">> cpp_field_name;  // C++ name: cpp_field_name, JSON name: json_name
    };
    
    Annotated<Test, not_required<"json_name">> obj{};  // Must use JSON key name
    std::string json = R"({})";  // json_name absent
    auto result = Parse(obj, json);
    
    return result;  // Should succeed because json_name is not_required
}
static_assert(test_not_required_key_vs_field_name(), "not_required uses JSON key name, not C++ field name when key<> is present");

// ============================================================================
// Test: Edge Cases
// ============================================================================

// Test: Empty object with all fields required - should fail
constexpr bool test_empty_object_all_required_fails() {
    struct Test {
        int field1;
        int field2;
    };
    
    Annotated<Test, required<"field1", "field2">> obj{};
    std::string json = R"({})";  // all required fields missing
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::missing_required_fields;
}
static_assert(test_empty_object_all_required_fails(), "Empty object with all fields required - should fail");

// Test: Empty object with all fields not_required - should succeed
constexpr bool test_empty_object_all_not_required_succeeds() {
    struct Test {
        int field1;
        int field2;
    };
    
    Annotated<Test, not_required<"field1", "field2">> obj{};
    std::string json = R"({})";  // all fields absent
    auto result = Parse(obj, json);
    
    return result;
}
static_assert(test_empty_object_all_not_required_succeeds(), "Empty object with all fields not_required - should succeed");

// Test: Single field required
constexpr bool test_single_field_required() {
    struct Test {
        int required;
        int optional;
    };
    
    Annotated<Test, required<"required">> obj{};
    std::string json = R"({"required": 42})";
    auto result = Parse(obj, json);
    
    return result && obj.get().required == 42;
}
static_assert(test_single_field_required(), "Single field required");

// Test: Single field not_required
constexpr bool test_single_field_not_required() {
    struct Test {
        int required;
        int optional;
    };
    
    Annotated<Test, not_required<"optional">> obj{};
    std::string json = R"({"required": 42})";
    auto result = Parse(obj, json);
    
    return result && obj.get().required == 42;
}
static_assert(test_single_field_not_required(), "Single field not_required");
