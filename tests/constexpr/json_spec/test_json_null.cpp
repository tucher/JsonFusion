#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <optional>
#include <memory>

using namespace TestHelpers;

// ============================================================================
// Test: JSON null Handling (RFC 8259 Compliance)
// ============================================================================

// Test 1: std::optional with null
struct WithOptionalInt {
    std::optional<int> value;
};

static_assert(
    TestParse(R"({"value": null})", WithOptionalInt{std::nullopt}),
    "Null: optional int with null"
);

static_assert(
    TestParse(R"({"value": 42})", WithOptionalInt{42}),
    "Null: optional int with value"
);

static_assert(
    TestParse(R"({"value": 0})", WithOptionalInt{0}),
    "Null: optional int with zero (not null)"
);

// Test 2: std::optional with string
struct WithOptionalString {
    std::optional<std::string> name;
};

static_assert(
    TestParse(R"({"name": null})", WithOptionalString{std::nullopt}),
    "Null: optional string with null"
);

static_assert(
    TestParse(R"({"name": "test"})", WithOptionalString{"test"}),
    "Null: optional string with value"
);

static_assert(
    TestParse(R"({"name": ""})", WithOptionalString{""}),
    "Null: optional string with empty string (not null)"
);

// Test 3: std::optional with nested struct
struct Inner {
    int x;
};

struct WithOptionalStruct {
    std::optional<Inner> inner;
};

static_assert(
    TestParse(R"({"inner": null})", WithOptionalStruct{std::nullopt}),
    "Null: optional struct with null"
);

static_assert(
    TestParse(R"({"inner": {"x": 42}})", WithOptionalStruct{Inner{42}}),
    "Null: optional struct with value"
);

// Test 4: std::unique_ptr with null
struct WithUniquePtr {
    std::unique_ptr<int> value;
};

static_assert(
    TestParse(R"({"value": null})", WithUniquePtr{nullptr}),
    "Null: unique_ptr with null"
);

static_assert(
    TestParse<WithUniquePtr>(R"({"value": 42})", 
        [](const WithUniquePtr& obj) {
            return obj.value != nullptr && *obj.value == 42;
        }),
    "Null: unique_ptr with value"
);

// Test 5: Multiple optional fields, some null
struct MultipleOptionals {
    std::optional<int> a;
    std::optional<std::string> b;
    std::optional<bool> c;
};

static_assert(
    TestParse(R"({"a": null, "b": null, "c": null})", 
        MultipleOptionals{std::nullopt, std::nullopt, std::nullopt}),
    "Null: all optionals null"
);

static_assert(
    TestParse(R"({"a": 1, "b": null, "c": true})", 
        MultipleOptionals{1, std::nullopt, true}),
    "Null: mixed null and values"
);

static_assert(
    TestParse(R"({"a": null, "b": "test", "c": null})", 
        MultipleOptionals{std::nullopt, "test", std::nullopt}),
    "Null: some null, some values"
);

// Test 6: Null in arrays
struct WithOptionalArray {
    std::optional<std::array<int, 3>> values;
};

static_assert(
    TestParse(R"({"values": null})", WithOptionalArray{std::nullopt}),
    "Null: optional array with null"
);

static_assert(
    TestParse<WithOptionalArray>(R"({"values": [1, 2, 3]})", 
        [](const WithOptionalArray& obj) {
            return obj.values.has_value() 
                && (*obj.values)[0] == 1 
                && (*obj.values)[1] == 2 
                && (*obj.values)[2] == 3;
        }),
    "Null: optional array with value"
);

// Test 7: Null in nested structures
struct NestedWithNull {
    int id;
    struct Inner {
        std::optional<int> value;
    } inner;
};

static_assert(
    TestParse(R"({"id": 1, "inner": {"value": null}})", 
        NestedWithNull{1, {std::nullopt}}),
    "Null: nested optional with null"
);

static_assert(
    TestParse(R"({"id": 1, "inner": {"value": 42}})", 
        NestedWithNull{1, {42}}),
    "Null: nested optional with value"
);

// Test 8: Array of optionals
struct WithArrayOfOptionals {
    std::array<std::optional<int>, 3> values;
};

static_assert(
    TestParse(R"({"values": [1, null, 3]})", 
        WithArrayOfOptionals{{1, std::nullopt, 3}}),
    "Null: array of optionals with null element"
);

static_assert(
    TestParse(R"({"values": [null, null, null]})", 
        WithArrayOfOptionals{{std::nullopt, std::nullopt, std::nullopt}}),
    "Null: array of optionals, all null"
);

// Test 9: Vector of optionals
struct WithVectorOfOptionals {
    std::vector<std::optional<int>> values;
};

static_assert(
    TestParse<WithVectorOfOptionals>(R"({"values": [1, null, 3, null, 5]})", 
        [](const WithVectorOfOptionals& obj) {
            return obj.values.size() == 5
                && obj.values[0] == 1
                && !obj.values[1].has_value()
                && obj.values[2] == 3
                && !obj.values[3].has_value()
                && obj.values[4] == 5;
        }),
    "Null: vector of optionals with null elements"
);

// Test 10: Null vs empty string vs zero
struct NullVsEmpty {
    std::optional<int> num;
    std::optional<std::string> str;
    std::optional<bool> flag;
};

static_assert(
    TestParse(R"({"num": null, "str": null, "flag": null})", 
        NullVsEmpty{std::nullopt, std::nullopt, std::nullopt}),
    "Null: all null"
);

static_assert(
    TestParse(R"({"num": 0, "str": "", "flag": false})", 
        NullVsEmpty{0, "", false}),
    "Null: zero/empty/false (not null)"
);

