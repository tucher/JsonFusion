#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/static_schema.hpp>
#include <optional>
#include <array>

using namespace TestHelpers;
using namespace JsonFusion::static_schema;

// ============================================================================
// Test: Optional Nested Types
// ============================================================================

// Negative test: Verify nested optionals are rejected by JsonParsableValue concept
static_assert(
    !JsonParsableValue<std::optional<std::optional<int>>>,
    "Nested optional (optional<optional<int>>) should be rejected"
);

static_assert(
    !JsonParsableValue<std::optional<std::optional<bool>>>,
    "Nested optional (optional<optional<bool>>) should be rejected"
);

// Positive test: Single optional is accepted
static_assert(
    JsonParsableValue<std::optional<int>>,
    "Single optional should be accepted"
);

static_assert(
    JsonParsableValue<std::optional<bool>>,
    "Single optional<bool> should be accepted"
);

// Test 1: std::optional<NestedStruct> - null vs object
struct Inner {
    int value;
    bool flag;
};

struct WithOptionalStruct {
    std::optional<Inner> inner;
};

static_assert(
    TestParse<WithOptionalStruct>(R"({"inner": {"value": 42, "flag": true}})", 
        [](const WithOptionalStruct& obj) {
            return obj.inner.has_value()
                && obj.inner->value == 42
                && obj.inner->flag == true;
        }),
    "Optional struct: present"
);

static_assert(
    TestParse(R"({"inner": null})", 
        WithOptionalStruct{std::nullopt}),
    "Optional struct: null"
);

// Test 2: Optional arrays: std::optional<std::array<int, N>>
struct WithOptionalArray {
    std::optional<std::array<int, 3>> values;
};

static_assert(
    TestParse<WithOptionalArray>(R"({"values": [1, 2, 3]})", 
        [](const WithOptionalArray& obj) {
            return obj.values.has_value()
                && (*obj.values)[0] == 1
                && (*obj.values)[1] == 2
                && (*obj.values)[2] == 3;
        }),
    "Optional array: present"
);

static_assert(
    TestParse(R"({"values": null})", 
        WithOptionalArray{std::nullopt}),
    "Optional array: null"
);

// Test 3: Optional struct with nested optional
struct InnerWithOptional {
    std::optional<int> value;
    bool flag;
};

struct WithNestedOptionalStruct {
    std::optional<InnerWithOptional> inner;
};

static_assert(
    TestParse<WithNestedOptionalStruct>(R"({"inner": {"value": 42, "flag": true}})", 
        [](const WithNestedOptionalStruct& obj) {
            return obj.inner.has_value()
                && obj.inner->value == 42
                && obj.inner->flag == true;
        }),
    "Optional struct with nested optional: all present"
);

static_assert(
    TestParse<WithNestedOptionalStruct>(R"({"inner": {"value": null, "flag": true}})", 
        [](const WithNestedOptionalStruct& obj) {
            return obj.inner.has_value()
                && !obj.inner->value.has_value()
                && obj.inner->flag == true;
        }),
    "Optional struct with nested optional: inner null"
);

// Test 4: Complex nested optional structure
struct Level2 {
    int data;
};

struct Level1 {
    std::optional<Level2> nested;
    int id;
};

struct Root {
    std::optional<Level1> level1;
};

static_assert(
    TestParse<Root>(R"({"level1": {"nested": {"data": 100}, "id": 1}})", 
        [](const Root& obj) {
            return obj.level1.has_value()
                && obj.level1->nested.has_value()
                && obj.level1->nested->data == 100
                && obj.level1->id == 1;
        }),
    "Complex nested optional structure"
);

static_assert(
    TestParse(R"({"level1": null})", 
        Root{std::nullopt}),
    "Complex nested optional: outer null"
);

