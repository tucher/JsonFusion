#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>

using namespace TestHelpers;
using JsonFusion::ParseError;

// ============================================================================
// Test: Invalid JSON Syntax (RFC 8259 Compliance - Error Detection)
// ============================================================================

struct Simple {
    int value;
};

struct TwoFields {
    int x;
    int y;
};

// Test 1: Missing closing brace
static_assert(
    TestParseError<Simple>(R"({"value": 1)", ParseError::UNEXPECTED_END_OF_DATA),
    "Invalid: missing closing brace"
);

static_assert(
    TestParseError<TwoFields>(R"({"x": 1, "y": 2)", ParseError::UNEXPECTED_END_OF_DATA),
    "Invalid: missing closing brace (multiple fields)"
);

// Test 2: Missing closing bracket
struct WithArray {
    std::array<int, 3> values;
};

static_assert(
    TestParseError<WithArray>(R"({"values": [1, 2, 3)", ParseError::UNEXPECTED_END_OF_DATA),
    "Invalid: missing closing bracket"
);

static_assert(
    TestParseError<WithArray>(R"({"values": [1, 2)", ParseError::UNEXPECTED_END_OF_DATA),
    "Invalid: missing closing bracket (incomplete array)"
);

// Test 3: Missing comma
static_assert(
    TestParseError<TwoFields>(R"({"x": 1 "y": 2})", ParseError::ILLFORMED_OBJECT),
    "Invalid: missing comma between fields"
);

struct ThreeFields {
    int a;
    int b;
    int c;
};

static_assert(
    TestParseError<ThreeFields>(R"({"a": 1, "b": 2 "c": 3})", ParseError::ILLFORMED_OBJECT),
    "Invalid: missing comma (middle field)"
);

// Test 4: Extra comma (trailing comma) - RFC 8259 does NOT allow this
static_assert(
    TestParseError<Simple>(R"({"value": 1,})", ParseError::ILLFORMED_OBJECT),
    "Invalid: trailing comma in object"
);

static_assert(
    TestParseError<WithArray>(R"({"values": [1, 2, 3,]})", ParseError::ILLFORMED_ARRAY),
    "Invalid: trailing comma in array"
);

// Test 5: Missing colon
static_assert(
    TestParseError<TwoFields>(R"({"x" 1, "y": 2})", ParseError::ILLFORMED_OBJECT),
    "Invalid: missing colon"
);

static_assert(
    TestParseError<Simple>(R"({"value" 42})", ParseError::ILLFORMED_OBJECT),
    "Invalid: missing colon (single field)"
);

// Test 6: Double colon
static_assert(
    TestParseError<Simple>(R"({"value":: 42})", ParseError::ILLFORMED_NUMBER),
    "Invalid: double colon"
);

// Test 7: Unquoted keys (not valid JSON)
static_assert(
    TestParseError<Simple>(R"({value: 42})", ParseError::ILLFORMED_OBJECT),
    "Invalid: unquoted key"
);

static_assert(
    TestParseError<TwoFields>(R"({x: 1, y: 2})", ParseError::ILLFORMED_OBJECT),
    "Invalid: unquoted keys"
);

// Test 8: Single quotes (not valid JSON)
static_assert(
    TestParseError<Simple>(R"({'value': 42})", ParseError::ILLFORMED_OBJECT),
    "Invalid: single quotes for key"
);

static_assert(
    TestParseError<Simple>(R"({"value": 'test'})", ParseError::ILLFORMED_NUMBER),
    "Invalid: single quotes for string value"
);

// Test 9: Truncated JSON
static_assert(
    TestParseError<Simple>(R"({"value":)", ParseError::UNEXPECTED_END_OF_DATA),
    "Invalid: truncated after colon"
);

static_assert(
    TestParseError<Simple>(R"({"value": 4)", ParseError::UNEXPECTED_END_OF_DATA),
    "Invalid: truncated number"
);

static_assert(
    TestParseError<Simple>(R"({"value": "test)", ParseError::ILLFORMED_NUMBER),
    "Invalid: unclosed string"
);

