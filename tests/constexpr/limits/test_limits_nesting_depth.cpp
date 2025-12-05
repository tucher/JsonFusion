#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <string>
#include <array>
#include <optional>

using namespace TestHelpers;
using namespace JsonFusion;

// ============================================================================
// Test: Deep Nesting Limits
// ============================================================================

// Test 1: 10 levels of nesting
struct Level11 {
    int value;
};
struct Level10 {
    Level11 nested;
};
struct Level9 {
    Level10 nested;
};

struct Level8 {
    Level9 nested;
};

struct Level7 {
    Level8 nested;
};

struct Level6 {
    Level7 nested;
};

struct Level5 {
    Level6 nested;
};

struct Level4 {
    Level5 nested;
};

struct Level3 {
    Level4 nested;
};

struct Level2 {
    Level3 nested;
};

struct Level1 {
    Level2 nested;
};

constexpr bool test_nesting_10_levels() {
    Level1 obj{};
    // Build JSON string properly - 10 levels deep
    constexpr std::string_view json = R"({"nested":{"nested":{"nested":{"nested":{"nested":{"nested":{"nested":{"nested":{"nested":{"nested":{"value":42}}}}}}}}}}})";
    auto result = JsonFusion::Parse(obj, json);
    
    if (!result) return false;
    
    // Navigate through all 10 levels
    return obj.nested.nested.nested.nested.nested.nested.nested.nested.nested.nested.value == 42;
}
static_assert(test_nesting_10_levels(), "10 levels of nesting");

// Test 2: 5 levels of nesting (more practical)
struct L5 { int v; };
struct L4 { L5 n; };
struct L3 { L4 n; };
struct L2 { L3 n; };
struct L1 { L2 n; };

constexpr bool test_nesting_5_levels() {
    L1 obj{};
    constexpr std::string_view json = R"({"n":{"n":{"n":{"n":{"v":100}}}}})";
    auto result = JsonFusion::Parse(obj, json);
    
    if (!result) return false;
    return obj.n.n.n.n.v == 100;
}
static_assert(test_nesting_5_levels(), "5 levels of nesting");

// Test 3: Deep nesting with arrays (simplified to 2 levels)
struct DeepArray {
    int value;
};


struct ArrayLevel1 {
    std::array<std::array<DeepArray, 3>, 2> items;
};

constexpr bool test_nesting_with_arrays() {
    ArrayLevel1 obj{};
    // 2 levels of arrays: outer array with 2 elements, each containing array with 3 elements
    constexpr std::string_view json = R"({"items":[[{"value":1},{"value":2},{"value":3}],[{"value":4},{"value":5},{"value":6}]]})";
    auto result = JsonFusion::Parse(obj, json);
    
    if (!result) return false;
    
    // Verify array access
    return obj.items[0][0].value == 1
        && obj.items[0][1].value == 2
        && obj.items[0][2].value == 3
        && obj.items[1][0].value == 4
        && obj.items[1][1].value == 5
        && obj.items[1][2].value == 6;
}
static_assert(test_nesting_with_arrays(), "Deep nesting with arrays");

// Test 4: Deep nesting with optional (tests unique_ptr too)
struct DeepOptional {
    int value;
};

struct OptLevel4 {
    std::optional<DeepOptional> nested;
};

struct OptLevel3 {
    OptLevel4 nested;
};

struct OptLevel2 {
    OptLevel3 nested;
};

struct OptLevel1 {
    OptLevel2 nested;
};

constexpr bool test_nesting_with_optional() {
    OptLevel1 obj{};
    constexpr std::string_view json = R"({"nested":{"nested":{"nested":{"nested":{"value":999}}}}})";
    auto result = JsonFusion::Parse(obj, json);
    
    if (!result) return false;
    
    return obj.nested.nested.nested.nested.has_value()
        && obj.nested.nested.nested.nested->value == 999;
}
static_assert(test_nesting_with_optional(), "Deep nesting with optional");

// Test 5: Mixed nesting (structs, arrays, optionals)
struct MixedLeaf {
    int id;
    bool flag;
};

struct MixedLevel2 {
    std::array<MixedLeaf, 3> items;
    std::optional<MixedLeaf> extra;
};

struct MixedLevel1 {
    MixedLevel2 nested;
    int count;
};

constexpr bool test_mixed_nesting() {
    MixedLevel1 obj{};
    constexpr std::string_view json = R"({"nested":{"items":[{"id":1,"flag":true},{"id":2,"flag":false},{"id":3,"flag":true}],"extra":{"id":99,"flag":false}},"count":42})";
    auto result = JsonFusion::Parse(obj, json);
    
    if (!result) return false;
    
    return obj.nested.items[0].id == 1
        && obj.nested.items[1].flag == false
        && obj.nested.items[2].id == 3
        && obj.nested.extra.has_value()
        && obj.nested.extra->id == 99
        && obj.count == 42;
}
static_assert(test_mixed_nesting(), "Mixed nesting (structs, arrays, optionals)");

