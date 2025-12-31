#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/options.hpp>
#include <string>
#include <array>
#include <vector>

using namespace JsonFusion;
using namespace JsonFusion::options;
using namespace TestHelpers;

// ============================================================================
// Test: wire_sink<> - Capture Raw JSON
// ============================================================================

// Test: wire_sink with std::string - captures primitive value
constexpr bool test_json_sink_string_primitive() {
    struct Test {
        Annotated<std::string, wire_sink<>> captured;
    };
    
    Test obj{};
    std::string json = R"({"captured": 42})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    
    // Should capture "42" (whitespace removed)
    if (obj.captured.get() != "42") return false;
    
    return true;
}
static_assert(test_json_sink_string_primitive(), "wire_sink with std::string - captures primitive");

// Test: wire_sink with std::string - captures string value
constexpr bool test_json_sink_string_string_value() {
    struct Test {
        Annotated<std::string, wire_sink<>> captured;
    };
    
    Test obj{};
    std::string json = R"({"captured": "hello"})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    
    // Should capture "hello" (quotes included in raw JSON)
    if (obj.captured.get() != R"("hello")") return false;
    
    return true;
}
static_assert(test_json_sink_string_string_value(), "wire_sink with std::string - captures string value");

// Test: wire_sink with std::string - captures object
constexpr bool test_json_sink_string_object() {
    struct Test {
        Annotated<std::string, wire_sink<>> captured;
    };
    
    Test obj{};
    std::string json = R"({"captured": {"a": 1, "b": 2}})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    
    // Should capture object (whitespace removed)
    std::string expected = R"({"a":1,"b":2})";
    if (obj.captured.get() != expected) return false;
    
    return true;
}
static_assert(test_json_sink_string_object(), "wire_sink with std::string - captures object");

// Test: wire_sink with std::string - captures array
constexpr bool test_json_sink_string_array() {
    struct Test {
        Annotated<std::string, wire_sink<>> captured;
    };
    
    Test obj{};
    std::string json = R"({"captured": [1, 2, 3]})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    
    // Should capture array (whitespace removed)
    std::string expected = "[1,2,3]";
    if (obj.captured.get() != expected) return false;
    
    return true;
}
static_assert(test_json_sink_string_array(), "wire_sink with std::string - captures array");

// Test: wire_sink with std::string - captures boolean
constexpr bool test_json_sink_string_boolean() {
    struct Test {
        Annotated<std::string, wire_sink<>> captured;
    };
    
    Test obj{};
    std::string json = R"({"captured": true})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    
    // Should capture "true"
    if (obj.captured.get() != "true") return false;
    
    return true;
}
static_assert(test_json_sink_string_boolean(), "wire_sink with std::string - captures boolean");

// Test: wire_sink with std::string - captures null
constexpr bool test_json_sink_string_null() {
    struct Test {
        Annotated<std::string, wire_sink<>> captured;
    };
    
    Test obj{};
    std::string json = R"({"captured": null})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    
    // Should capture "null"
    if (obj.captured.get() != "null") return false;
    
    return true;
}
static_assert(test_json_sink_string_null(), "wire_sink with std::string - captures null");

// Test: wire_sink with std::array<char, N> - fixed size buffer
constexpr bool test_json_sink_array_fixed_size() {
    struct Test {
        Annotated<std::array<char, 32>, wire_sink<>> captured;
    };
    
    Test obj{};
    std::string json = R"({"captured": "hello"})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    
    // Should capture into array
    std::string_view captured_view(obj.captured.get().data(), 7);  // R"("hello")" is 7 chars
    if (captured_view != R"("hello")") return false;
    
    return true;
}
static_assert(test_json_sink_array_fixed_size(), "wire_sink with std::array<char, N> - fixed size buffer");

// Test: wire_sink with std::array<char, N> - fails when buffer too small
constexpr bool test_json_sink_array_truncation() {
    struct Test {
        Annotated<std::array<char, 10>, wire_sink<>> captured;
    };
    
    Test obj{};
    // Short string should work
    std::string json1 = R"({"captured": "short"})";
    auto result1 = Parse(obj, json1);
    if (!result1) return false;
    
    // Long string should fail due to buffer overflow
    std::string json2 = R"({"captured": "very long string that exceeds buffer"})";
    auto result2 = Parse(obj, json2);
    if (result2) return false;  // Should fail with buffer overflow
    
    return true;
}
static_assert(test_json_sink_array_truncation(), "wire_sink with std::array<char, N> - fails when buffer too small");

