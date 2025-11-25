#include "test_helpers.hpp"
#include <array>
using namespace TestHelpers;

struct Config {
    std::array<char, 16> name;
};

// String with null-termination check (DeepEqual handles this automatically)
static_assert(TestParse(R"({"name": "hello"})", Config{{'h','e','l','l','o','\0'}}));

// Empty string
static_assert(TestParse(R"({"name": ""})", Config{{'\0'}}));

// Single character
static_assert(TestParse(R"({"name": "x"})", Config{{'x','\0'}}));

// Longer string
static_assert(TestParse(R"({"name": "test_name"})", 
    Config{{'t','e','s','t','_','n','a','m','e','\0'}}));

// Explicit null-termination verification (when needed)
static_assert([]() constexpr {
    Config c;
    return ParseAndVerify(c, std::string_view(R"({"name": "hello"})"),
        [](const Config& cfg) {
            // DeepEqual checks null-termination, but we can also verify explicitly
            return cfg.name[0] == 'h' 
                && cfg.name[4] == 'o'
                && cfg.name[5] == '\0';  // ← explicit check
        });
}());

