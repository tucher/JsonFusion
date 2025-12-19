#include "test_helpers.hpp"
#include <array>
using namespace TestHelpers;

struct Config { std::array<char, 32> text; };
struct SmallConfig { std::array<char, 4> text; };
struct LargeConfig { std::array<char, 128> text; };

// ===== Empty string =====
static_assert(TestParse(R"({"text": ""})", Config{{'\0'}}));

// ===== Single character =====
static_assert(TestParse(R"({"text": "x"})", Config{{'x','\0'}}));

// ===== Whitespace in strings =====
static_assert(TestParse(R"({"text": "   "})", Config{{' ',' ',' ','\0'}}));
static_assert(TestParse(R"({"text": " a "})", Config{{' ','a',' ','\0'}}));

// ===== String exactly filling buffer (with null terminator) =====
// SmallConfig has array<char, 4>. "abc" = 3 chars + null terminator = 4 total
static_assert(TestParse(R"({"text": "abc"})", SmallConfig{{'a','b','c','\0'}}));

// ===== Error: String too long for buffer =====
// "abcd" needs 5 chars (4 + null), SmallConfig only has 4
static_assert(TestParseError<SmallConfig>(R"({"text": "abcd"})", JsonFusion::ParseError::FIXED_SIZE_CONTAINER_OVERFLOW));
static_assert(TestParseError<SmallConfig>(R"({"text": "toolong"})", JsonFusion::ParseError::FIXED_SIZE_CONTAINER_OVERFLOW));

// ===== Long valid string =====
static_assert(TestParse(R"({"text": "This is a longer string to test"})", Config{{'T','h','i','s',' ','i','s',' ','a',' ','l','o','n','g','e','r',' ','s','t','r','i','n','g',' ','t','o',' ','t','e','s','t','\0'}}));

// ===== Special characters (high ASCII) =====
static_assert(TestParse(R"({"text": "test123!@#"})", Config{{'t','e','s','t','1','2','3','!','@','#','\0'}}));

// ===== Error: Unclosed string =====
static_assert(TestParseError<Config>(R"({"text": "unclosed})", JsonFusion::JsonIteratorReaderError::UNEXPECTED_END_OF_DATA));
static_assert(TestParseError<Config>(R"({"text": "unclosed)", JsonFusion::JsonIteratorReaderError::UNEXPECTED_END_OF_DATA));

// ===== Error: Missing quotes =====
static_assert(TestParseError<Config>(R"({"text": hello})", JsonFusion::ParseError::NON_STRING_IN_STRING_STORAGE));

// ===== Strings with quotes and backslashes =====
static_assert(TestParse(R"({"text": "a\"b"})", Config{{'a','"','b','\0'}}));
static_assert(TestParse(R"({"text": "a\\b"})", Config{{'a','\\','b','\0'}}));

// ===== Consecutive strings (separate fields) =====
struct MultiString {
    std::array<char, 16> first;
    std::array<char, 16> second;
};
static_assert(TestParse(R"({"first": "hello", "second": "world"})", 
    MultiString{{'h','e','l','l','o','\0'}, {'w','o','r','l','d','\0'}}));

// ===== Empty vs whitespace =====
static_assert(TestParse(R"({"text": ""})", Config{{'\0'}}));
static_assert(TestParse(R"({"text": " "})", Config{{' ','\0'}}));

