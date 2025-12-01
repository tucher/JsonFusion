#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <array>

using namespace TestHelpers;
using namespace JsonFusion;

// ============================================================================
// Test: JSON Path Tracking for Array Element Errors
// ============================================================================

// Struct with std::array field
struct WithArray {
    int id;
    std::array<int, 3> values;
    bool flag;
};

// Test 1: Error in first array element
static_assert(
    TestParseErrorWithJsonPath<WithArray>(
        R"({"id": 1, "values": ["bad", 2, 3], "flag": true})",
        ParseError::ILLFORMED_NUMBER,
        "values", 0  // Expected path: $.middle.deep.data
    ),
   "Array path: error in first element ($.values[0])"
);


// Test 2: Error in middle array element
static_assert(
    TestParseErrorWithJsonPath<WithArray>(
        R"({"id": 1, "values": [1, "bad", 3], "flag": true})",
        ParseError::ILLFORMED_NUMBER,
        "values", 1  // Expected path: $.values[1]
    ),
    "Array path: error in middle element ($.values[1])"
);

// Test 3: Error in last array element
static_assert(
    TestParseErrorWithJsonPath<WithArray>(
        R"({"id": 1, "values": [1, 2, null], "flag": true})",
        ParseError::NULL_IN_NON_OPTIONAL,
        "values", 2  // Expected path: $.values[2]
    ),
    "Array path: error in last element ($.values[2])"
);

// ============================================================================
// Nested Arrays (2D)
// ============================================================================

struct With2DArray {
    std::array<std::array<int, 3>, 2> matrix;
};

// Test 4: Error in nested array - 2D coordinates
static_assert(
    TestParseErrorWithJsonPath<With2DArray>(
        R"({"matrix": [[1, 2, 3], [4, "bad", 6]]})",
        ParseError::ILLFORMED_NUMBER,
        "matrix", 1, 1  // Expected path: $.matrix[1][1]
    ),
    "2D Array path: error at [1][1] ($.matrix[1][1])"
);

// Test 5: Error in first row of 2D array
static_assert(
    TestParseErrorWithJsonPath<With2DArray>(
        R"({"matrix": [[1, null, 3], [4, 5, 6]]})",
        ParseError::NULL_IN_NON_OPTIONAL,
        "matrix", 0, 1  // Expected path: $.matrix[0][1]
    ),
    "2D Array path: error at [0][1] ($.matrix[0][1])"
);

// ============================================================================
// Array of Structs
// ============================================================================

struct Point {
    int x;
    int y;
};

struct WithStructArray {
    std::array<Point, 3> points;
};

// Test 6: Error in struct field within array
static_assert(
    TestParseErrorWithJsonPath<WithStructArray>(
        R"({"points": [{"x": 1, "y": 2}, {"x": "bad", "y": 4}, {"x": 5, "y": 6}]})",
        ParseError::ILLFORMED_NUMBER,
        "points", 1, "x"  // Expected path: $.points[1].x
    ),
    "Array of structs: error in field ($.points[1].x)"
);

// Test 7: Error in different field of array element
static_assert(
    TestParseErrorWithJsonPath<WithStructArray>(
        R"({"points": [{"x": 1, "y": 2}, {"x": 3, "y": 4}, {"x": 5, "y": true}]})",
        ParseError::ILLFORMED_NUMBER,
        "points", 2, "y"  // Expected path: $.points[2].y
    ),
    "Array of structs: error in different element ($.points[2].y)"
);

// ============================================================================
// Nested Struct with Array
// ============================================================================

struct Outer {
    int id;
    struct Inner {
        std::array<int, 2> data;
    } inner;
};

// Test 8: Error in array within nested struct
static_assert(
    TestParseErrorWithJsonPath<Outer>(
        R"({"id": 1, "inner": {"data": [10, "bad"]}})",
        ParseError::ILLFORMED_NUMBER,
        "inner", "data", 1  // Expected path: $.inner.data[1]
    ),
    "Nested struct with array: error path ($.inner.data[1])"
);

// ============================================================================
// Using Helper Functions
// ============================================================================

