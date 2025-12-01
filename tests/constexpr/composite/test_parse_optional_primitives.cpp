#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <optional>
#include <array>

using namespace TestHelpers;

// ============================================================================
// Test: Optional Primitives
// ============================================================================

// Test 1: std::optional<int> - null vs present
struct WithOptionalInt {
    std::optional<int> value;
};

static_assert(
    TestParse(R"({"value": 42})", 
        WithOptionalInt{42}),
    "Optional int: present value"
);

static_assert(
    TestParse(R"({"value": null})", 
        WithOptionalInt{std::nullopt}),
    "Optional int: null value"
);

// Test 2: std::optional<bool> - null vs true/false
struct WithOptionalBool {
    std::optional<bool> flag;
};

static_assert(
    TestParse(R"({"flag": true})", 
        WithOptionalBool{true}),
    "Optional bool: true"
);

static_assert(
    TestParse(R"({"flag": false})", 
        WithOptionalBool{false}),
    "Optional bool: false"
);

static_assert(
    TestParse(R"({"flag": null})", 
        WithOptionalBool{std::nullopt}),
    "Optional bool: null"
);

// Test 3: std::optional<std::array<char, N>> - null vs string
struct WithOptionalString {
    std::optional<std::array<char, 32>> name;
};

static_assert(
    TestParse(R"({"name": "Alice"})", 
        WithOptionalString{std::array<char, 32>{'A', 'l', 'i', 'c', 'e', '\0'}}),
    "Optional string: present"
);

static_assert(
    TestParse(R"({"name": null})", 
        WithOptionalString{std::nullopt}),
    "Optional string: null"
);

// Test 4: Multiple optionals in same struct
struct WithMultipleOptionals {
    std::optional<int> id;
    std::optional<bool> enabled;
    std::optional<std::array<char, 16>> tag;
};

static_assert(
    TestParse(R"({"id": 1, "enabled": true, "tag": "test"})", 
        WithMultipleOptionals{1, true, std::array<char, 16>{'t', 'e', 's', 't', '\0'}}),
    "Multiple optionals: all present"
);

static_assert(
    TestParse(R"({"id": null, "enabled": null, "tag": null})", 
        WithMultipleOptionals{std::nullopt, std::nullopt, std::nullopt}),
    "Multiple optionals: all null"
);

static_assert(
    TestParse(R"({"id": 42, "enabled": null, "tag": "active"})", 
        WithMultipleOptionals{42, std::nullopt, std::array<char, 16>{'a', 'c', 't', 'i', 'v', 'e', '\0'}}),
    "Multiple optionals: mixed present/null"
);

// Test 5: Optional in various positions (first, middle, last field)
struct OptionalFirst {
    std::optional<int> first;
    int second;
    bool third;
};

static_assert(
    TestParse(R"({"first": 10, "second": 20, "third": true})", 
        OptionalFirst{10, 20, true}),
    "Optional in first position"
);

struct OptionalMiddle {
    int first;
    std::optional<int> middle;
    bool third;
};

static_assert(
    TestParse(R"({"first": 10, "middle": 20, "third": true})", 
        OptionalMiddle{10, 20, true}),
    "Optional in middle position"
);

struct OptionalLast {
    int first;
    bool second;
    std::optional<int> last;
};

static_assert(
    TestParse(R"({"first": 10, "second": true, "last": 30})", 
        OptionalLast{10, true, 30}),
    "Optional in last position"
);

