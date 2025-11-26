#include "test_helpers.hpp"
#include <string>
using namespace TestHelpers;

struct Config { std::string text; };

// ===== Basic escape sequences =====
// Escaped quote
static_assert(TestParse(R"({"text": "hello\"world"})", Config{"hello\"world"}));

// Escaped backslash
static_assert(TestParse(R"({"text": "path\\file"})", Config{"path\\file"}));

// Escaped forward slash
static_assert(TestParse(R"({"text": "a\/b"})", Config{"a/b"}));

// Backspace
static_assert(TestParse(R"({"text": "a\bc"})", Config{"a\bc"}));

// Form feed
static_assert(TestParse(R"({"text": "a\fc"})", Config{"a\fc"}));

// Newline
static_assert(TestParse(R"({"text": "line1\nline2"})", Config{"line1\nline2"}));

// Carriage return
static_assert(TestParse(R"({"text": "a\rc"})", Config{"a\rc"}));

// Tab
static_assert(TestParse(R"({"text": "a\tb"})", Config{"a\tb"}));

// ===== Unicode escapes (basic ASCII) =====
// \u0041 = 'A'
static_assert(TestParse(R"({"text": "\u0041"})", Config{"A"}));
// \u0042 = 'B', \u0043 = 'C'
static_assert(TestParse(R"({"text": "\u0042\u0043"})", Config{"BC"}));
// \u0020 = space
static_assert(TestParse(R"({"text": "a\u0020b"})", Config{"a b"}));

// ===== Multiple escapes in one string =====
static_assert(TestParse(R"({"text": "a\nb\tc"})", Config{"a\nb\tc"}));
static_assert(TestParse(R"({"text": "\"quoted\""})", Config{"\"quoted\""}));

// ===== Error: Invalid escape sequences =====
static_assert(TestParseError<Config>(R"({"text": "\x"})", JsonFusion::ParseError::UNEXPECTED_SYMBOL));
static_assert(TestParseError<Config>(R"({"text": "\z"})", JsonFusion::ParseError::UNEXPECTED_SYMBOL));

// ===== Error: Incomplete unicode escape =====
static_assert(TestParseError<Config>(R"({"text": "\u"})", JsonFusion::ParseError::UNEXPECTED_SYMBOL));
static_assert(TestParseError<Config>(R"({"text": "\u123"})", JsonFusion::ParseError::UNEXPECTED_SYMBOL));
static_assert(TestParseError<Config>(R"({"text": "\u12"})", JsonFusion::ParseError::UNEXPECTED_SYMBOL));

// ===== Error: Unescaped control characters (RFC 8259 §7 violation) =====
// RFC 8259 §7: Control characters (U+0000 to U+001F) MUST be escaped.
// JsonFusion correctly rejects unescaped control characters to be spec-compliant.

// Unescaped newline (0x0A) - MUST be rejected
static_assert([]() constexpr {
    std::string json = R"({"text": "hello)";
    json += '\n';  // Literal newline byte (illegal per JSON spec)
    json += R"(world"})";
    
    Config obj{};
    auto result = JsonFusion::Parse(obj, json.begin(), json.end());
    return !result;  // MUST fail - unescaped control character
}());

// Unescaped tab (0x09) - MUST be rejected
static_assert([]() constexpr {
    std::string json = R"({"text": "a)";
    json += '\t';  // Literal tab byte (illegal)
    json += R"(b"})";
    
    Config obj{};
    auto result = JsonFusion::Parse(obj, json.begin(), json.end());
    return !result;  // MUST fail
}());

// Unescaped carriage return (0x0D) - MUST be rejected
static_assert([]() constexpr {
    std::string json = R"({"text": "line1)";
    json += '\r';  // Literal CR byte (illegal)
    json += R"(line2"})";
    
    Config obj{};
    auto result = JsonFusion::Parse(obj, json.begin(), json.end());
    return !result;  // MUST fail
}());

// Unescaped backspace (0x08) - MUST be rejected
static_assert([]() constexpr {
    std::string json = R"({"text": "a)";
    json += '\b';  // Literal backspace byte (illegal)
    json += R"(b"})";
    
    Config obj{};
    auto result = JsonFusion::Parse(obj, json.begin(), json.end());
    return !result;  // MUST fail
}());

// Unescaped form feed (0x0C) - MUST be rejected
static_assert([]() constexpr {
    std::string json = R"({"text": "a)";
    json += '\f';  // Literal form feed byte (illegal)
    json += R"(b"})";
    
    Config obj{};
    auto result = JsonFusion::Parse(obj, json.begin(), json.end());
    return !result;  // MUST fail
}());

// Unescaped null byte (0x00) - MUST be rejected
static_assert([]() constexpr {
    std::string json = R"({"text": "a)";
    json += '\0';  // Literal null byte (illegal)
    json += R"(b"})";
    
    Config obj{};
    auto result = JsonFusion::Parse(obj, json.begin(), json.end());
    return !result;  // MUST fail
}());

