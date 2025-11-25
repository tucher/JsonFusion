#include "test_helpers.hpp"
using namespace TestHelpers;

struct Config {
    int value;
};

// Positive values
static_assert(TestParse(R"({"value": 42})", Config{42}));

// Negative values
static_assert(TestParse(R"({"value": -123})", Config{-123}));

// Zero
static_assert(TestParse(R"({"value": 0})", Config{0}));

// Boundary values
static_assert(TestParse(R"({"value": 2147483647})", Config{2147483647}));   // INT_MAX
static_assert(TestParse(R"({"value": -2147483648})", Config{-2147483648})); // INT_MIN