// Test 9: Using TestParseErrorWithJsonPath for array paths
static_assert(
    TestParseErrorWithJsonPath<WithArray>(
        R"({"id": 1, "values": [1, "bad", 3], "flag": true})",
        ParseError::ILLFORMED_NUMBER,
        "values", 1  // $.values[1]
    ),
    "Helper: Array element path with TestParseErrorWithJsonPath"
);

// Test 10: 2D array with helper
static_assert(
    TestParseErrorWithJsonPath<With2DArray>(
        R"({"matrix": [[1, 2, 3], [4, "bad", 6]]})",
        ParseError::ILLFORMED_NUMBER,
        "matrix", 1, 1  // $.matrix[1][1]
    ),
    "Helper: 2D array path"
);

// Test 11: Array of structs with helper
static_assert(
    TestParseErrorWithJsonPath<WithStructArray>(
        R"({"points": [{"x": 1, "y": 2}, {"x": "bad", "y": 4}, {"x": 5, "y": 6}]})",
        ParseError::ILLFORMED_NUMBER,
        "points", 1, "x"  // $.points[1].x
    ),
    "Helper: Array of structs path"
);

// Test 12: Using TestParseErrorWithPathDepth
static_assert(
    TestParseErrorWithPathDepth<WithArray>(
        R"({"id": 1, "values": [1, "bad", 3], "flag": true})",
        ParseError::ILLFORMED_NUMBER,
        2  // Path depth: "values", index 1
    ),
    "Helper: Verify path depth for array error"
);

// ============================================================================
// Complex Nested Array Paths
// ============================================================================

struct Level2 {
    std::array<int, 2> values;
};

struct DeepNested {
    struct Level1 {
        std::array<Level2, 2> items;
    } data;
};

// Test 13: Very deep array nesting
static_assert(
    TestParseErrorWithJsonPath<DeepNested>(
        R"({"data": {"items": [{"values": [1, 2]}, {"values": [3, null]}]}})",
        ParseError::NULL_IN_NON_OPTIONAL,
        "data", "items", 1, "values", 1  // Expected path: $.middle.deep.data
    ),
   "Deep nesting: struct -> array -> struct -> array -> element"
);

// ============================================================================
// Array Index at Different Positions
// ============================================================================

struct MultiArray {
    std::array<int, 2> first;
    std::array<int, 2> second;
    std::array<int, 2> third;
};

// Test 14: Verify correct array index for different fields
static_assert(
    TestParseErrorWithJsonPath<MultiArray>(
        R"({"first": [1, 2], "second": [3, "bad"], "third": [5, 6]})",
        ParseError::ILLFORMED_NUMBER,
        "second", 1  // Expected path: $.second[1]
    ),
    "Multiple arrays: correct field and index ($.second[1])"
);

// ============================================================================
// Using Generic JsonPath Comparison (Cleaner API)
// ============================================================================

// Test 15: Generic comparison - simple array element
static_assert(
    TestParseErrorWithJsonPath<WithArray>(
        R"({"id": 1, "values": [1, "bad", 3], "flag": true})",
        ParseError::ILLFORMED_NUMBER,
        "values", 1  // Expected path: $.values[1]
    ),
    "Generic path: array element error"
);

// Test 16: Generic comparison - 2D array
static_assert(
    TestParseErrorWithJsonPath<With2DArray>(
        R"({"matrix": [[1, 2, 3], [4, "bad", 6]]})",
        ParseError::ILLFORMED_NUMBER,
        "matrix", 1, 1  // Expected path: $.matrix[1][1]
    ),
    "Generic path: 2D array error"
);

// Test 17: Generic comparison - array of structs
static_assert(
    TestParseErrorWithJsonPath<WithStructArray>(
        R"({"points": [{"x": 1, "y": 2}, {"x": "bad", "y": 4}, {"x": 5, "y": 6}]})",
        ParseError::ILLFORMED_NUMBER,
        "points", 1, "x"  // Expected path: $.points[1].x
    ),
    "Generic path: array of structs error"
);

// Test 18: Generic comparison - deep nesting with arrays
static_assert(
    TestParseErrorWithJsonPath<DeepNested>(
        R"({"data": {"items": [{"values": [1, 2]}, {"values": [3, null]}]}})",
        ParseError::NULL_IN_NON_OPTIONAL,
        "data", "items", 1, "values", 1  // $.data.items[1].values[1]
    ),
    "Generic path: deeply nested array error"
);

