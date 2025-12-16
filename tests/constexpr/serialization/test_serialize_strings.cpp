#include "test_helpers.hpp"
using namespace TestHelpers;

// ============================================================================
// Basic String Serialization
// ============================================================================

struct Config_string {
    std::string value;
};

// Empty string
static_assert(TestSerialize(Config_string{""}, R"({"value":""})"));

// Simple string
static_assert(TestSerialize(Config_string{"hello"}, R"({"value":"hello"})"));
static_assert(TestSerialize(Config_string{"Hello, World!"}, R"({"value":"Hello, World!"})"));

// ============================================================================
// Escape Sequences
// ============================================================================

// Quotes
static_assert(TestSerialize(
    Config_string{R"(say "hi")"},
    R"({"value":"say \"hi\""})"
));

// Backslash
static_assert(TestSerialize(
    Config_string{R"(path\to\file)"},
    R"({"value":"path\\to\\file"})"
));

// Newline
static_assert(TestSerialize(
    Config_string{"line1\nline2"},
    R"({"value":"line1\nline2"})"
));

// Tab
static_assert(TestSerialize(
    Config_string{"col1\tcol2"},
    R"({"value":"col1\tcol2"})"
));

// Carriage return
static_assert(TestSerialize(
    Config_string{"line1\rline2"},
    R"({"value":"line1\rline2"})"
));

// Backspace
static_assert(TestSerialize(
    Config_string{"hello\bworld"},
    R"({"value":"hello\bworld"})"
));

// Form feed
static_assert(TestSerialize(
    Config_string{"page1\fpage2"},
    R"({"value":"page1\fpage2"})"
));

// Multiple escape sequences in one string
static_assert(TestSerialize(
    Config_string{"line1\nline2\ttab\r\n\"quoted\""},
    R"({"value":"line1\nline2\ttab\r\n\"quoted\""})"
));

// Backslash followed by quote
static_assert(TestSerialize(
    Config_string{R"(\\\"test\\\")"},
    R"({"value":"\\\\\\\"test\\\\\\\""})"
));

// ============================================================================
// Control Characters (0x00-0x1F)
// ============================================================================

// Control characters must be escaped as \uXXXX per RFC 8259
static_assert(TestSerialize(
    Config_string{"\x01\x02\x03"},
    R"({"value":"\u0001\u0002\u0003"})"
));

static_assert(TestSerialize(
    Config_string{"\x1F"},
    R"({"value":"\u001f"})"
));

// Mix of control chars and regular escapes
static_assert([]() constexpr {
    Config_string cfg;
    cfg.value = std::string("\x00\n\x1F", 3);  // Explicit size for null byte
    std::string result;
    auto r = JsonFusion::Serialize(cfg, result);
    return r && result == R"({"value":"\u0000\n\u001f"})";
}());

// ============================================================================
// Different String Types
// ============================================================================

// std::string (already tested above)

// std::array<char, N>
struct Config_array {
    std::array<char, 20> value;
};

static_assert([]() constexpr {
    Config_array cfg;
    const char* src = "hello";
    for (int i = 0; i < 6; ++i) cfg.value[i] = src[i];  // Include null terminator
    std::string result;
    auto r = JsonFusion::Serialize(cfg, result);
    return r && result == R"({"value":"hello"})";
}());

static_assert([]() constexpr {
    Config_array cfg;
    const char* src = "test\nline";
    for (int i = 0; i < 10; ++i) cfg.value[i] = src[i];
    std::string result;
    auto r = JsonFusion::Serialize(cfg, result);
    return r && result == R"({"value":"test\nline"})";
}());

// ============================================================================
// Multiple String Fields
// ============================================================================

struct MultiString {
    std::string name;
    std::string path;
    std::string message;
};

static_assert(TestSerialize(
    MultiString{"user", "/home/user", "Hello\nWorld"},
    R"({"name":"user","path":"/home/user","message":"Hello\nWorld"})"
));

static_assert(TestSerialize(
    MultiString{"", "", ""},
    R"({"name":"","path":"","message":""})"
));

// ============================================================================
// Roundtrip Tests
// ============================================================================

// Simple strings roundtrip
static_assert(TestRoundTrip<Config_string>(R"({"value":"hello"})", Config_string{"hello"}));
static_assert(TestRoundTrip<Config_string>(R"({"value":""})", Config_string{""}));

// Escaped strings roundtrip
static_assert(TestRoundTrip<Config_string>(R"({"value":"line1\nline2"})", Config_string{"line1\nline2"}));
static_assert(TestRoundTrip<Config_string>(R"({"value":"tab\there"})", Config_string{"tab\there"}));
static_assert(TestRoundTrip<Config_string>(R"({"value":"say \"hi\""})", Config_string{R"(say "hi")"}));
static_assert(TestRoundTrip<Config_string>(R"({"value":"path\\to\\file"})", Config_string{R"(path\to\file)"}));

// Complex escape roundtrip
static_assert(TestRoundTrip<Config_string>(
    R"({"value":"line1\nline2\ttab\r\n\"quoted\""})",
    Config_string{"line1\nline2\ttab\r\n\"quoted\""}
));

// Control characters roundtrip
static_assert(TestRoundTrip<Config_string>(
    R"({"value":"\u0001\u0002\u0003"})",
    Config_string{"\x01\x02\x03"}
));

// Multi-field roundtrip
static_assert(TestRoundTrip<MultiString>(
    R"({"name":"user","path":"/home/user","message":"Hello\nWorld"})",
    MultiString{"user", "/home/user", "Hello\nWorld"}
));

// ============================================================================
// Edge Cases
// ============================================================================

// String with only escapes
static_assert(TestSerialize(
    Config_string{"\n\n\n"},
    R"({"value":"\n\n\n"})"
));

// Very long escape sequence
static_assert(TestSerialize(
    Config_string{"a\nb\nc\nd\ne\nf\ng\nh"},
    R"({"value":"a\nb\nc\nd\ne\nf\ng\nh"})"
));

// All basic escapes in one string
static_assert(TestSerialize(
    Config_string{"\"\\\b\f\n\r\t"},
    R"({"value":"\"\\\b\f\n\r\t"})"
));

