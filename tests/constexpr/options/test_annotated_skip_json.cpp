#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/options.hpp>
#include <string>
#include <vector>
#include <array>

using namespace JsonFusion;
using namespace JsonFusion::options;
using namespace TestHelpers;

// ============================================================================
// Test: skip_json<> - Field Not Filled
// ============================================================================

// Test: skip_json on int - field remains default-initialized
constexpr bool test_skip_json_int_not_filled() {
    struct Test {
        int regular;
        Annotated<int, skip_json<>> skipped;
    };
    
    Test obj{};
    obj.regular = 0;
    obj.skipped.get() = 999;  // Set to non-zero to verify it's not changed
    
    std::string json = R"({"regular": 42, "skipped": 100})";
    auto result = Parse(obj, json);
    
    // Parsing should succeed
    if (!result) return false;
    
    // Regular field should be filled
    if (obj.regular != 42) return false;
    
    // Skipped field should remain unchanged (999)
    if (obj.skipped.get() != 999) return false;
    
    return true;
}
static_assert(test_skip_json_int_not_filled(), "skip_json on int - field not filled");

// Test: skip_json on string - field remains default-initialized
constexpr bool test_skip_json_string_not_filled() {
    struct Test {
        std::string regular;
        Annotated<std::string, skip_json<>> skipped;
    };
    
    Test obj{};
    obj.regular = "initial";
    obj.skipped.get() = "unchanged";
    
    std::string json = R"({"regular": "filled", "skipped": "should_not_appear"})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    if (obj.regular != "filled") return false;
    if (obj.skipped.get() != "unchanged") return false;
    
    return true;
}
static_assert(test_skip_json_string_not_filled(), "skip_json on string - field not filled");

// Test: skip_json on nested object - field remains default-initialized
constexpr bool test_skip_json_nested_object_not_filled() {
    struct Inner {
        int value;
    };
    
    struct Test {
        Inner regular;
        Annotated<Inner, skip_json<>> skipped;
    };
    
    Test obj{};
    obj.regular.value = 0;
    obj.skipped.get().value = 999;
    
    std::string json = R"({"regular": {"value": 42}, "skipped": {"value": 100}})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    if (obj.regular.value != 42) return false;
    if (obj.skipped.get().value != 999) return false;
    
    return true;
}
static_assert(test_skip_json_nested_object_not_filled(), "skip_json on nested object - field not filled");

// Test: skip_json on array - field remains default-initialized
constexpr bool test_skip_json_array_not_filled() {
    struct Test {
        std::vector<int> regular;
        Annotated<std::vector<int>, skip_json<>> skipped;
    };
    
    Test obj{};
    obj.regular = {1, 2, 3};
    obj.skipped.get() = {999};
    
    std::string json = R"({"regular": [10, 20, 30], "skipped": [100, 200, 300]})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    if (obj.regular.size() != 3 || obj.regular[0] != 10 || obj.regular[1] != 20 || obj.regular[2] != 30) return false;
    if (obj.skipped.get().size() != 1 || obj.skipped.get()[0] != 999) return false;
    
    return true;
}
static_assert(test_skip_json_array_not_filled(), "skip_json on array - field not filled");

// Test: skip_json on bool - field remains default-initialized
constexpr bool test_skip_json_bool_not_filled() {
    struct Test {
        bool regular;
        Annotated<bool, skip_json<>> skipped;
    };
    
    Test obj{};
    obj.regular = false;
    obj.skipped.get() = true;  // Set to true, should remain true
    
    std::string json = R"({"regular": true, "skipped": false})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    if (obj.regular != true) return false;
    if (obj.skipped.get() != true) return false;  // Should remain unchanged
    
    return true;
}
static_assert(test_skip_json_bool_not_filled(), "skip_json on bool - field not filled");

// Test: skip_json with custom depth limit
constexpr bool test_skip_json_custom_depth() {
    struct Test {
        Annotated<int, skip_json<2>> skipped;
    };
    
    Test obj{};
    obj.skipped.get() = 999;
    
    // Simple value should work fine
    std::string json1 = R"({"skipped": 42})";
    auto result1 = Parse(obj, json1);
    if (!result1) return false;
    if (obj.skipped.get() != 999) return false;
    
    // Nested object within depth limit should work
    std::string json2 = R"({"skipped": {"a": 1}})";
    auto result2 = Parse(obj, json2);
    if (!result2) return false;
    if (obj.skipped.get() != 999) return false;
    
    return true;
}
static_assert(test_skip_json_custom_depth(), "skip_json with custom depth limit");

// Test: skip_json - JSON structure is still validated
constexpr bool test_skip_json_validates_json() {
    struct Test {
        Annotated<int, skip_json<>> skipped;
    };
    
    Test obj{};
    obj.skipped.get() = 999;
    
    // Invalid JSON should fail even with skip_json
    std::string json = R"({"skipped": [unclosed)";
    auto result = Parse(obj, json);
    
    // Should fail due to invalid JSON
    if (result) return false;
    
    // Field should remain unchanged
    if (obj.skipped.get() != 999) return false;
    
    return true;
}
static_assert(test_skip_json_validates_json(), "skip_json still validates JSON structure");

// Test: skip_json on multiple fields
constexpr bool test_skip_json_multiple_fields() {
    struct Test {
        int regular;
        Annotated<int, skip_json<>> skipped1;
        Annotated<std::string, skip_json<>> skipped2;
        bool regular2;
    };
    
    Test obj{};
    obj.regular = 0;
    obj.skipped1.get() = 111;
    obj.skipped2.get() = "unchanged";
    obj.regular2 = false;
    
    std::string json = R"({"regular": 42, "skipped1": 100, "skipped2": "ignored", "regular2": true})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    if (obj.regular != 42) return false;
    if (obj.skipped1.get() != 111) return false;
    if (obj.skipped2.get() != "unchanged") return false;
    if (obj.regular2 != true) return false;
    
    return true;
}
static_assert(test_skip_json_multiple_fields(), "skip_json on multiple fields");

