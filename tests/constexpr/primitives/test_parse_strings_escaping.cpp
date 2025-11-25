#include "test_helpers.hpp"
#include <array>
using namespace TestHelpers;

struct Config { std::array<char, 32> text; };

// ===== Basic escape sequences =====
// Escaped quote
static_assert(TestParse(R"({"text": "hello\"world"})", Config{{'h','e','l','l','o','"','w','o','r','l','d','\0'}}));

// Escaped backslash
static_assert(TestParse(R"({"text": "path\\file"})", Config{{'p','a','t','h','\\','f','i','l','e','\0'}}));

// Escaped forward slash
static_assert(TestParse(R"({"text": "a\/b"})", Config{{'a','/','b','\0'}}));

// Backspace
static_assert(TestParse(R"({"text": "a\bc"})", Config{{'a','\b','c','\0'}}));

// Form feed
static_assert(TestParse(R"({"text": "a\fc"})", Config{{'a','\f','c','\0'}}));

// Newline
static_assert(TestParse(R"({"text": "line1\nline2"})", Config{{'l','i','n','e','1','\n','l','i','n','e','2','\0'}}));

// Carriage return
static_assert(TestParse(R"({"text": "a\rc"})", Config{{'a','\r','c','\0'}}));

// Tab
static_assert(TestParse(R"({"text": "a\tb"})", Config{{'a','\t','b','\0'}}));

// ===== Unicode escapes (basic ASCII) =====
// \u0041 = 'A'
static_assert(TestParse(R"({"text": "\u0041"})", Config{{'A','\0'}}));
// \u0042 = 'B', \u0043 = 'C'
static_assert(TestParse(R"({"text": "\u0042\u0043"})", Config{{'B','C','\0'}}));
// \u0020 = space
static_assert(TestParse(R"({"text": "a\u0020b"})", Config{{'a',' ','b','\0'}}));

// ===== Multiple escapes in one string =====
static_assert(TestParse(R"({"text": "a\nb\tc"})", Config{{'a','\n','b','\t','c','\0'}}));
static_assert(TestParse(R"({"text": "\"quoted\""})", Config{{'"','q','u','o','t','e','d','"','\0'}}));

// ===== Error: Invalid escape sequences =====
static_assert(TestParseError<Config>(R"({"text": "\x"})", JsonFusion::ParseError::UNEXPECTED_SYMBOL));
static_assert(TestParseError<Config>(R"({"text": "\z"})", JsonFusion::ParseError::UNEXPECTED_SYMBOL));

// ===== Error: Incomplete unicode escape =====
static_assert(TestParseError<Config>(R"({"text": "\u"})", JsonFusion::ParseError::UNEXPECTED_SYMBOL));
static_assert(TestParseError<Config>(R"({"text": "\u123"})", JsonFusion::ParseError::UNEXPECTED_SYMBOL));
static_assert(TestParseError<Config>(R"({"text": "\u12"})", JsonFusion::ParseError::UNEXPECTED_SYMBOL));

// ===== Error: Unescaped control characters =====
// Actual newline in string (not \n) should fail
// Note: Can't easily test this in constexpr without literal newlines

