#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/wire_sink.hpp>
#include <string>

using namespace JsonFusion;
using namespace TestHelpers;

// ============================================================================
// Test: WireSink - Capture and Output Raw JSON
// ============================================================================

// Test: WireSink captures primitive value
constexpr bool test_wiresink_primitive() {
    struct Test {
        WireSink<64> captured;
    };
    
    Test obj{};
    std::string json = R"({"captured": 42})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    
    // Should capture "42" - NO CAST NEEDED, fully constexpr!
    std::string_view captured_view(obj.captured.data(), obj.captured.current_size());
    if (captured_view != "42") return false;
    
    return true;
}
static_assert(test_wiresink_primitive(), "WireSink captures primitive");

// Test: WireSink captures string value
constexpr bool test_wiresink_string() {
    struct Test {
        WireSink<64> captured;
    };
    
    Test obj{};
    std::string json = R"({"captured": "hello"})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    
    // Should capture "hello" with quotes
    std::string_view captured_view(obj.captured.data(), obj.captured.current_size());
    if (captured_view != R"("hello")") return false;
    
    return true;
}
static_assert(test_wiresink_string(), "WireSink captures string");

// Test: WireSink captures object
constexpr bool test_wiresink_object() {
    struct Test {
        WireSink<128> captured;
    };
    
    Test obj{};
    std::string json = R"({"captured": {"a": 1, "b": 2}})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    
    // Should capture object (whitespace removed)
    std::string_view captured_view(obj.captured.data(), obj.captured.current_size());
    if (captured_view != R"({"a":1,"b":2})") return false;
    
    return true;
}
static_assert(test_wiresink_object(), "WireSink captures object");

// Test: WireSink captures array
constexpr bool test_wiresink_array() {
    struct Test {
        WireSink<64> captured;
    };
    
    Test obj{};
    std::string json = R"({"captured": [1, 2, 3]})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    
    // Should capture array (whitespace removed)
    std::string_view captured_view(obj.captured.data(), obj.captured.current_size());
    if (captured_view != "[1,2,3]") return false;
    
    return true;
}
static_assert(test_wiresink_array(), "WireSink captures array");

// Test: WireSink captures boolean
constexpr bool test_wiresink_boolean() {
    struct Test {
        WireSink<64> captured;
    };
    
    Test obj{};
    std::string json = R"({"captured": true})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    
    std::string_view captured_view(obj.captured.data(), obj.captured.current_size());
    if (captured_view != "true") return false;
    
    return true;
}
static_assert(test_wiresink_boolean(), "WireSink captures boolean");

// Test: WireSink captures null
constexpr bool test_wiresink_null() {
    struct Test {
        WireSink<64> captured;
    };
    
    Test obj{};
    std::string json = R"({"captured": null})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    
    std::string_view captured_view(obj.captured.data(), obj.captured.current_size());
    if (captured_view != "null") return false;
    
    return true;
}
static_assert(test_wiresink_null(), "WireSink captures null");

// Test: WireSink with dynamic buffer
constexpr bool test_wiresink_dynamic() {
    struct Test {
        WireSink<128, true> captured;  // Dynamic buffer
    };
    
    Test obj{};
    std::string json = R"({"captured": [1, 2, 3, 4, 5]})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    
    std::string_view captured_view(obj.captured.data(), obj.captured.current_size());
    if (captured_view != "[1,2,3,4,5]") return false;
    
    return true;
}
static_assert(test_wiresink_dynamic(), "WireSink with dynamic buffer");

// Test: WireSink with nested structures
constexpr bool test_wiresink_nested() {
    struct Test {
        WireSink<256> captured;
    };
    
    Test obj{};
    std::string json = R"({"captured": {"outer": {"inner": [1, 2, {"deep": true}]}}})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    
    std::string_view captured_view(obj.captured.data(), obj.captured.current_size());
    if (captured_view != R"({"outer":{"inner":[1,2,{"deep":true}]}})") return false;
    
    return true;
}
static_assert(test_wiresink_nested(), "WireSink captures nested structures");

