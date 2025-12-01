#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>

using namespace TestHelpers;

// ============================================================================
// Test: Round-Trip Tests for Primitive Types (C++23 constexpr-compatible)
// ============================================================================

// Test 1: Integer round-trip
struct WithInt {
    int value;
};

static_assert(
    TestRoundTripSemantic<WithInt>(R"({"value": 42})"),
    "Round-trip: integer"
);

static_assert(
    TestRoundTripSemantic<WithInt>(R"({"value": -123})"),
    "Round-trip: negative integer"
);

static_assert(
    TestRoundTripSemantic<WithInt>(R"({"value": 0})"),
    "Round-trip: zero"
);

static_assert(
    TestRoundTripSemantic<WithInt>(R"({"value": 2147483647})"),  // INT_MAX
    "Round-trip: INT_MAX"
);

static_assert(
    TestRoundTripSemantic<WithInt>(R"({"value": -2147483648})"),  // INT_MIN
    "Round-trip: INT_MIN"
);

// Test 2: Boolean round-trip
struct WithBool {
    bool flag;
};

static_assert(
    TestRoundTripSemantic<WithBool>(R"({"flag": true})"),
    "Round-trip: boolean true"
);

static_assert(
    TestRoundTripSemantic<WithBool>(R"({"flag": false})"),
    "Round-trip: boolean false"
);

// Test 3: Multiple primitives in same struct
struct WithMultiplePrimitives {
    int x;
    bool y;
    int z;
};

static_assert(
    TestRoundTripSemantic<WithMultiplePrimitives>(R"({"x": 1, "y": true, "z": 42})"),
    "Round-trip: multiple primitives"
);

// Test 4: Char array round-trip
struct WithCharArray {
    std::array<char, 32> text;
};

static_assert(
    TestRoundTripSemantic<WithCharArray>(R"({"text": "hello"})"),
    "Round-trip: char array"
);

static_assert(
    TestRoundTripSemantic<WithCharArray>(R"({"text": ""})"),
    "Round-trip: empty char array"
);

static_assert(
    TestRoundTripSemantic<WithCharArray>(R"({"text": "Hello\nWorld"})"),
    "Round-trip: char array with escape sequences"
);

// Test 5: String round-trip
struct WithString {
    std::string text;
};

static_assert(
    TestRoundTripSemantic<WithString>(R"({"text": "hello"})"),
    "Round-trip: string"
);

static_assert(
    TestRoundTripSemantic<WithString>(R"({"text": ""})"),
    "Round-trip: empty string"
);

static_assert(
    TestRoundTripSemantic<WithString>(R"({"text": "This is a longer string"})"),
    "Round-trip: longer string"
);

