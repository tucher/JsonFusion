#include "test_helpers.hpp"
using namespace TestHelpers;
using JsonFusion::A;
using namespace JsonFusion::validators;

// ============================================================================
// 3+ Levels: Array of Objects containing Arrays of Objects
// ============================================================================

struct Item {
    int id;
    std::string name;
};

struct Container {
    std::vector<Item> items;
};

struct Root {
    std::vector<Container> containers;
};

// 3 levels: Root → containers[] → items[]
// WORKAROUND for GCC constexpr: construct expected value locally in lambda
static_assert([]() constexpr {
    Root obj{};
    auto result = JsonFusion::Parse(obj, std::string_view(
        R"({"containers":[{"items":[{"id":1,"name":"a"},{"id":2,"name":"b"}]},{"items":[{"id":3,"name":"c"}]}]})"));
    if (!result) return false;
    
    Root expected{.containers={
        Container{.items={{1, "a"}, {2, "b"}}},
        Container{.items={{3, "c"}}}
    }};
    
    return DeepEqual(obj, expected);
}());

// Empty at each level
static_assert(TestParse(R"({"containers":[]})", Root{{}}));

// WORKAROUND for GCC constexpr: empty Container construction
static_assert([]() constexpr {
    Root obj{};
    auto result = JsonFusion::Parse(obj, std::string_view(R"({"containers":[{"items":[]}]})"));
    if (!result) return false;
    
    Root expected{{Container{}}};
    return DeepEqual(obj, expected);
}());

// ============================================================================
// Optional → Array → Optional Objects
// ============================================================================

struct OptionalItem {
    std::optional<int> value;
};

struct OptionalArrayContainer {
    std::optional<std::vector<OptionalItem>> maybe_items;
};

// All present
static_assert(TestParse(
    R"({"maybe_items":[{"value":1},{"value":null},{"value":3}]})",
    OptionalArrayContainer{.maybe_items=std::vector<OptionalItem>{
        {.value=1}, {.value=std::nullopt}, {.value=3}
    }}
));

// Outer optional null
static_assert(TestParse(
    R"({"maybe_items":null})",
    OptionalArrayContainer{.maybe_items=std::nullopt}
));

// Outer present, array empty
static_assert(TestParse(
    R"({"maybe_items":[]})",
    OptionalArrayContainer{.maybe_items=std::vector<OptionalItem>{}}
));

// ============================================================================
// Unique_ptr → Struct with Optional Array
// ============================================================================

struct DeepStruct {
    std::optional<std::vector<int>> maybe_values;
};

struct UniqueContainer {
    std::unique_ptr<DeepStruct> deep;
};

// Test with lambda (unique_ptr not copyable)
static_assert([]() constexpr {
    UniqueContainer cfg;
    auto r = JsonFusion::Parse(cfg, R"({"deep":{"maybe_values":[1,2,3]}})");
    if (!r || !cfg.deep || !cfg.deep->maybe_values) return false;
    auto& vec = *cfg.deep->maybe_values;
    return vec.size() == 3 && vec[0] == 1 && vec[1] == 2 && vec[2] == 3;
}());

// Null at various levels
static_assert([]() constexpr {
    UniqueContainer cfg;
    auto r = JsonFusion::Parse(cfg, R"({"deep":null})");
    return r && !cfg.deep;
}());

static_assert([]() constexpr {
    UniqueContainer cfg;
    auto r = JsonFusion::Parse(cfg, R"({"deep":{"maybe_values":null}})");
    return r && cfg.deep && !cfg.deep->maybe_values;
}());

// ============================================================================
// Vector → Optional → Array → Struct
// ============================================================================

struct NestedItem {
    int x;
    int y;
};

struct ArrayWrapper {
    std::array<NestedItem, 2> items;
};

struct VectorOptionalArray {
    std::vector<std::optional<ArrayWrapper>> data;
};

