#include "test_helpers.hpp"
using namespace TestHelpers;

struct Config {
    int value;
};

// Positive values
static_assert(TestSerialize(Config{42}, R"({"value":42})"));

// Negative values
static_assert(TestSerialize(Config{-123}, R"({"value":-123})"));

// Zero
static_assert(TestSerialize(Config{0}, R"({"value":0})"));

// Boundary values
static_assert(TestSerialize(Config{2147483647}, R"({"value":2147483647})"));   // INT_MAX
static_assert(TestSerialize(Config{-2147483648}, R"({"value":-2147483648})")); // INT_MIN

