#include "test_helpers.hpp"
using namespace TestHelpers;

struct Config {
    bool flag;
};

// Boolean true
static_assert(TestParse(R"({"flag": true})", Config{true}));

// Boolean false
static_assert(TestParse(R"({"flag": false})", Config{false}));