// Mix of present/absent optionals
static_assert(TestParse(
    R"({"data":[{"items":[{"x":1,"y":2},{"x":3,"y":4}]},null,{"items":[{"x":5,"y":6},{"x":7,"y":8}]}]})",
    VectorOptionalArray{.data={
        std::optional<ArrayWrapper>{ArrayWrapper{.items={NestedItem{1, 2}, NestedItem{3, 4}}}},
        std::nullopt,
        std::optional<ArrayWrapper>{ArrayWrapper{.items={NestedItem{5, 6}, NestedItem{7, 8}}}}
    }}
));

// ============================================================================
// All Fields with Defaults at Each Level
// ============================================================================

struct Level3 {
    int value = 300;
    std::string name = "default3";
};

struct Level2 {
    Level3 inner = Level3{400, "default2"};
    int count = 20;
};

struct Level1 {
    Level2 mid = Level2{Level3{500, "default1"}, 30};
    bool active = true;
};

// All defaults cascade
static_assert(TestParse(R"({})", Level1{Level2{Level3{500, "default1"}, 30}, true}));

// Override only at deepest level (other defaults preserved)
static_assert(TestParse(
    R"({"mid":{"inner":{"value":999}}})",
    Level1{Level2{Level3{999, "default1"}, 30}, true}  // Uses Level2's default for inner and count
));

// Override only at middle level
static_assert(TestParse(
    R"({"mid":{"count":777}})",
    Level1{Level2{Level3{500, "default1"}, 777}, true}
));

// Override at all levels
static_assert(TestParse(
    R"({"mid":{"inner":{"value":1,"name":"a"},"count":2},"active":false})",
    Level1{Level2{Level3{1, "a"}, 2}, false}
));

// ============================================================================
// Error Propagation from Deep Levels
// ============================================================================

struct ValidatedDeep {
    A<int, range<0, 100>> validated_value;
};

struct ValidatedMiddle {
    ValidatedDeep deep;
};

struct ValidatedOuter {
    ValidatedMiddle middle;
};

// Valid: value in range at deep level
static_assert(TestParse(
    R"({"middle":{"deep":{"validated_value":50}}})",
    ValidatedOuter{ValidatedMiddle{ValidatedDeep{50}}}
));

// Invalid: violation at deep level propagates up
static_assert(TestParseError<ValidatedOuter>(
    R"({"middle":{"deep":{"validated_value":150}}})",
    JsonFusion::ParseError::SCHEMA_VALIDATION_ERROR
));

// Invalid: type error at deep level propagates up
static_assert(TestParseError<ValidatedOuter>(
    R"({"middle":{"deep":{"validated_value":"not a number"}}})",
    JsonFusion::JsonIteratorReaderError::ILLFORMED_NUMBER
));

// ============================================================================
// Deeply Nested Arrays with Validation
// ============================================================================

struct DeepArrays {
    A<std::vector<std::vector<A<int, range<1, 10>>>>, min_items<1>> data;
};

// Valid: all constraints satisfied
// WORKAROUND for GCC constexpr: nested vector initialization in lambda
static_assert([]() constexpr {
    DeepArrays obj{};
    auto result = JsonFusion::Parse(obj, std::string_view(R"({"data":[[1,2,3],[4,5],[6,7,8,9]]})"));
    if (!result) return false;
    
    DeepArrays expected{.data=std::vector<std::vector<A<int, range<1, 10>>>>{
        std::vector<A<int, range<1, 10>>>{1, 2, 3},
        std::vector<A<int, range<1, 10>>>{4, 5},
        std::vector<A<int, range<1, 10>>>{6, 7, 8, 9}
    }};
    
    return DeepEqual(obj, expected);
}());

// Invalid: empty outer array (violates min_items<1>)
static_assert(TestParseError<DeepArrays>(
    R"({"data":[]})",
    JsonFusion::ParseError::SCHEMA_VALIDATION_ERROR
));

// Invalid: value out of range at innermost level
static_assert(TestParseError<DeepArrays>(
    R"({"data":[[1,2,15]]})",
    JsonFusion::ParseError::SCHEMA_VALIDATION_ERROR
));

