#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/variant_transformer.hpp>
#include <string>
#include <variant>

using namespace JsonFusion;
using namespace JsonFusion::transformers;

// ============================================================================
// Test Structures
// ============================================================================

struct TypeA {
    int a_value;
};

struct TypeB {
    std::array<char, 32> b_name{};
};

struct TypeC {
    bool c_flag;
    int c_count;
};

// ============================================================================
// Test: Basic variant with two distinct types
// ============================================================================

constexpr bool test_variant_parses_type_a() {
    struct Model {
        VariantOneOf<TypeA, TypeB> data;
    };

    std::string_view json = R"({"data":{"a_value":42}})";

    Model m;
    auto result = Parse(m, json);
    if (!result) return false;

    // Should have parsed as TypeA
    if (!std::holds_alternative<TypeA>(m.data.value)) return false;
    if (std::get<TypeA>(m.data.value).a_value != 42) return false;

    return true;
}
static_assert(test_variant_parses_type_a(),
              "VariantOneOf should parse TypeA correctly");

// ============================================================================
// Test: Parse second variant type
// ============================================================================

constexpr bool test_variant_parses_type_b() {
    struct Model {
        VariantOneOf<TypeA, TypeB> data;
    };

    std::string_view json = R"({"data":{"b_name":"hello"}})";

    Model m;
    auto result = Parse(m, json);
    if (!result) return false;

    // Should have parsed as TypeB
    if (!std::holds_alternative<TypeB>(m.data.value)) return false;
    if (!TestHelpers::CStrEqual(std::get<TypeB>(m.data.value).b_name, "hello")) return false;

    return true;
}
static_assert(test_variant_parses_type_b(),
              "VariantOneOf should parse TypeB correctly");

// ============================================================================
// Test: Serialize variant (TypeA)
// ============================================================================

constexpr bool test_variant_serializes_type_a() {
    struct Model {
        VariantOneOf<TypeA, TypeB> data;
    };

    Model m;
    m.data.value = TypeA{123};

    std::string json;
    auto result = Serialize(m, json);
    if (!result) return false;

    // Should contain the TypeA field
    if (json.find("\"a_value\":123") == std::string::npos) return false;

    return true;
}
static_assert(test_variant_serializes_type_a(),
              "VariantOneOf should serialize TypeA correctly");

// ============================================================================
// Test: Serialize variant (TypeB)
// ============================================================================

constexpr bool test_variant_serializes_type_b() {
    struct Model {
        VariantOneOf<TypeA, TypeB> data;
    };

    Model m;
    TypeB b;
    for (std::size_t i = 0; i < 5; ++i) b.b_name[i] = "world"[i];
    b.b_name[5] = '\0';
    m.data.value = b;

    std::string json;
    auto result = Serialize(m, json);
    if (!result) return false;

    // Should contain the TypeB field
    if (json.find("\"b_name\":\"world\"") == std::string::npos) return false;

    return true;
}
static_assert(test_variant_serializes_type_b(),
              "VariantOneOf should serialize TypeB correctly");

// ============================================================================
// Test: Roundtrip with TypeA
// ============================================================================

constexpr bool test_variant_roundtrip_type_a() {
    struct Model {
        VariantOneOf<TypeA, TypeB> data;
    };

    Model original;
    original.data.value = TypeA{999};

    std::string json;
    if (!Serialize(original, json)) return false;

    Model parsed;
    if (!Parse(parsed, json)) return false;

    if (!std::holds_alternative<TypeA>(parsed.data.value)) return false;
    if (std::get<TypeA>(parsed.data.value).a_value != 999) return false;

    return true;
}
static_assert(test_variant_roundtrip_type_a(),
              "VariantOneOf should roundtrip TypeA");

// ============================================================================
// Test: Three-way variant
// ============================================================================

constexpr bool test_variant_three_types() {
    struct Model {
        VariantOneOf<TypeA, TypeB, TypeC> data;
    };

    // Test TypeC
    std::string_view json = R"({"data":{"c_flag":true,"c_count":10}})";

    Model m;
    auto result = Parse(m, json);
    if (!result) return false;

    if (!std::holds_alternative<TypeC>(m.data.value)) return false;
    if (!std::get<TypeC>(m.data.value).c_flag) return false;
    if (std::get<TypeC>(m.data.value).c_count != 10) return false;

    return true;
}
static_assert(test_variant_three_types(),
              "VariantOneOf should work with three types");

// ============================================================================
// Test: Ambiguous input fails (matches multiple types)
// ============================================================================

// Note: VariantOneOf uses "exactly one match" semantics
// If JSON matches multiple types, it should fail

constexpr bool test_variant_fails_on_ambiguous() {
    // TypeA has a_value (int)
    // Create a type that also accepts int fields
    struct IntType1 { int x; };
    struct IntType2 { int x; };  // Same shape!

    struct Model {
        VariantOneOf<IntType1, IntType2> data;
    };

    // This JSON matches BOTH types
    std::string_view json = R"({"data":{"x":42}})";

    Model m;
    auto result = Parse(m, json);

    // Should fail because both types match
    return !result;
}
static_assert(test_variant_fails_on_ambiguous(),
              "VariantOneOf should fail when multiple types match");

// ============================================================================
// Test: No match fails
// ============================================================================

constexpr bool test_variant_fails_on_no_match() {
    struct Model {
        VariantOneOf<TypeA, TypeB> data;
    };

    // This JSON has fields from neither TypeA nor TypeB
    std::string_view json = R"({"data":{"unknown_field":123}})";

    Model m;
    auto result = Parse(m, json);

    // Should fail because no type matches
    return !result;
}
static_assert(test_variant_fails_on_no_match(),
              "VariantOneOf should fail when no type matches");

// ============================================================================
// Test: Variant in nested structure
// ============================================================================

constexpr bool test_variant_nested() {
    struct Inner {
        VariantOneOf<TypeA, TypeB> item;
    };

    struct Outer {
        Inner nested;
        int other;
    };

    std::string_view json = R"({"nested":{"item":{"a_value":77}},"other":99})";

    Outer o;
    auto result = Parse(o, json);
    if (!result) return false;

    if (!std::holds_alternative<TypeA>(o.nested.item.value)) return false;
    if (std::get<TypeA>(o.nested.item.value).a_value != 77) return false;
    if (o.other != 99) return false;

    return true;
}
static_assert(test_variant_nested(),
              "VariantOneOf should work in nested structures");

// ============================================================================
// Test: Concept checks
// ============================================================================

static_assert(static_schema::ParseTransformerLike<VariantOneOf<TypeA, TypeB>>,
              "VariantOneOf should satisfy ParseTransformerLike");

static_assert(static_schema::SerializeTransformerLike<VariantOneOf<TypeA, TypeB>>,
              "VariantOneOf should satisfy SerializeTransformerLike");

