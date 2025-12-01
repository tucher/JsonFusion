#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <array>

using namespace TestHelpers;

// ============================================================================
// Test: Fixed-Size Arrays of Primitives
// ============================================================================

// Test 1: std::array<int, N> for various N
struct WithIntArray {
    std::array<int, 3> values;
};

static_assert(
    TestParse(R"({"values": [1, 2, 3]})", 
        WithIntArray{{{1, 2, 3}}}),
    "Array of 3 integers"
);

struct WithIntArray5 {
    std::array<int, 5> values;
};

static_assert(
    TestParse(R"({"values": [10, 20, 30, 40, 50]})", 
        WithIntArray5{{{10, 20, 30, 40, 50}}}),
    "Array of 5 integers"
);

// Test 2: std::array<bool, N>
struct WithBoolArray {
    std::array<bool, 4> flags;
};

static_assert(
    TestParse(R"({"flags": [true, false, true, false]})", 
        WithBoolArray{{{true, false, true, false}}}),
    "Array of booleans"
);

// Test 3: Single element array
struct WithSingleElement {
    std::array<int, 1> value;
};

static_assert(
    TestParse(R"({"value": [42]})", 
        WithSingleElement{{{42}}}),
    "Single element array"
);

// Test 4: Large array
struct WithLargeArray {
    std::array<int, 10> values;
};

static_assert(
    TestParse(R"({"values": [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]})", 
        WithLargeArray{{{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}}}),
    "Large array (10 elements)"
);

// Test 5: Multiple arrays in same struct
struct WithMultipleArrays {
    std::array<int, 2> first;
    std::array<bool, 3> second;
    std::array<int, 2> third;
};

static_assert(
    TestParse(R"({
        "first": [1, 2],
        "second": [true, false, true],
        "third": [10, 20]
    })", 
        WithMultipleArrays{{{1, 2}}, {{true, false, true}}, {{10, 20}}}),
    "Multiple arrays in same struct"
);