// Test 10: Invalid escape sequences
struct WithString {
    std::string text;
};

static_assert(
    TestParseError<WithString>(R"({"text": "test\x"})", ParseError::ILLFORMED_STRING),
    "Invalid: incomplete escape sequence \\x"
);

static_assert(
    TestParseError<WithString>(R"({"text": "test\u12"})", ParseError::ILLFORMED_STRING),
    "Invalid: incomplete Unicode escape \\u12"
);

// Test 11: Invalid number formats (for constexpr, integer-only)
static_assert(
    TestParseError<Simple>(R"({"value": 01})", ParseError::ILLFORMED_NUMBER),
    "Invalid: leading zero (01)"
);

static_assert(
    TestParseError<Simple>(R"({"value": 00})", ParseError::ILLFORMED_NUMBER),
    "Invalid: leading zero (00)"
);

// Test 12: Invalid boolean
struct WithBool {
    bool flag;
};

static_assert(
    TestParseError<WithBool>(R"({"flag": True})", ParseError::NON_BOOL_JSON_IN_BOOL_VALUE),
    "Invalid: capitalized True (should be lowercase)"
);

static_assert(
    TestParseError<WithBool>(R"({"flag": TRUE})", ParseError::NON_BOOL_JSON_IN_BOOL_VALUE),
    "Invalid: uppercase TRUE"
);

static_assert(
    TestParseError<WithBool>(R"({"flag": truee})", ParseError::ILLFORMED_BOOL),
    "Invalid: typo in true (truee)"
);

static_assert(
    TestParseError<WithBool>(R"({"flag": fals})", ParseError::ILLFORMED_BOOL),
    "Invalid: incomplete false (fals)"
);

// Test 13: Invalid null
struct WithOptional {
    std::optional<int> value;
};

static_assert(
    TestParseError<WithOptional>(R"({"value": Null})", ParseError::ILLFORMED_NUMBER),
    "Invalid: capitalized Null"
);

static_assert(
    TestParseError<WithOptional>(R"({"value": NULL})", ParseError::ILLFORMED_NUMBER),
    "Invalid: uppercase NULL"
);

static_assert(
    TestParseError<WithOptional>(R"({"value": nul})", ParseError::ILLFORMED_NULL),
    "Invalid: incomplete null"
);

// Test 14: Mismatched brackets/braces
static_assert(
    TestParseError<WithArray>(R"({"values": [1, 2, 3})", ParseError::ILLFORMED_ARRAY),
    "Invalid: array closed with brace"
);

static_assert(
    TestParseError<Simple>(R"({"value": 42])", ParseError::ILLFORMED_OBJECT),
    "Invalid: object closed with bracket"
);

// Test 15: Nested structure errors
struct Nested {
    struct Inner {
        int x;
    } inner;
};

static_assert(
    TestParseError<Nested>(R"({"inner": {"x": 1)", ParseError::UNEXPECTED_END_OF_DATA),
    "Invalid: missing closing brace in nested object"
);

static_assert(
    TestParseError<Nested>(R"({"inner": {"x": 1,})", ParseError::ILLFORMED_OBJECT),
    "Invalid: trailing comma in nested object"
);

// Test 16: Array errors

static_assert(
    TestParseError<WithArray>(R"({"values": [1, 2, })", ParseError::ILLFORMED_NUMBER),
    "Invalid: trailing comma in array"
);

static_assert(
    TestParseError<WithArray>(R"({"values": [1 2, 3]})", ParseError::ILLFORMED_ARRAY),
    "Invalid: missing comma in array"
);

// Test 17: Empty key (edge case - might be valid or invalid depending on implementation)
// Note: RFC 8259 allows empty strings as keys, so this might be valid
// Leaving this out for now as it depends on JsonFusion's behavior

// Test 18: Control characters in strings (should be escaped)
static_assert(
    TestParseError<WithString>(R"({"text": "test
newline"})", ParseError::ILLFORMED_STRING),
    "Invalid: unescaped newline in string"
);