// Test: wire_sink with MaxStringLength limit
constexpr bool test_json_sink_string_length_limit() {
    struct Test {
        Annotated<std::string, wire_sink<64, 20>> captured;  // Max 20 chars
    };
    
    Test obj{};
    // Short string should work
    std::string json1 = R"({"captured": "short"})";
    auto result1 = Parse(obj, json1);
    if (!result1) return false;
    if (obj.captured.get() != R"("short")") return false;
    
    // Long string should fail or be truncated
    std::string json2 = R"({"captured": "this is a very long string that exceeds the limit"})";
    auto result2 = Parse(obj, json2);
    // Should fail due to length limit
    if (result2) return false;
    
    return true;
}
static_assert(test_json_sink_string_length_limit(), "wire_sink with MaxStringLength limit");

// Test: wire_sink with nested structures
constexpr bool test_json_sink_nested_structures() {
    struct Test {
        Annotated<std::string, wire_sink<>> captured;
    };
    
    Test obj{};
    std::string json = R"({"captured": {"outer": {"inner": [1, 2, {"deep": true}]}}})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    
    // Should capture nested structure (whitespace removed)
    std::string expected = R"({"outer":{"inner":[1,2,{"deep":true}]}})";
    if (obj.captured.get() != expected) return false;
    
    return true;
}
static_assert(test_json_sink_nested_structures(), "wire_sink with nested structures");

// Test: wire_sink with custom depth limit
// Depth limit = max number of opened arrays/objects during parsing
constexpr bool test_json_sink_custom_depth() {
    struct Test {
        Annotated<std::string, wire_sink<2>> captured;  // Max 2 opened arrays/objects
    };
    
    Test obj{};
    // Depth 1: single object {"a": 1} - should work
    std::string json1 = R"({"captured": {"a": 1}})";
    auto result1 = Parse(obj, json1);
    if (!result1) return false;
    if (obj.captured.get() != R"({"a":1})") return false;
    
    // Depth 2: nested object {"a": {"b": 1}} - should work (outer object + inner object = 2)
    std::string json2 = R"({"captured": {"a": {"b": 1}}})";
    auto result2 = Parse(obj, json2);
    if (!result2) return false;
    if (obj.captured.get() != R"({"a":{"b":1}})") return false;
    
    // Depth 3: triple nested {"a": {"b": {"c": 1}}} - should fail (exceeds limit of 2)
    std::string json3 = R"({"captured": {"a": {"b": {"c": 1}}}})";
    auto result3 = Parse(obj, json3);
    if (result3) return false;  // Should fail due to depth limit
    
    // Depth 2 with array: [{"a": 1}] - should work (array + object = 2)
    std::string json4 = R"({"captured": [{"a": 1}]})";
    auto result4 = Parse(obj, json4);
    if (!result4) return false;
    if (obj.captured.get() != R"([{"a":1}])") return false;
    
    return true;
}
static_assert(test_json_sink_custom_depth(), "wire_sink with custom depth limit");

// Test: wire_sink - whitespace removal
constexpr bool test_json_sink_whitespace_removal() {
    struct Test {
        Annotated<std::string, wire_sink<>> captured;
    };
    
    Test obj{};
    // JSON with lots of whitespace
    std::string json = R"({"captured": { "a" : 1 , "b" : [ 2 , 3 ] } })";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    
    // Should have whitespace removed
    std::string expected = R"({"a":1,"b":[2,3]})";
    if (obj.captured.get() != expected) return false;
    
    return true;
}
static_assert(test_json_sink_whitespace_removal(), "wire_sink - whitespace removal");

// Test: wire_sink - validates JSON correctness
constexpr bool test_json_sink_validates_json() {
    struct Test {
        Annotated<std::string, wire_sink<>> captured;
    };
    
    Test obj{};
    // Invalid JSON should fail
    std::string json = R"({"captured": [unclosed)";
    auto result = Parse(obj, json);
    
    // Should fail due to invalid JSON
    if (result) return false;
    
    return true;
}
static_assert(test_json_sink_validates_json(), "wire_sink - validates JSON correctness");

// Test: wire_sink with multiple fields
constexpr bool test_json_sink_multiple_fields() {
    struct Test {
        int regular;
        Annotated<std::string, wire_sink<>> captured1;
        Annotated<std::string, wire_sink<>> captured2;
        bool regular2;
    };
    
    Test obj{};
    std::string json = R"({"regular": 42, "captured1": {"a": 1}, "captured2": [1, 2], "regular2": true})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    if (obj.regular != 42) return false;
    if (obj.captured1.get() != R"({"a":1})") return false;
    if (obj.captured2.get() != "[1,2]") return false;
    if (obj.regular2 != true) return false;
    
    return true;
}
static_assert(test_json_sink_multiple_fields(), "wire_sink with multiple fields");

