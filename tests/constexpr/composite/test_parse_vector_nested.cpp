#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <vector>

using namespace TestHelpers;

// ============================================================================
// Test: std::vector<T> with Nested Types (C++23 constexpr-compatible)
// ============================================================================

// Test 1: std::vector of nested structs
struct Inner {
    int value;
    std::string name;
};

struct WithVectorOfStructs {
    std::vector<Inner> items;
};

static_assert(
    TestParse<WithVectorOfStructs>(R"({
        "items": [
            {"value": 1, "name": "first"},
            {"value": 2, "name": "second"},
            {"value": 3, "name": "third"}
        ]
    })", 
        [](const WithVectorOfStructs& obj) {
            return obj.items.size() == 3
                && obj.items[0].value == 1 && obj.items[0].name == "first"
                && obj.items[1].value == 2 && obj.items[1].name == "second"
                && obj.items[2].value == 3 && obj.items[2].name == "third";
        }),
    "Vector of nested structs"
);

static_assert(
    TestParse<WithVectorOfStructs>(R"({"items": []})", 
        [](const WithVectorOfStructs& obj) {
            return obj.items.size() == 0;
        }),
    "Vector of structs: empty"
);

// Test 2: std::vector of arrays
struct WithVectorOfArrays {
    std::vector<std::array<int, 3>> matrix;
};

static_assert(
    TestParse<WithVectorOfArrays>(R"({
        "matrix": [
            [1, 2, 3],
            [4, 5, 6],
            [7, 8, 9]
        ]
    })", 
        [](const WithVectorOfArrays& obj) {
            return obj.matrix.size() == 3
                && obj.matrix[0][0] == 1 && obj.matrix[0][1] == 2 && obj.matrix[0][2] == 3
                && obj.matrix[1][0] == 4 && obj.matrix[1][1] == 5 && obj.matrix[1][2] == 6
                && obj.matrix[2][0] == 7 && obj.matrix[2][1] == 8 && obj.matrix[2][2] == 9;
        }),
    "Vector of arrays"
);

// Test 3: std::vector of vectors (nested vectors)
struct WithNestedVectors {
    std::vector<std::vector<int>> grid;
};

static_assert(
    TestParse<WithNestedVectors>(R"({
        "grid": [
            [1, 2],
            [3, 4, 5],
            [6]
        ]
    })", 
        [](const WithNestedVectors& obj) {
            return obj.grid.size() == 3
                && obj.grid[0].size() == 2 && obj.grid[0][0] == 1 && obj.grid[0][1] == 2
                && obj.grid[1].size() == 3 && obj.grid[1][0] == 3 && obj.grid[1][1] == 4 && obj.grid[1][2] == 5
                && obj.grid[2].size() == 1 && obj.grid[2][0] == 6;
        }),
    "Nested vectors"
);

// Test 4: std::vector of optionals
struct WithVectorOfOptionals {
    std::vector<std::optional<Inner>> items;
};

static_assert(
    TestParse<WithVectorOfOptionals>(R"({
        "items": [
            {"value": 1, "name": "first"},
            null,
            {"value": 3, "name": "third"}
        ]
    })", 
        [](const WithVectorOfOptionals& obj) {
            return obj.items.size() == 3
                && obj.items[0].has_value() 
                && obj.items[0]->value == 1 && obj.items[0]->name == "first"
                && !obj.items[1].has_value()
                && obj.items[2].has_value()
                && obj.items[2]->value == 3 && obj.items[2]->name == "third";
        }),
    "Vector of optional structs"
);

// Test 5: Complex nested structure
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
    TestParse<WithComplexNesting>(R"({
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
    })", 
        [](const WithComplexNesting& obj) {
            return obj.levels.size() == 2
                && obj.levels[0].count == 2
                && obj.levels[0].children.size() == 2
                && obj.levels[0].children[0].id == 1 && obj.levels[0].children[0].tag == "a"
                && obj.levels[0].children[1].id == 2 && obj.levels[0].children[1].tag == "b"
                && obj.levels[1].count == 1
                && obj.levels[1].children.size() == 1
                && obj.levels[1].children[0].id == 3 && obj.levels[1].children[0].tag == "c";
        }),
    "Complex nested structure with vectors"
);

