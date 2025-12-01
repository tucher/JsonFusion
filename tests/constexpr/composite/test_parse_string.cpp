#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <string>

using namespace TestHelpers;

// ============================================================================
// Test: std::string Parsing (C++23 constexpr-compatible)
// ============================================================================

// Test 1: std::string with various lengths
struct WithString {
    std::string text;
};

static_assert(
    TestParse(R"({"text": "hello"})", 
        WithString{"hello"}),
    "String: short text"
);

static_assert(
    TestParse(R"({"text": "a"})", 
        WithString{"a"}),
    "String: single character"
);

static_assert(
    TestParse(R"({"text": "This is a longer string with multiple words"})", 
        WithString{"This is a longer string with multiple words"}),
    "String: longer text"
);

// Test 2: Empty string
static_assert(
    TestParse(R"({"text": ""})", 
        WithString{""}),
    "String: empty string"
);

// Test 3: Strings with special characters
static_assert(
    TestParse(R"({"text": "Hello\nWorld\tTab"})", 
        WithString{"Hello\nWorld\tTab"}),
    "String: with escape sequences"
);

static_assert(
    TestParse(R"({"text": "Quote: \"test\""})", 
        WithString{"Quote: \"test\""}),
    "String: with escaped quotes"
);

// Test 4: Multiple strings in same struct
struct WithMultipleStrings {
    std::string first;
    std::string second;
    std::string third;
};

static_assert(
    TestParse(R"({
        "first": "one",
        "second": "two",
        "third": "three"
    })", 
        WithMultipleStrings{"one", "two", "three"}),
    "Multiple strings in same struct"
);

// Test 5: String in nested struct
struct Outer {
    int id;
    struct Inner {
        std::string name;
    } inner;
};

static_assert(
    TestParse(R"({"id": 1, "inner": {"name": "Alice"}})", 
        Outer{1, {"Alice"}}),
    "String in nested struct"
);

// Test 6: String with optional
struct WithOptionalString {
    std::optional<std::string> name;
};

static_assert(
    TestParse(R"({"name": "Bob"})", 
        WithOptionalString{"Bob"}),
    "Optional string: present"
);

static_assert(
    TestParse(R"({"name": null})", 
        WithOptionalString{std::nullopt}),
    "Optional string: null"
);

