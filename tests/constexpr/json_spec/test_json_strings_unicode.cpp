#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <string>
#include <array>

using namespace TestHelpers;
using JsonFusion::ParseError;

// ============================================================================
// Test: JSON Unicode String Handling (RFC 8259 Compliance)
// ============================================================================

struct WithString {
    std::string text;
};

// Test 1: Basic ASCII via Unicode escapes
static_assert(
    TestParse(R"({"text": "\u0041"})", WithString{"A"}),
    "Unicode: \u0041 = 'A'"
);

static_assert(
    TestParse(R"({"text": "\u0042\u0043"})", WithString{"BC"}),
    "Unicode: \u0042\u0043 = 'BC'"
);

static_assert(
    TestParse(R"({"text": "\u0020"})", WithString{" "}),
    "Unicode: \u0020 = space"
);

static_assert(
    TestParse(R"({"text": "a\u0020b"})", WithString{"a b"}),
    "Unicode: space in middle of string"
);

// Test 2: Unicode escapes for common characters
static_assert(
    TestParse(R"({"text": "\u00E9"})", WithString{"é"}),
    "Unicode: \u00E9 = 'é' (Latin small e with acute)"
);

static_assert(
    TestParse(R"({"text": "\u00F1"})", WithString{"ñ"}),
    "Unicode: \u00F1 = 'ñ' (Latin small n with tilde)"
);

static_assert(
    TestParse(R"({"text": "\u03B1"})", WithString{"α"}),
    "Unicode: \u03B1 = 'α' (Greek alpha)"
);

static_assert(
    TestParse(R"({"text": "\u4E2D"})", WithString{"中"}),
    "Unicode: \u4E2D = '中' (Chinese character)"
);

// Test 3: Surrogate pairs (characters beyond BMP)
// U+1F600 = grinning face = high surrogate D83D + low surrogate DE00
static_assert(
    TestParse<WithString>(R"({"text": "\uD83D\uDE00"})", 
        [](const WithString& obj) {
            // Check that emoji was correctly decoded (UTF-8: F0 9F 98 80)
            return obj.text.size() == 4 
                && static_cast<unsigned char>(obj.text[0]) == 0xF0
                && static_cast<unsigned char>(obj.text[1]) == 0x9F
                && static_cast<unsigned char>(obj.text[2]) == 0x98
                && static_cast<unsigned char>(obj.text[3]) == 0x80;
        }),
    "Unicode: surrogate pair D83D+DE00 = emoji"
);

// U+1F44D = thumbs up = high surrogate D83D + low surrogate DC4D
static_assert(
    TestParse<WithString>(R"({"text": "\uD83D\uDC4D"})", 
        [](const WithString& obj) {
            // Check UTF-8 encoding: F0 9F 91 8D
            return obj.text.size() == 4
                && static_cast<unsigned char>(obj.text[0]) == 0xF0
                && static_cast<unsigned char>(obj.text[1]) == 0x9F
                && static_cast<unsigned char>(obj.text[2]) == 0x91
                && static_cast<unsigned char>(obj.text[3]) == 0x8D;
        }),
    "Unicode: surrogate pair D83D+DC4D = thumbs up emoji"
);

// Test 4: Mixed Unicode escapes and regular characters
static_assert(
    TestParse(R"({"text": "Hello\u0020World"})", WithString{"Hello World"}),
    "Unicode: mixed regular and escape"
);

static_assert(
    TestParse(R"({"text": "Price: \u20AC100"})", WithString{"Price: €100"}),
    "Unicode: \u20AC = € (Euro sign)"
);

// Test 5: Multiple Unicode escapes in sequence
static_assert(
    TestParse(R"({"text": "\u0048\u0065\u006C\u006C\u006F"})", WithString{"Hello"}),
    "Unicode: 'Hello' spelled with escapes"
);

// Test 6: Unicode escapes in char arrays
struct WithCharArray {
    std::array<char, 32> text;
};

static_assert(
    TestParse(R"({"text": "\u0041\u0042"})", WithCharArray{"AB"}),
    "Unicode: escapes in char array"
);

// Test 7: Error cases - Invalid Unicode escapes
static_assert(
    TestParseError<WithString>(R"({"text": "\u"})", JsonFusion::JsonIteratorReaderError::ILLFORMED_STRING),
    "Unicode error: incomplete escape (no hex digits)"
);

