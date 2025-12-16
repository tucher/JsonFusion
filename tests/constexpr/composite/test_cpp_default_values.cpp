#include "test_helpers.hpp"
using namespace TestHelpers;
using JsonFusion::A;
using namespace JsonFusion::options;
using namespace JsonFusion::validators;

// ============================================================================
// Struct with Defaults - Empty JSON
// ============================================================================

struct ConfigWithDefaults {
    int value = 42;
    std::string name = "default";
    bool active = true;
};

// Parse empty object - fields should keep C++ defaults
static_assert(TestParse(R"({})", ConfigWithDefaults{42, "default", true}));

// ============================================================================
// Partial JSON - Some Fields Provided
// ============================================================================

struct ConfigPartial {
    int a = 10;
    int b = 20;
    int c = 30;
};

// Provide only 'a' - others should keep defaults
static_assert(TestParse(R"({"a":100})", ConfigPartial{100, 20, 30}));

// Provide only 'b' - others should keep defaults
static_assert(TestParse(R"({"b":200})", ConfigPartial{10, 200, 30}));

// Provide all - overrides defaults
static_assert(TestParse(R"({"a":1,"b":2,"c":3})", ConfigPartial{1, 2, 3}));

// ============================================================================
// Optional Fields - Missing vs Null
// ============================================================================

struct ConfigOptional {
    std::optional<int> maybe_value;
    int required_value = 99;
};

// Missing field → nullopt (NOT default constructed)
static_assert(TestParse(R"({"required_value":42})", ConfigOptional{std::nullopt, 42}));

// Explicit null → nullopt
static_assert(TestParse(R"({"maybe_value":null,"required_value":42})", ConfigOptional{std::nullopt, 42}));

// Provided → has value
static_assert(TestParse(R"({"maybe_value":123,"required_value":42})", ConfigOptional{123, 42}));

// ============================================================================
// Required Fields - Must Fail Even With C++ Defaults
// ============================================================================

struct ConfigRequired_ {
    int required_field = 999;  // Has C++ default
    int optional_field = 888;
};
using ConfigRequired = A<ConfigRequired_, required<"required_field">>;

// Missing required field → should FAIL
static_assert(TestParseError<ConfigRequired>(
    R"({"optional_field":42})",
    JsonFusion::ParseError::SCHEMA_VALIDATION_ERROR
));

// Required field present → should PASS
static_assert(TestParse(R"({"required_field":42})", ConfigRequired{ConfigRequired_{.required_field=42, .optional_field=888}}));

// Both present → should PASS
static_assert(TestParse(R"({"required_field":1,"optional_field":2})", ConfigRequired{ConfigRequired_{.required_field=1, .optional_field=2}}));

// ============================================================================
// Nested Struct Defaults
// ============================================================================

struct Inner {
    int x = 10;
    int y = 20;
};

struct OuterWithNested {
    Inner inner;
    int value = 100;
};

// Empty object - nested struct keeps its defaults
static_assert(TestParse(R"({})", OuterWithNested{Inner{10, 20}, 100}));

// Provide only outer field - inner keeps defaults
static_assert(TestParse(R"({"value":200})", OuterWithNested{Inner{10, 20}, 200}));

// Provide only inner.x - inner.y keeps default
static_assert(TestParse(R"({"inner":{"x":99}})", OuterWithNested{Inner{99, 20}, 100}));

// Provide all
static_assert(TestParse(R"({"inner":{"x":1,"y":2},"value":3})", OuterWithNested{Inner{1, 2}, 3}));

// ============================================================================
// Fixed-Size Array with Defaults
// ============================================================================

struct ConfigArrayDefaults {
    std::array<int, 3> values = {1, 2, 3};
};

// Empty object - array keeps defaults
static_assert(TestParse(R"({})", ConfigArrayDefaults{{1, 2, 3}}));

// Provide array - overrides defaults
static_assert(TestParse(R"({"values":[10,20,30]})", ConfigArrayDefaults{{10, 20, 30}}));

// ============================================================================
// Multiple Levels of Nesting
// ============================================================================

struct Level2 {
    int x = 100;
};

struct Level1 {
    Level2 inner = Level2{200};
    int y = 50;
};

struct Level0 {
    Level1 mid = Level1{Level2{300}, 60};
    int z = 10;
};

// All defaults cascade
static_assert(TestParse(R"({})", Level0{Level1{Level2{300}, 60}, 10}));

// Override only innermost
static_assert(TestParse(R"({"mid":{"inner":{"x":999}}})", Level0{Level1{Level2{999}, 60}, 10}));

// Override only outermost
static_assert(TestParse(R"({"z":777})", Level0{Level1{Level2{300}, 60}, 777}));

