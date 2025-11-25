#include "test_helpers.hpp"
using namespace TestHelpers;

struct Config {
    bool flag;
};

// Boolean true
static_assert(TestSerialize(Config{true}, R"({"flag":true})"));

// Boolean false
static_assert(TestSerialize(Config{false}, R"({"flag":false})"));