// Test: WireSink with multiple fields
constexpr bool test_wiresink_multiple_fields() {
    struct Test {
        int regular;
        WireSink<64> captured1;
        WireSink<64> captured2;
        bool regular2;
    };
    
    Test obj{};
    std::string json = R"({"regular": 42, "captured1": {"a": 1}, "captured2": [1, 2], "regular2": true})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    if (obj.regular != 42) return false;
    
    std::string_view captured1_view(obj.captured1.data(), obj.captured1.current_size());
    if (captured1_view != R"({"a":1})") return false;
    
    std::string_view captured2_view(obj.captured2.data(), obj.captured2.current_size());
    if (captured2_view != "[1,2]") return false;
    
    if (obj.regular2 != true) return false;
    
    return true;
}
static_assert(test_wiresink_multiple_fields(), "WireSink with multiple fields");

// Test: WireSink roundtrip (parse → serialize)
constexpr bool test_wiresink_roundtrip() {
    struct Test {
        int id;
        WireSink<128> data;
    };
    
    // Parse
    Test obj{};
    std::string json_in = R"({"id": 123, "data": {"nested": [1, 2, 3]}})";
    auto parse_result = Parse(obj, json_in);
    if (!parse_result) return false;
    
    // Serialize
    std::string json_out;
    auto serialize_result = Serialize(obj, json_out);
    if (!serialize_result) return false;
    
    // Verify output contains the captured data
    if (json_out.find(R"({"nested":[1,2,3]})") == std::string::npos) return false;
    
    return true;
}
static_assert(test_wiresink_roundtrip(), "WireSink roundtrip parse → serialize");

// Test: WireSink overflow detection
constexpr bool test_wiresink_overflow() {
    struct Test {
        WireSink<10> captured;  // Very small buffer
    };
    
    Test obj{};
    std::string json = R"({"captured": "this string is way too long for the buffer"})";
    auto result = Parse(obj, json);
    
    // Should fail due to buffer overflow
    if (result) return false;
    
    return true;
}
static_assert(test_wiresink_overflow(), "WireSink detects overflow");

// Test: WireSink with escape sequences
constexpr bool test_wiresink_escapes() {
    struct Test {
        WireSink<128> captured;
    };
    
    Test obj{};
    std::string json = R"({"captured": "hello\nworld\t\"quote\""})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    
    // Escapes should be preserved in raw JSON
    std::string_view captured_view(obj.captured.data(), obj.captured.current_size());
    if (captured_view != R"("hello\nworld\t\"quote\"")") return false;
    
    return true;
}
static_assert(test_wiresink_escapes(), "WireSink preserves escape sequences");

// Test: WireSink with Unicode
constexpr bool test_wiresink_unicode() {
    struct Test {
        WireSink<128> captured;
    };
    
    Test obj{};
    std::string json = R"({"captured": "Unicode: \u0041\u0042\u0043"})";
    auto result = Parse(obj, json);
    
    if (!result) return false;
    
    std::string_view captured_view(obj.captured.data(), obj.captured.current_size());
    if (captured_view != R"("Unicode: \u0041\u0042\u0043")") return false;
    
    return true;
}
static_assert(test_wiresink_unicode(), "WireSink preserves Unicode escapes");

// Test: WireSink validates JSON correctness
constexpr bool test_wiresink_validates() {
    struct Test {
        WireSink<64> captured;
    };
    
    Test obj{};
    std::string json = R"({"captured": [unclosed)";
    auto result = Parse(obj, json);
    
    // Should fail due to invalid JSON
    if (result) return false;
    
    return true;
}
static_assert(test_wiresink_validates(), "WireSink validates JSON correctness");

int main() {
    // All tests are static_assert, so if we compile, we pass
    return 0;
}

