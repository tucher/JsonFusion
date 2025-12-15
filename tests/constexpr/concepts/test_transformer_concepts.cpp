#include "../test_helpers.hpp"
#include <JsonFusion/static_schema.hpp>
#include <string>

using namespace JsonFusion;
using namespace JsonFusion::static_schema;

// ============================================================================
// Bypass Transformers - Simple pass-through for testing
// ============================================================================

// Bypass Parse Transformer: holds T, exposes T as wire_type
template<typename T>
struct BypassParseTransformer {
    using wire_type = T;
    T value{};
    
    constexpr bool transform_from(const wire_type& w) {
        value = w;
        return true;
    }
};

// Bypass Serialize Transformer: holds T, exposes T as wire_type
template<typename T>
struct BypassSerializeTransformer {
    using wire_type = T;
    T value{};
    
    constexpr bool transform_to(wire_type& w) const {
        w = value;
        return true;
    }
};

// ============================================================================
// Test: ParseTransformer concept with basic types
// ============================================================================

// Test: ParseTransformer with int
static_assert(ParseTransformer<BypassParseTransformer<int>>,
              "BypassParseTransformer<int> should satisfy ParseTransformer");

// Test: ParseTransformer with bool
static_assert(ParseTransformer<BypassParseTransformer<bool>>,
              "BypassParseTransformer<bool> should satisfy ParseTransformer");

// Test: ParseTransformer with double
static_assert(ParseTransformer<BypassParseTransformer<double>>,
              "BypassParseTransformer<double> should satisfy ParseTransformer");

// Test: ParseTransformer with std::string
#if __has_include(<string>)
static_assert(ParseTransformer<BypassParseTransformer<std::string>>,
              "BypassParseTransformer<std::string> should satisfy ParseTransformer");
#endif

// ============================================================================
// Test: SerializeTransformer concept with basic types
// ============================================================================

// Test: SerializeTransformer with int
static_assert(SerializeTransformer<BypassSerializeTransformer<int>>,
              "BypassSerializeTransformer<int> should satisfy SerializeTransformer");

// Test: SerializeTransformer with bool
static_assert(SerializeTransformer<BypassSerializeTransformer<bool>>,
              "BypassSerializeTransformer<bool> should satisfy SerializeTransformer");

// Test: SerializeTransformer with double
static_assert(SerializeTransformer<BypassSerializeTransformer<double>>,
              "BypassSerializeTransformer<double> should satisfy SerializeTransformer");

// Test: SerializeTransformer with std::string
#if __has_include(<string>)
static_assert(SerializeTransformer<BypassSerializeTransformer<std::string>>,
              "BypassSerializeTransformer<std::string> should satisfy SerializeTransformer");
#endif

// ============================================================================
// Test: Negative cases - missing methods
// ============================================================================

// Missing transform_from method
template<typename T>
struct BadParseTransformer1 {
    using wire_type = T;
    T value{};
    // Missing transform_from!
};

static_assert(!ParseTransformer<BadParseTransformer1<int>>,
              "Transformer without transform_from should NOT satisfy ParseTransformer");

// Missing wire_type
template<typename T>
struct BadParseTransformer2 {
    // Missing wire_type!
    T value{};
    constexpr bool transform_from(const T& w) {
        value = w;
        return true;
    }
};

static_assert(!ParseTransformer<BadParseTransformer2<int>>,
              "Transformer without wire_type should NOT satisfy ParseTransformer");

// Wrong return type for transform_from
template<typename T>
struct BadParseTransformer3 {
    using wire_type = T;
    T value{};
    constexpr void transform_from(const wire_type& w) {  // returns void, not bool!
        value = w;
    }
};

static_assert(!ParseTransformer<BadParseTransformer3<int>>,
              "Transformer with wrong return type should NOT satisfy ParseTransformer");

// Similar negative tests for SerializeTransformer
template<typename T>
struct BadSerializeTransformer1 {
    using wire_type = T;
    T value{};
    // Missing transform_to!
};

static_assert(!SerializeTransformer<BadSerializeTransformer1<int>>,
              "Transformer without transform_to should NOT satisfy SerializeTransformer");

// ============================================================================
// Test: parse_transform_traits and serialize_transform_traits
// ============================================================================

// Test: is_parse_transformer_v detects transformers correctly
static_assert(is_parse_transformer_v<BypassParseTransformer<int>>,
              "is_parse_transformer_v should detect BypassParseTransformer");

static_assert(!is_parse_transformer_v<int>,
              "is_parse_transformer_v should NOT detect plain int as transformer");

static_assert(!is_parse_transformer_v<BadParseTransformer1<int>>,
              "is_parse_transformer_v should NOT detect bad transformer");

// Test: is_serialize_transformer_v detects transformers correctly
static_assert(is_serialize_transformer_v<BypassSerializeTransformer<int>>,
              "is_serialize_transformer_v should detect BypassSerializeTransformer");

static_assert(!is_serialize_transformer_v<int>,
              "is_serialize_transformer_v should NOT detect plain int as transformer");

static_assert(!is_serialize_transformer_v<BadSerializeTransformer1<int>>,
              "is_serialize_transformer_v should NOT detect bad transformer");

// ============================================================================
// Test: Transformer traits wire_type extraction
// ============================================================================

// Test: parse_transform_traits extracts wire_type correctly
static_assert(std::is_same_v<
                  parse_transform_traits<BypassParseTransformer<int>>::wire_type,
                  int>,
              "parse_transform_traits should extract wire_type correctly");

static_assert(std::is_same_v<
                  parse_transform_traits<BypassParseTransformer<double>>::wire_type,
                  double>,
              "parse_transform_traits should extract wire_type for double");

