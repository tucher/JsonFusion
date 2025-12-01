#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <vector>

using namespace TestHelpers;

// ============================================================================
// Test: std::vector<T> with Primitives (C++23 constexpr-compatible)
// ============================================================================

// Test 1: std::vector<int> with various sizes
struct WithIntVector {
    std::vector<int> values;
};

static_assert(
    TestParse<WithIntVector>(R"({"values": [1, 2, 3]})", 
        [](const WithIntVector& obj) {
            return obj.values.size() == 3
                && obj.values[0] == 1
                && obj.values[1] == 2
                && obj.values[2] == 3;
        }),
    "Vector of integers: 3 elements"
);

static_assert(
    TestParse<WithIntVector>(R"({"values": []})", 
        [](const WithIntVector& obj) {
            return obj.values.size() == 0;
        }),
    "Vector: empty"
);

static_assert(
    TestParse<WithIntVector>(R"({"values": [42]})", 
        [](const WithIntVector& obj) {
            return obj.values.size() == 1 && obj.values[0] == 42;
        }),
    "Vector: single element"
);

// Test 2: Large vectors (100+ elements)
struct WithLargeVector {
    std::vector<int> numbers;
};

static_assert(
    TestParse<WithLargeVector>(R"({"numbers": [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99]})", 
        [](const WithLargeVector& obj) {
            if (obj.numbers.size() != 100) return false;
            for (std::size_t i = 0; i < 100; ++i) {
                if (obj.numbers[i] != static_cast<int>(i)) return false;
            }
            return true;
        }),
    "Vector: 100 elements"
);

// Test 4: Multiple vectors in same struct
struct WithMultipleVectors {
    std::vector<int> first;
    std::vector<int> second;
    std::vector<int> third;
};

static_assert(
    TestParse<WithMultipleVectors>(R"({
        "first": [1, 2],
        "second": [10, 20],
        "third": [100, 200, 300]
    })", 
        [](const WithMultipleVectors& obj) {
            return obj.first.size() == 2
                && obj.second.size() == 2
                && obj.third.size() == 3
                && obj.first[0] == 1 && obj.first[1] == 2
                && obj.second[0] == 10 && obj.second[1] == 20
                && obj.third[0] == 100 && obj.third[1] == 200 && obj.third[2] == 300;
        }),
    "Multiple vectors in same struct"
);

// Test 5: Vector with optional elements
struct WithOptionalVector {
    std::vector<std::optional<int>> values;
};

static_assert(
    TestParse<WithOptionalVector>(R"({"values": [1, null, 3, null, 5]})", 
        [](const WithOptionalVector& obj) {
            return obj.values.size() == 5
                && obj.values[0] == 1
                && !obj.values[1].has_value()
                && obj.values[2] == 3
                && !obj.values[3].has_value()
                && obj.values[4] == 5;
        }),
    "Vector with optional elements"
);

