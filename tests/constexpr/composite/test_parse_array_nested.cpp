#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <array>

using namespace TestHelpers;

// ============================================================================
// Test: Arrays of Nested Types
// ============================================================================

// Test 1: std::array<NestedStruct, N>
struct Point {
    int x;
    int y;
};

struct WithStructArray {
    std::array<Point, 3> points;
};

static_assert(
    TestParse<WithStructArray>(R"({"points": [{"x": 1, "y": 2}, {"x": 3, "y": 4}, {"x": 5, "y": 6}]})", 
        [](const WithStructArray& obj) {
            return obj.points[0].x == 1 && obj.points[0].y == 2
                && obj.points[1].x == 3 && obj.points[1].y == 4
                && obj.points[2].x == 5 && obj.points[2].y == 6;
        }),
    "Array of structs"
);

// Test 2: 2D arrays: std::array<std::array<int, 3>, 3>
struct With2DArray {
    std::array<std::array<int, 3>, 3> matrix;
};

static_assert(
    TestParse<With2DArray>(R"({"matrix": [[1, 2, 3], [4, 5, 6], [7, 8, 9]]})", 
        [](const With2DArray& obj) {
            return obj.matrix[0][0] == 1 && obj.matrix[0][1] == 2 && obj.matrix[0][2] == 3
                && obj.matrix[1][0] == 4 && obj.matrix[1][1] == 5 && obj.matrix[1][2] == 6
                && obj.matrix[2][0] == 7 && obj.matrix[2][1] == 8 && obj.matrix[2][2] == 9;
        }),
    "2D array (3x3)"
);

// Test 3: 2D array with different dimensions
struct With2DArray2x4 {
    std::array<std::array<int, 4>, 2> matrix;
};

static_assert(
    TestParse<With2DArray2x4>(R"({"matrix": [[1, 2, 3, 4], [5, 6, 7, 8]]})", 
        [](const With2DArray2x4& obj) {
            return obj.matrix[0][0] == 1 && obj.matrix[0][3] == 4
                && obj.matrix[1][0] == 5 && obj.matrix[1][3] == 8;
        }),
    "2D array (2x4)"
);

// Test 4: 3D arrays
struct With3DArray {
    std::array<std::array<std::array<int, 2>, 2>, 2> tensor;
};

static_assert(
    TestParse<With3DArray>(R"({"tensor": [[[1, 2], [3, 4]], [[5, 6], [7, 8]]]})", 
        [](const With3DArray& obj) {
            return obj.tensor[0][0][0] == 1 && obj.tensor[0][0][1] == 2
                && obj.tensor[1][1][0] == 7 && obj.tensor[1][1][1] == 8;
        }),
    "3D array (2x2x2)"
);

// Test 5: Array of nested structs with multiple fields
struct Person {
    int id;
    bool active;
};

struct WithPersonArray {
    std::array<Person, 2> people;
};

static_assert(
    TestParse<WithPersonArray>(R"({"people": [{"id": 1, "active": true}, {"id": 2, "active": false}]})", 
        [](const WithPersonArray& obj) {
            return obj.people[0].id == 1 && obj.people[0].active == true
                && obj.people[1].id == 2 && obj.people[1].active == false;
        }),
    "Array of structs with multiple fields"
);

// Test 6: Array of deeply nested structs
struct Level3 {
    int value;
};

struct Level2 {
    Level3 deep;
};

struct WithDeepArray {
    std::array<Level2, 2> items;
};

static_assert(
    TestParse<WithDeepArray>(R"({"items": [{"deep": {"value": 10}}, {"deep": {"value": 20}}]})", 
        [](const WithDeepArray& obj) {
            return obj.items[0].deep.value == 10
                && obj.items[1].deep.value == 20;
        }),
    "Array of deeply nested structs"
);