// Test: wire_sink with escape sequences - basic escapes
constexpr bool test_json_sink_escape_sequences_basic() {
    struct Test {
        Annotated<std::string, wire_sink<>> captured;
    };
    
    Test obj{};
    // Use regular string literal to get actual escape sequences in JSON
    std::string json = "{\"captured\": \"hello\\nworld\\t\\\"quote\\\"\"}";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    
    // wire_sink now PRESERVES escape sequences as valid JSON
    // The captured string should contain the raw JSON with escapes intact:
    // "hello\nworld\t\"quote\"" (with literal backslash-n, backslash-t, etc.)
    std::string expected = "\"hello\\nworld\\t\\\"quote\\\"\"";
    if (obj.captured.get() != expected) return false;
    
    return true;
}
static_assert(test_json_sink_escape_sequences_basic(), "wire_sink with escape sequences - preserves escapes");

// Test: wire_sink with all standard escape types
constexpr bool test_json_sink_all_escape_types() {
    struct Test {
        Annotated<std::string, wire_sink<>> captured;
    };
    
    Test obj{};
    // Test all JSON escape sequences: \" \\ \/ \b \f \n \r \t
    std::string json = R"({"captured": "quote:\" slash:\\ solidus:\/ back:\b form:\f newline:\n return:\r tab:\t"})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    
    // All escapes should be preserved
    std::string expected = R"("quote:\" slash:\\ solidus:\/ back:\b form:\f newline:\n return:\r tab:\t")";
    if (obj.captured.get() != expected) return false;
    
    return true;
}
static_assert(test_json_sink_all_escape_types(), "wire_sink with all standard escape types");

// Test: wire_sink with Unicode escape sequences
constexpr bool test_json_sink_unicode_escapes() {
    struct Test {
        Annotated<std::string, wire_sink<>> captured;
    };
    
    Test obj{};
    // Unicode escape: \u0041 is 'A'
    std::string json = R"({"captured": "Unicode: \u0041\u0042\u0043"})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    
    // Unicode escapes should be preserved as-is
    std::string expected = R"("Unicode: \u0041\u0042\u0043")";
    if (obj.captured.get() != expected) return false;
    
    return true;
}
static_assert(test_json_sink_unicode_escapes(), "wire_sink with Unicode escapes");

// Test: wire_sink with Unicode surrogate pairs
constexpr bool test_json_sink_surrogate_pairs() {
    struct Test {
        Annotated<std::string, wire_sink<>> captured;
    };
    
    Test obj{};
    // Surrogate pair for emoji 😀 (U+1F600): \ud83d\ude00
    std::string json = R"({"captured": "emoji: \ud83d\ude00"})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    
    // Surrogate pairs should be preserved
    std::string expected = R"("emoji: \ud83d\ude00")";
    if (obj.captured.get() != expected) return false;
    
    return true;
}
static_assert(test_json_sink_surrogate_pairs(), "wire_sink with Unicode surrogate pairs");

// Test: wire_sink with escapes in nested structures
constexpr bool test_json_sink_escapes_in_nested() {
    struct Test {
        Annotated<std::string, wire_sink<>> captured;
    };
    
    Test obj{};
    std::string json = R"({"captured": {"key": "val\nue", "array": ["item\t1", "item\"2"]}})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    
    // Escapes should be preserved in nested structures
    std::string expected = R"({"key":"val\nue","array":["item\t1","item\"2"]})";
    if (obj.captured.get() != expected) return false;
    
    return true;
}
static_assert(test_json_sink_escapes_in_nested(), "wire_sink with escapes in nested structures");

// Test: wire_sink with mixed content and escapes
constexpr bool test_json_sink_mixed_escapes() {
    struct Test {
        Annotated<std::string, wire_sink<>> captured;
    };
    
    Test obj{};
    // Mix of strings with escapes, numbers, booleans, null
    std::string json = R"({"captured": [42, "text\nwith\nlines", true, null, "quote:\""]})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    
    // All escapes preserved
    std::string expected = R"([42,"text\nwith\nlines",true,null,"quote:\""])";
    if (obj.captured.get() != expected) return false;
    
    return true;
}
static_assert(test_json_sink_mixed_escapes(), "wire_sink with mixed content and escapes");

// Test: wire_sink with number formats
constexpr bool test_json_sink_number_formats() {
    struct Test {
        Annotated<std::string, wire_sink<>> captured;
    };
    
    Test obj{};
    // Test various number formats
    std::string json1 = R"({"captured": 42})";
    auto result1 = Parse(obj, json1);
    if (!result1 || obj.captured.get() != "42") return false;
    
    std::string json2 = R"({"captured": -123})";
    auto result2 = Parse(obj, json2);
    if (!result2 || obj.captured.get() != "-123") return false;
    
    std::string json3 = R"({"captured": 0})";
    auto result3 = Parse(obj, json3);
    if (!result3 || obj.captured.get() != "0") return false;
    
    return true;
}
static_assert(test_json_sink_number_formats(), "wire_sink with number formats");