static_assert(
    TestParseError<WithString>(R"({"text": "\u123"})", JsonFusion::JsonIteratorReaderError::ILLFORMED_STRING),
    "Unicode error: incomplete escape (3 hex digits)"
);

static_assert(
    TestParseError<WithString>(R"({"text": "\u12"})", JsonFusion::JsonIteratorReaderError::ILLFORMED_STRING),
    "Unicode error: incomplete escape (2 hex digits)"
);

static_assert(
    TestParseError<WithString>(R"({"text": "\u1"})", JsonFusion::JsonIteratorReaderError::ILLFORMED_STRING),
    "Unicode error: incomplete escape (1 hex digit)"
);

// Test 8: Error cases - Invalid surrogate pairs
// Lone low surrogate (invalid)
static_assert(
    TestParseError<WithString>(R"({"text": "\uDC00"})", JsonFusion::JsonIteratorReaderError::ILLFORMED_STRING),
    "Unicode error: lone low surrogate DC00"
);

// High surrogate without low surrogate
static_assert(
    TestParseError<WithString>(R"({"text": "\uD83D"})", JsonFusion::JsonIteratorReaderError::ILLFORMED_STRING),
    "Unicode error: high surrogate without low surrogate"
);

// High surrogate followed by non-surrogate
static_assert(
    TestParseError<WithString>(R"({"text": "\uD83D\u0041"})", JsonFusion::JsonIteratorReaderError::ILLFORMED_STRING),
    "Unicode error: high surrogate followed by non-surrogate"
);

// High surrogate not followed by backslash-u
static_assert(
    TestParseError<WithString>(R"({"text": "\uD83Dx"})", JsonFusion::JsonIteratorReaderError::ILLFORMED_STRING),
    "Unicode error: high surrogate not followed by escape sequence"
);

// Test 9: All required escape sequences from RFC 8259
static_assert(
    TestParse(R"({"text": "\"\\\/\b\f\n\r\t"})", 
        WithString{"\"\\/\b\f\n\r\t"}),
    "Unicode: all RFC 8259 escape sequences"
);

// Test 10: Unicode escapes for control characters (must be escaped)
// \u000A = newline (must be escaped, not literal)
static_assert(
    TestParse(R"({"text": "\u000A"})", WithString{"\n"}),
    "Unicode: \u000A = newline (escaped)"
);

// \u0009 = tab (must be escaped, not literal)
static_assert(
    TestParse(R"({"text": "\u0009"})", WithString{"\t"}),
    "Unicode: \u0009 = tab (escaped)"
);

// \u000D = carriage return (must be escaped, not literal)
static_assert(
    TestParse(R"({"text": "\u000D"})", WithString{"\r"}),
    "Unicode: \u000D = carriage return (escaped)"
);

// Test 11: Unicode in nested structures
struct Nested {
    struct Inner {
        std::string name;
    } inner;
};

static_assert(
    TestParse(R"({"inner": {"name": "\u4E2D\u6587"}})", 
        Nested{{"中文"}}),
    "Unicode: in nested structure"
);

// Test 12: Unicode in arrays
struct WithStringArray {
    std::array<std::string, 2> texts;
};

static_assert(
    TestParse(R"({"texts": ["\u0041", "\u0042"]})", 
        WithStringArray{{"A", "B"}}),
    "Unicode: in array elements"
);

// Test 13: Long strings with Unicode
static_assert(
    TestParse(R"({"text": "Hello\u0020\u0057\u006F\u0072\u006C\u0064"})", 
        WithString{"Hello World"}),
    "Unicode: long string with multiple escapes"
);

// Test 14: Unicode boundary values
// U+0000 (null) - testing behavior
// Note: JsonFusion may handle this differently, testing actual behavior
// static_assert(
//     TestParseError<WithString>(R"({"text": "\u0000"})", JsonFusion::JsonIteratorReaderError::ILLFORMED_STRING),
//     "Unicode: U+0000 (null) should be rejected"
// );

// U+007F (DEL) - control character, should be rejected if unescaped
// But \u007F as escape should work
static_assert(
    TestParse(R"({"text": "\u007F"})", WithString{"\x7F"}),
    "Unicode: \u007F = DEL (escaped)"
);

// Test 15: Mixed escape types
static_assert(
    TestParse(R"({"text": "Line1\nLine2\u0020Tab\t"})", 
        WithString{"Line1\nLine2 Tab\t"}),
    "Unicode: mixed \\n escape and \u0020 Unicode escape"
);

