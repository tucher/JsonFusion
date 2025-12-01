#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>

using namespace TestHelpers;

// ============================================================================
// Test: JSON Field Ordering Independence (RFC 8259 Compliance)
// ============================================================================

// Test 1: Fields in different orders than struct definition
struct ThreeFields {
    int a;
    int b;
    int c;
};

static_assert(
    TestParse(R"({"a": 1, "b": 2, "c": 3})", ThreeFields{1, 2, 3}),
    "Field order: same as struct definition"
);

static_assert(
    TestParse(R"({"c": 3, "b": 2, "a": 1})", ThreeFields{1, 2, 3}),
    "Field order: reversed order"
);

static_assert(
    TestParse(R"({"b": 2, "a": 1, "c": 3})", ThreeFields{1, 2, 3}),
    "Field order: middle field first"
);

static_assert(
    TestParse(R"({"a": 1, "c": 3, "b": 2})", ThreeFields{1, 2, 3}),
    "Field order: last two swapped"
);

static_assert(
    TestParse(R"({"c": 3, "a": 1, "b": 2})", ThreeFields{1, 2, 3}),
    "Field order: first and last swapped"
);

static_assert(
    TestParse(R"({"b": 2, "c": 3, "a": 1})", ThreeFields{1, 2, 3}),
    "Field order: all permutations - b, c, a"
);

// Test 2: All permutations for 3-field struct (6 total)
// Already covered above: abc, cba, bac, acb, cab, bca

// Test 3: Fields at different nesting levels
struct Level2 {
    int x;
    int y;
};

struct Level1 {
    int id;
    Level2 nested;
    int z;
};

static_assert(
    TestParse(R"({"id": 1, "nested": {"x": 10, "y": 20}, "z": 3})", 
        Level1{1, {10, 20}, 3}),
    "Nested order: same as struct definition"
);

static_assert(
    TestParse(R"({"z": 3, "nested": {"y": 20, "x": 10}, "id": 1})", 
        Level1{1, {10, 20}, 3}),
    "Nested order: outer fields reversed, inner fields reversed"
);

static_assert(
    TestParse(R"({"nested": {"x": 10, "y": 20}, "id": 1, "z": 3})", 
        Level1{1, {10, 20}, 3}),
    "Nested order: nested field first"
);

static_assert(
    TestParse(R"({"id": 1, "z": 3, "nested": {"y": 20, "x": 10}})", 
        Level1{1, {10, 20}, 3}),
    "Nested order: outer fields swapped, inner fields reversed"
);

// Test 4: Multiple nested levels
struct DeepLevel3 {
    int value;
};

struct DeepLevel2 {
    int id;
    DeepLevel3 deep;
};

struct DeepLevel1 {
    int top;
    DeepLevel2 middle;
    int bottom;
};

static_assert(
    TestParse(R"({
        "top": 1,
        "middle": {
            "id": 2,
            "deep": {"value": 3}
        },
        "bottom": 4
    })", 
        DeepLevel1{1, {2, {3}}, 4}),
    "Deep nesting: same order as struct"
);

static_assert(
    TestParse(R"({
        "bottom": 4,
        "middle": {
            "deep": {"value": 3},
            "id": 2
        },
        "top": 1
    })", 
        DeepLevel1{1, {2, {3}}, 4}),
    "Deep nesting: all levels reversed"
);

// Test 5: Mixed types (primitives, strings, arrays)
struct MixedTypes {
    int number;
    std::string text;
    bool flag;
};

static_assert(
    TestParse(R"({"number": 42, "text": "hello", "flag": true})", 
        MixedTypes{42, "hello", true}),
    "Mixed types: same order"
);

static_assert(
    TestParse(R"({"flag": true, "text": "hello", "number": 42})", 
        MixedTypes{42, "hello", true}),
    "Mixed types: reversed order"
);

static_assert(
    TestParse(R"({"text": "hello", "number": 42, "flag": true})", 
        MixedTypes{42, "hello", true}),
    "Mixed types: string first"
);

// Test 6: With arrays
struct WithArrayField {
    int id;
    std::array<int, 3> values;
    std::string name;
};

static_assert(
    TestParse(R"({"id": 1, "values": [10, 20, 30], "name": "test"})", 
        WithArrayField{1, {10, 20, 30}, "test"}),
    "With array: same order"
);

static_assert(
    TestParse(R"({"name": "test", "values": [10, 20, 30], "id": 1})", 
        WithArrayField{1, {10, 20, 30}, "test"}),
    "With array: different order"
);

// Test 7: With optional fields
struct WithOptional {
    int required;
    std::optional<int> opt1;
    std::optional<std::string> opt2;
};

static_assert(
    TestParse(R"({"required": 1, "opt1": 2, "opt2": "test"})", 
        WithOptional{1, 2, "test"}),
    "With optional: all present, same order"
);

static_assert(
    TestParse(R"({"opt2": "test", "required": 1, "opt1": 2})", 
        WithOptional{1, 2, "test"}),
    "With optional: all present, different order"
);

static_assert(
    TestParse(R"({"opt1": 2, "required": 1})", 
        WithOptional{1, 2, std::nullopt}),
    "With optional: some present, different order"
);

