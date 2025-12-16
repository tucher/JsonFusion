#include "test_helpers.hpp"
using namespace TestHelpers;

struct Config {
    double value;
};

// Positive values
static_assert(TestSerialize(Config{42.5}, R"({"value":42.5})"));

// Negative values
static_assert(TestSerialize(Config{-123.5}, R"({"value":-123.5})"));

// Zero
// static_assert(TestSerialize(Config{0.0}, R"({"value":0.0})"));

