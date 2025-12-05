#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/static_schema.hpp>
#include <memory>
#include <array>

using namespace TestHelpers;
using namespace JsonFusion::static_schema;

// ============================================================================
// Test: Unique Pointer Nested Types
// ============================================================================

// Test 1: std::unique_ptr<NestedStruct> - null vs object
struct Inner {
    int value;
    bool flag;
};

struct WithUniquePtrStruct {
    std::unique_ptr<Inner> inner;
};

static_assert(
    TestParse<WithUniquePtrStruct>(R"({"inner": {"value": 42, "flag": true}})", 
        [](const WithUniquePtrStruct& obj) {
            return obj.inner != nullptr
                && obj.inner->value == 42
                && obj.inner->flag == true;
        }),
    "Unique ptr struct: present"
);

static_assert(
    TestParse(R"({"inner": null})", 
        WithUniquePtrStruct{nullptr}),
    "Unique ptr struct: null"
);

// Test 2: Optional arrays: std::unique_ptr<std::array<int, N>>
struct WithUniquePtrArray {
    std::unique_ptr<std::array<int, 3>> values;
};

static_assert(
    TestParse<WithUniquePtrArray>(R"({"values": [1, 2, 3]})", 
        [](const WithUniquePtrArray& obj) {
            return obj.values != nullptr
                && (*obj.values)[0] == 1
                && (*obj.values)[1] == 2
                && (*obj.values)[2] == 3;
        }),
    "Unique ptr array: present"
);

static_assert(
    TestParse(R"({"values": null})", 
        WithUniquePtrArray{nullptr}),
    "Unique ptr array: null"
);

// Test 3: Unique ptr struct with nested optional
struct InnerWithOptional {
    std::optional<int> value;
    bool flag;
};

struct WithNestedUniquePtrStruct {
    std::unique_ptr<InnerWithOptional> inner;
};

static_assert(
    TestParse<WithNestedUniquePtrStruct>(R"({"inner": {"value": 42, "flag": true}})", 
        [](const WithNestedUniquePtrStruct& obj) {
            return obj.inner != nullptr
                && obj.inner->value == 42
                && obj.inner->flag == true;
        }),
    "Unique ptr struct with nested optional: all present"
);

static_assert(
    TestParse<WithNestedUniquePtrStruct>(R"({"inner": {"value": null, "flag": true}})", 
        [](const WithNestedUniquePtrStruct& obj) {
            return obj.inner != nullptr
                && !obj.inner->value.has_value()
                && obj.inner->flag == true;
        }),
    "Unique ptr struct with nested optional: inner null"
);

// Test 4: Complex nested unique_ptr structure
struct Level2 {
    int data;
};

struct Level1 {
    std::unique_ptr<Level2> nested;
    int id;
};

struct Root {
    std::unique_ptr<Level1> level1;
};

static_assert(
    TestParse<Root>(R"({"level1": {"nested": {"data": 100}, "id": 1}})", 
        [](const Root& obj) {
            return obj.level1 != nullptr
                && obj.level1->nested != nullptr
                && obj.level1->nested->data == 100
                && obj.level1->id == 1;
        }),
    "Complex nested unique_ptr structure"
);

static_assert(
    TestParse(R"({"level1": null})", 
        Root{nullptr}),
    "Complex nested unique_ptr: outer null"
);