// Test: serialize_transform_traits extracts wire_type correctly
static_assert(std::is_same_v<
                  serialize_transform_traits<BypassSerializeTransformer<int>>::wire_type,
                  int>,
              "serialize_transform_traits should extract wire_type correctly");

static_assert(std::is_same_v<
                  serialize_transform_traits<BypassSerializeTransformer<bool>>::wire_type,
                  bool>,
              "serialize_transform_traits should extract wire_type for bool");

// ============================================================================
// Test: Functional behavior of bypass transformers
// ============================================================================

constexpr bool test_bypass_parse_transformer_int() {
    BypassParseTransformer<int> transformer;
    int wire_value = 42;
    
    if (!transformer.transform_from(wire_value)) return false;
    if (transformer.value != 42) return false;
    
    return true;
}
static_assert(test_bypass_parse_transformer_int(),
              "BypassParseTransformer<int> should transform correctly");

constexpr bool test_bypass_serialize_transformer_int() {
    BypassSerializeTransformer<int> transformer;
    transformer.value = 99;
    
    int wire_value = 0;
    if (!transformer.transform_to(wire_value)) return false;
    if (wire_value != 99) return false;
    
    return true;
}
static_assert(test_bypass_serialize_transformer_int(),
              "BypassSerializeTransformer<int> should transform correctly");

constexpr bool test_bypass_parse_transformer_bool() {
    BypassParseTransformer<bool> transformer;
    
    if (!transformer.transform_from(true)) return false;
    if (transformer.value != true) return false;
    
    if (!transformer.transform_from(false)) return false;
    if (transformer.value != false) return false;
    
    return true;
}
static_assert(test_bypass_parse_transformer_bool(),
              "BypassParseTransformer<bool> should transform correctly");

// ============================================================================
// Test: Transformers with complex wire types (arrays)
// ============================================================================

constexpr bool test_bypass_parse_transformer_array() {
    using ArrayType = std::array<int, 3>;
    BypassParseTransformer<ArrayType> transformer;
    
    ArrayType wire_value{1, 2, 3};
    if (!transformer.transform_from(wire_value)) return false;
    if (transformer.value[0] != 1) return false;
    if (transformer.value[1] != 2) return false;
    if (transformer.value[2] != 3) return false;
    
    return true;
}
static_assert(test_bypass_parse_transformer_array(),
              "BypassParseTransformer with std::array should work");

static_assert(ParseTransformer<BypassParseTransformer<std::array<int, 3>>>,
              "BypassParseTransformer<std::array<int, 3>> should satisfy ParseTransformer");

// ============================================================================
// Test: Transformers with optional wire types
// ============================================================================

constexpr bool test_bypass_parse_transformer_optional() {
    using OptType = std::optional<int>;
    BypassParseTransformer<OptType> transformer;
    
    OptType wire_value = 42;
    if (!transformer.transform_from(wire_value)) return false;
    if (!transformer.value.has_value()) return false;
    if (transformer.value.value() != 42) return false;
    
    wire_value = std::nullopt;
    if (!transformer.transform_from(wire_value)) return false;
    if (transformer.value.has_value()) return false;
    
    return true;
}
static_assert(test_bypass_parse_transformer_optional(),
              "BypassParseTransformer with std::optional should work");

static_assert(ParseTransformer<BypassParseTransformer<std::optional<int>>>,
              "BypassParseTransformer<std::optional<int>> should satisfy ParseTransformer");

// ============================================================================
// Test: Nested transformers (transformer as wire_type)
// ============================================================================

// Test: Nested ParseTransformer - outer transformer's wire_type is another transformer
using NestedParseTransformer = BypassParseTransformer<BypassParseTransformer<int>>;

static_assert(ParseTransformer<NestedParseTransformer>,
              "Nested ParseTransformer (transformer wrapping transformer) should satisfy concept");

static_assert(std::is_same_v<
                  parse_transform_traits<NestedParseTransformer>::wire_type,
                  BypassParseTransformer<int>>,
              "Nested ParseTransformer should have inner transformer as wire_type");

constexpr bool test_nested_parse_transformer() {
    // Outer transformer holds inner transformer
    NestedParseTransformer outer;
    
    // Wire value is an inner transformer with value 42
    BypassParseTransformer<int> inner_wire;
    inner_wire.value = 42;
    
    // Transform from inner transformer
    if (!outer.transform_from(inner_wire)) return false;
    
    // Outer now holds a copy of inner
    if (outer.value.value != 42) return false;
    
    return true;
}
static_assert(test_nested_parse_transformer(),
              "Nested ParseTransformer should function correctly");

// Test: Nested SerializeTransformer - outer transformer's wire_type is another transformer
using NestedSerializeTransformer = BypassSerializeTransformer<BypassSerializeTransformer<int>>;

static_assert(SerializeTransformer<NestedSerializeTransformer>,
              "Nested SerializeTransformer (transformer wrapping transformer) should satisfy concept");

static_assert(std::is_same_v<
                  serialize_transform_traits<NestedSerializeTransformer>::wire_type,
                  BypassSerializeTransformer<int>>,
              "Nested SerializeTransformer should have inner transformer as wire_type");

constexpr bool test_nested_serialize_transformer() {
    // Outer transformer with inner value
    NestedSerializeTransformer outer;
    outer.value.value = 99;
    
    // Transform to wire (which is an inner transformer)
    BypassSerializeTransformer<int> inner_wire;
    if (!outer.transform_to(inner_wire)) return false;
    
    // Wire should now hold the value
    if (inner_wire.value != 99) return false;
    
    return true;
}
static_assert(test_nested_serialize_transformer(),
              "Nested SerializeTransformer should function correctly");


