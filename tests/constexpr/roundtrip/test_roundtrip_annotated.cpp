#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/options.hpp>
#include <array>

using namespace TestHelpers;
using namespace JsonFusion;
using namespace JsonFusion::options;

// ============================================================================
// Test: Round-Trip Tests with Annotations (C++23 constexpr-compatible)
// ============================================================================

// Test 1: key<> annotation (JSON key remapping)
struct WithKeyRemap {
    A<int, key<"json_id">> cpp_id;
    std::string name;
};

static_assert(
    TestRoundTripSemantic<WithKeyRemap>(R"({"json_id": 42, "name": "test"})"),
    "Round-trip: key<> annotation"
);

// Test 2: Multiple key<> annotations
struct WithMultipleKeyRemaps {
    A<int, key<"x-coord">> x;
    A<int, key<"y-coord">> y;
    A<std::string, key<"display-name">> name;
};

static_assert(
    TestRoundTripSemantic<WithMultipleKeyRemaps>(R"({
        "x-coord": 10,
        "y-coord": 20,
        "display-name": "point"
    })"),
    "Round-trip: multiple key<> annotations"
);

// Test 3: exclude annotation (field skipped in JSON)
struct WithNotJson {
    int visible;
    A<int, exclude> hidden;
};

static_assert(
    TestRoundTripSemantic<WithNotJson>(R"({"visible": 42})"),
    "Round-trip: exclude annotation (hidden field)"
);

// Test 4: as_array annotation (struct serialized as array)
struct Point {
    int x;
    int y;
};

struct WithAsArray {
    A<Point, as_array> point;
    std::string name;
};

static_assert(
    TestRoundTripSemantic<WithAsArray>(R"({"point": [10, 20], "name": "origin"})"),
    "Round-trip: as_array annotation"
);

// Test 5: Nested structs with key<> annotations
struct InnerWithKey {
    A<int, key<"inner-value">> value;
};

struct OuterWithNestedKey {
    int id;
    InnerWithKey inner;
};

static_assert(
    TestRoundTripSemantic<OuterWithNestedKey>(R"({
        "id": 1,
        "inner": {"inner-value": 42}
    })"),
    "Round-trip: nested structs with key<>"
);

// Test 6: Array of structs with as_array
struct WithArrayOfAsArray {
    std::array<A<Point, as_array>, 2> points;
};

static_assert(
    TestRoundTripSemantic<WithArrayOfAsArray>(R"({
        "points": [[1, 2], [3, 4]]
    })"),
    "Round-trip: array of as_array structs"
);

// Test 7: Mixed annotations
struct WithMixedAnnotations {
    A<int, key<"id">> identifier;
    A<std::string, key<"full-name">> name;
    A<int, exclude> internal_counter;
    A<Point, as_array> position;
};

static_assert(
    TestRoundTripSemantic<WithMixedAnnotations>(R"({
        "id": 1,
        "full-name": "Alice",
        "position": [100, 200]
    })"),
    "Round-trip: mixed annotations (key<>, exclude, as_array)"
);

// Test 8: Nested with multiple annotation types
struct Level2Annotated {
    A<int, key<"val">> value;
    A<int, exclude> hidden;
};

struct Level1Annotated {
    A<int, key<"level1-id">> id;
    Level2Annotated nested;
    A<Point, as_array> point;
};

static_assert(
    TestRoundTripSemantic<Level1Annotated>(R"({
        "level1-id": 10,
        "nested": {"val": 20},
        "point": [5, 6]
    })"),
    "Round-trip: deeply nested with multiple annotation types"
);

