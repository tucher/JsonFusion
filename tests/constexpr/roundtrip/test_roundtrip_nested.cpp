#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>
#include <vector>
#include <array>
#include <optional>

using namespace TestHelpers;

// ============================================================================
// Test: Round-Trip Tests for Nested Structures (C++23 constexpr-compatible)
// ============================================================================

// Test 1: Nested structs
struct Inner {
    int value;
    std::string name;
};

struct Outer {
    int id;
    Inner inner;
};

static_assert(
    TestRoundTripSemantic<Outer>(R"({"id": 1, "inner": {"value": 42, "name": "test"}})"),
    "Round-trip: nested structs"
);

// Test 2: Array of structs
struct WithArrayOfStructs {
    std::array<Inner, 3> items;
};

static_assert(
    TestRoundTripSemantic<WithArrayOfStructs>(R"({
        "items": [
            {"value": 1, "name": "first"},
            {"value": 2, "name": "second"},
            {"value": 3, "name": "third"}
        ]
    })"),
    "Round-trip: array of structs"
);

// Test 3: Vector of structs
struct WithVectorOfStructs {
    std::vector<Inner> items;
};

static_assert(
    TestRoundTripSemantic<WithVectorOfStructs>(R"({
        "items": [
            {"value": 1, "name": "a"},
            {"value": 2, "name": "b"}
        ]
    })"),
    "Round-trip: vector of structs"
);

// Test 4: Optional nested struct
struct WithOptionalStruct {
    std::optional<Inner> inner;
};

static_assert(
    TestRoundTripSemantic<WithOptionalStruct>(R"({"inner": {"value": 42, "name": "test"}})"),
    "Round-trip: optional struct (present)"
);

static_assert(
    TestRoundTripSemantic<WithOptionalStruct>(R"({"inner": null})"),
    "Round-trip: optional struct (null)"
);

// Test 5: Nested arrays
struct WithNestedArrays {
    std::array<std::array<int, 2>, 2> matrix;
};

static_assert(
    TestRoundTripSemantic<WithNestedArrays>(R"({"matrix": [[1, 2], [3, 4]]})"),
    "Round-trip: nested arrays"
);

// Test 6: Nested vectors
struct WithNestedVectors {
    std::vector<std::vector<int>> grid;
};

static_assert(
    TestRoundTripSemantic<WithNestedVectors>(R"({"grid": [[1, 2], [3, 4, 5]]})"),
    "Round-trip: nested vectors"
);

// Test 7: Complex nested structure
struct Level2 {
    int id;
    std::string tag;
};

struct Level1 {
    int count;
    std::vector<Level2> children;
};

struct WithComplexNesting {
    std::vector<Level1> levels;
};

static_assert(
    TestRoundTripSemantic<WithComplexNesting>(R"({
        "levels": [
            {
                "count": 2,
                "children": [
                    {"id": 1, "tag": "a"},
                    {"id": 2, "tag": "b"}
                ]
            },
            {
                "count": 1,
                "children": [
                    {"id": 3, "tag": "c"}
                ]
            }
        ]
    })"),
    "Round-trip: complex nested structure"
);

// Test 8: Mixed nested types
struct MixedNested {
    int id;
    std::optional<Inner> optional_inner;
    std::array<int, 3> fixed_array;
    std::vector<std::string> dynamic_array;
};

static_assert(
    TestRoundTripSemantic<MixedNested>(R"({
        "id": 1,
        "optional_inner": {"value": 42, "name": "test"},
        "fixed_array": [1, 2, 3],
        "dynamic_array": ["a", "b", "c"]
    })"),
    "Round-trip: mixed nested types"
);

static_assert(
    TestRoundTripSemantic<MixedNested>(R"({
        "id": 2,
        "optional_inner": null,
        "fixed_array": [10, 20, 30],
        "dynamic_array": []
    })"),
    "Round-trip: mixed nested types with null optional"
);

