#include "test_helpers.hpp"
using namespace TestHelpers;
using JsonFusion::A;
using namespace JsonFusion::options;

// ============================================================================
// Empty Struct
// ============================================================================

struct Empty {};

static_assert(TestParse(R"({})", Empty{}));
static_assert(TestSerialize(Empty{}, R"({})"));
static_assert(TestRoundTrip<Empty>(R"({})", Empty{}));

// ============================================================================
// Struct with only exclude fields (effectively empty)
// ============================================================================

struct OnlyNotJson {
    A<int, exclude> internal1;
    A<std::string, exclude> internal2;
};

// Should serialize to empty object
static_assert(TestSerialize(OnlyNotJson{42, "test"}, R"({})"));

// Should parse from empty object
static_assert(TestParse(R"({})", OnlyNotJson{0, ""}));

// Roundtrip
static_assert(TestRoundTrip<OnlyNotJson>(R"({})", OnlyNotJson{0, ""}));

// ============================================================================
// Empty Vector (dynamic array with zero elements)
// ============================================================================

struct ConfigEmptyVector {
    std::vector<int> values;
};

static_assert(TestParse(R"({"values":[]})", ConfigEmptyVector{{}}));
static_assert(TestSerialize(ConfigEmptyVector{{}}, R"({"values":[]})"));
static_assert(TestRoundTrip<ConfigEmptyVector>(R"({"values":[]})", ConfigEmptyVector{{}}));

// ============================================================================
// Array of Empty Structs
// ============================================================================

struct ConfigEmptyStructArray {
    std::vector<Empty> empties;
};

// Multiple empty structs in array
static_assert(TestParse(R"({"empties":[{},{},{}]})", ConfigEmptyStructArray{{Empty{}, Empty{}, Empty{}}}));
static_assert(TestSerialize(ConfigEmptyStructArray{{Empty{}, Empty{}, Empty{}}}, R"({"empties":[{},{},{}]})"));

// Empty array of empty structs
static_assert(TestParse(R"({"empties":[]})", ConfigEmptyStructArray{{}}));
static_assert(TestSerialize(ConfigEmptyStructArray{{}}, R"({"empties":[]})"));

// ============================================================================
// Optional of Empty Struct
// ============================================================================

struct ConfigOptionalEmpty {
    std::optional<Empty> maybe_empty;
};

// Present (but empty)
static_assert(TestParse(R"({"maybe_empty":{}})", ConfigOptionalEmpty{Empty{}}));
static_assert(TestSerialize(ConfigOptionalEmpty{Empty{}}, R"({"maybe_empty":{}})"));

// Absent (null)
static_assert(TestParse(R"({"maybe_empty":null})", ConfigOptionalEmpty{std::nullopt}));
static_assert(TestSerialize(ConfigOptionalEmpty{std::nullopt}, R"({"maybe_empty":null})"));

// Missing field
static_assert(TestParse(R"({})", ConfigOptionalEmpty{std::nullopt}));

// ============================================================================
// Nested Empty Structs
// ============================================================================

struct OuterEmpty {
    Empty e1;
    Empty e2;
    Empty e3;
};

// All empty, but still nested (field names still appear)
static_assert(TestParse(R"({"e1":{},"e2":{},"e3":{}})", OuterEmpty{Empty{}, Empty{}, Empty{}}));
static_assert(TestSerialize(OuterEmpty{Empty{}, Empty{}, Empty{}}, R"({"e1":{},"e2":{},"e3":{}})"));
static_assert(TestRoundTrip<OuterEmpty>(R"({"e1":{},"e2":{},"e3":{}})", OuterEmpty{Empty{}, Empty{}, Empty{}}));

// ============================================================================
// Mixed: Empty with Non-Empty Fields
// ============================================================================

struct MixedEmpty {
    Empty e1;
    int value;
    Empty e2;
    std::string name;
    Empty e3;
};

static_assert(TestParse(R"({"e1":{},"value":42,"e2":{},"name":"test","e3":{}})", MixedEmpty{Empty{}, 42, Empty{}, "test", Empty{}}));
static_assert(TestSerialize(MixedEmpty{Empty{}, 42, Empty{}, "test", Empty{}}, R"({"e1":{},"value":42,"e2":{},"name":"test","e3":{}})"));

// ============================================================================
// Empty String (edge case for completeness)
// ============================================================================

struct ConfigEmptyString {
    std::string name;
};

static_assert(TestParse(R"({"name":""})", ConfigEmptyString{""}));
static_assert(TestSerialize(ConfigEmptyString{""}, R"({"name":""})"));
static_assert(TestRoundTrip<ConfigEmptyString>(R"({"name":""})", ConfigEmptyString{""}));

