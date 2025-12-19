#include "test_helpers.hpp"
using namespace TestHelpers;
using JsonFusion::A;
using namespace JsonFusion::validators;

// ============================================================================
// Optional with Null vs Missing
// ============================================================================

struct ConfigOptional {
    std::optional<int> maybe_value;
    int other = 99;
};

// Missing field → nullopt
static_assert(TestParse(R"({"other":42})", ConfigOptional{std::nullopt, 42}));

// Explicit null → nullopt
static_assert(TestParse(R"({"maybe_value":null,"other":42})", ConfigOptional{std::nullopt, 42}));

// Provided value → has_value
static_assert(TestParse(R"({"maybe_value":123,"other":42})", ConfigOptional{123, 42}));

// Both missing → both default/nullopt
static_assert(TestParse(R"({})", ConfigOptional{std::nullopt, 99}));

// ============================================================================
// Optional with Wrong Type
// ============================================================================

struct ConfigOptionalInt {
    std::optional<int> opt_int;
};

// String instead of int → should FAIL
static_assert(TestParseError<ConfigOptionalInt>(
    R"({"opt_int":"string"})",
    JsonFusion::ParseError::ILLFORMED_NUMBER
));

// Array instead of int → should FAIL
static_assert(TestParseError<ConfigOptionalInt>(
    R"({"opt_int":[]})",
    JsonFusion::ParseError::ILLFORMED_NUMBER
));

// Object instead of int → should FAIL
static_assert(TestParseError<ConfigOptionalInt>(
    R"({"opt_int":{}})",
    JsonFusion::ParseError::ILLFORMED_NUMBER
));

// ============================================================================
// Optional Object
// ============================================================================

struct Inner {
    int x = 10;
    int y = 20;
};

struct ConfigOptionalObject {
    std::optional<Inner> maybe_inner;
};

// Valid: object present
static_assert(TestParse(R"({"maybe_inner":{"x":1,"y":2}})", 
    ConfigOptionalObject{.maybe_inner=Inner{.x=1, .y=2}}));

// Valid: object present with partial fields
static_assert(TestParse(R"({"maybe_inner":{"x":99}})", 
    ConfigOptionalObject{.maybe_inner=Inner{.x=99, .y=20}}));

// Valid: object absent (null)
static_assert(TestParse(R"({"maybe_inner":null})", ConfigOptionalObject{.maybe_inner=std::nullopt}));

// Invalid: wrong type (string instead of object) → should FAIL
static_assert(TestParseError<ConfigOptionalObject>(
    R"({"maybe_inner":"not an object"})",
    JsonFusion::ParseError::NON_MAP_IN_MAP_LIKE_VALUE
));

// ============================================================================
// Optional Array: null vs empty
// ============================================================================

struct ConfigOptionalArray {
    std::optional<std::vector<int>> maybe_array;
};

// Null → nullopt
static_assert(TestParse(R"({"maybe_array":null})", ConfigOptionalArray{std::nullopt}));

// Empty array → empty vector (NOT nullopt)
static_assert(TestParse(R"({"maybe_array":[]})", ConfigOptionalArray{std::vector<int>{}}));

// Non-empty array → vector with values
static_assert(TestParse(R"({"maybe_array":[1,2,3]})", ConfigOptionalArray{std::vector<int>{1, 2, 3}}));

// ============================================================================
// Unique_ptr - Same Semantics as Optional
// ============================================================================

struct ConfigUniquePtr {
    std::unique_ptr<int> maybe_value;
};

// Note: unique_ptr tests cannot use TestParse as it requires copyable types
// We test that the structure compiles and basic semantics work

// Present value
static_assert([]() constexpr {
    ConfigUniquePtr cfg;
    auto r = JsonFusion::Parse(cfg, R"({"maybe_value":42})");
    return r && cfg.maybe_value && *cfg.maybe_value == 42;
}());

// Null value
static_assert([]() constexpr {
    ConfigUniquePtr cfg;
    auto r = JsonFusion::Parse(cfg, R"({"maybe_value":null})");
    return r && !cfg.maybe_value;
}());

// Missing field
static_assert([]() constexpr {
    ConfigUniquePtr cfg;
    auto r = JsonFusion::Parse(cfg, R"({})");
    return r && !cfg.maybe_value;
}());

// ============================================================================
// Optional with Validators
// ============================================================================

struct ConfigOptionalValidated {
    A<std::optional<int>, range<0, 100>> validated_opt;
};

// Valid: value in range
static_assert(TestParse(R"({"validated_opt":50})", ConfigOptionalValidated{50}));

// Valid: absent
static_assert(TestParse(R"({"validated_opt":null})", ConfigOptionalValidated{std::nullopt}));

// Invalid: below range
static_assert(TestParseError<ConfigOptionalValidated>(
    R"({"validated_opt":-1})",
    JsonFusion::ParseError::SCHEMA_VALIDATION_ERROR
));

// Invalid: above range
static_assert(TestParseError<ConfigOptionalValidated>(
    R"({"validated_opt":101})",
    JsonFusion::ParseError::SCHEMA_VALIDATION_ERROR
));

// Boundary: exactly at min
static_assert(TestParse(R"({"validated_opt":0})", ConfigOptionalValidated{0}));

// Boundary: exactly at max
static_assert(TestParse(R"({"validated_opt":100})", ConfigOptionalValidated{100}));

// ============================================================================
// Optional with Required Fields Inside (Struct-Level Validators)
// ============================================================================

struct InnerWithRequired {
    int a;
    int b;
    int c;
};

struct OuterWithOptionalRequired {
    A<std::optional<InnerWithRequired>, required<"a", "b">> maybe_inner;
};

// Valid: null (optional absent)
static_assert(TestParse(R"({"maybe_inner":null})", OuterWithOptionalRequired{std::nullopt}));

// Valid: present with all required fields
static_assert(TestParse(
    R"({"maybe_inner":{"a":1,"b":2,"c":3}})", 
    OuterWithOptionalRequired{InnerWithRequired{1, 2, 3}}
));

// Valid: present with only required fields (c gets default)
static_assert(TestParse(
    R"({"maybe_inner":{"a":1,"b":2}})", 
    OuterWithOptionalRequired{InnerWithRequired{1, 2, 0}}
));

// Invalid: present but missing required field 'a'
static_assert(TestParseError<OuterWithOptionalRequired>(
    R"({"maybe_inner":{"b":2,"c":3}})",
    JsonFusion::ParseError::SCHEMA_VALIDATION_ERROR
));

// Invalid: present but missing required field 'b'
static_assert(TestParseError<OuterWithOptionalRequired>(
    R"({"maybe_inner":{"a":1,"c":3}})",
    JsonFusion::ParseError::SCHEMA_VALIDATION_ERROR
));

