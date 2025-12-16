#include "test_helpers.hpp"
using namespace TestHelpers;

struct Config {
    double value;
};

// Positive values
static_assert(TestParse(R"({"value": 42.5})", Config{42.5}));

// Negative values
static_assert(TestParse(R"({"value": -123.5})", Config{-123.5}));

// Zero
static_assert(TestParse(R"({"value": 0.0})", Config{0.0}));

