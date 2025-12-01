#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>

using namespace TestHelpers;

// ============================================================================
// Test: JSON Whitespace Handling (RFC 8259 Compliance)
// ============================================================================

// Test 1: Leading/trailing whitespace in document
struct Simple {
    int value;
};

static_assert(
    TestParse(R"(  {"value": 42}  )", Simple{42}),
    "Whitespace: leading and trailing spaces"
);

static_assert(
    TestParse(R"(
{"value": 42}
)", Simple{42}),
    "Whitespace: leading and trailing newlines"
);

static_assert(
    TestParse(R"(	{"value": 42}	)", Simple{42}),
    "Whitespace: leading and trailing tabs"
);

// Test 2: Whitespace between keys and colons
struct WithString {
    std::string name;
    int count;
};

static_assert(
    TestParse(R"({"name" : "test", "count" : 42})", 
        WithString{"test", 42}),
    "Whitespace: between keys and colons"
);

static_assert(
    TestParse(R"({"name": "test", "count": 42})", 
        WithString{"test", 42}),
    "Whitespace: no whitespace (compact JSON)"
);

// Test 3: Whitespace around commas
struct MultipleFields {
    int a;
    int b;
    int c;
};

static_assert(
    TestParse(R"({"a": 1 , "b": 2 , "c": 3})", 
        MultipleFields{1, 2, 3}),
    "Whitespace: before commas"
);

static_assert(
    TestParse(R"({"a": 1, "b": 2, "c": 3})", 
        MultipleFields{1, 2, 3}),
    "Whitespace: no whitespace around commas (compact)"
);

static_assert(
    TestParse(R"({"a": 1  ,  "b": 2  ,  "c": 3})", 
        MultipleFields{1, 2, 3}),
    "Whitespace: multiple spaces around commas"
);

// Test 4: Tabs, newlines, carriage returns
static_assert(
    TestParse(R"({
	"a": 1,
	"b": 2,
	"c": 3
})", 
        MultipleFields{1, 2, 3}),
    "Whitespace: tabs and newlines"
);

static_assert(
    TestParse(R"({"value": 42})", Simple{42}),
    "Whitespace: no whitespace (compact JSON)"
);

// Test 5: Whitespace in nested structures
struct Inner {
    int x;
    int y;
};

struct Outer {
    Inner inner;
    int z;
};

static_assert(
    TestParse(R"({
	"inner": {
		"x": 1,
		"y": 2
	},
	"z": 3
})", 
        Outer{{1, 2}, 3}),
    "Whitespace: nested structures with tabs/newlines"
);

// Test 6: Whitespace in arrays
struct WithArray {
    std::array<int, 3> values;
};

static_assert(
    TestParse(R"({"values": [ 1 , 2 , 3 ]})", 
        WithArray{{1, 2, 3}}),
    "Whitespace: spaces around array elements"
);

static_assert(
    TestParse(R"({"values": [1, 2, 3]})", 
        WithArray{{1, 2, 3}}),
    "Whitespace: minimal whitespace in arrays"
);

static_assert(
    TestParse(R"({
	"values": [
		1,
		2,
		3
	]
})", 
        WithArray{{1, 2, 3}}),
    "Whitespace: newlines in arrays"
);

// Test 7: Multiple consecutive spaces
static_assert(
    TestParse(R"(    {"value": 42}    )", Simple{42}),
    "Whitespace: multiple consecutive spaces"
);

static_assert(
    TestParse(R"({"a": 1  ,  "b": 2  ,  "c": 3})", 
        MultipleFields{1, 2, 3}),
    "Whitespace: multiple spaces around commas"
);

// Test 8: Mixed whitespace characters
static_assert(
    TestParse(R"({
 "a": 1	,
 "b": 2	,
 "c": 3
})", 
        MultipleFields{1, 2, 3}),
    "Whitespace: mixed spaces and tabs"
);

// Test 9: Whitespace with strings containing spaces
struct WithSpacedString {
    std::string text;
};

static_assert(
    TestParse(R"({"text": "hello world"})", 
        WithSpacedString{"hello world"}),
    "Whitespace: string content with spaces (not structural whitespace)"
);

static_assert(
    TestParse(R"(  {  "text"  :  "hello world"  }  )", 
        WithSpacedString{"hello world"}),
    "Whitespace: structural whitespace around string field"
);

