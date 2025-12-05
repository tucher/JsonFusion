#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <memory>
#include <array>

using namespace TestHelpers;

// ============================================================================
// Test: Unique Pointer Primitives
// ============================================================================

// Test 1: std::unique_ptr<int> - null vs present
struct WithUniquePtrInt {
    std::unique_ptr<int> value;
};

static_assert(
    TestParse<WithUniquePtrInt>(R"({"value": 42})", 
        [](const WithUniquePtrInt& obj) {
            return obj.value != nullptr && *obj.value == 42;
        }),
    "Unique ptr int: present value"
);

static_assert(
    TestParse(R"({"value": null})", 
        WithUniquePtrInt{nullptr}),
    "Unique ptr int: null value"
);

// Test 2: std::unique_ptr<bool> - null vs true/false
struct WithUniquePtrBool {
    std::unique_ptr<bool> flag;
};

static_assert(
    TestParse<WithUniquePtrBool>(R"({"flag": true})", 
        [](const WithUniquePtrBool& obj) {
            return obj.flag != nullptr && *obj.flag == true;
        }),
    "Unique ptr bool: true"
);

static_assert(
    TestParse<WithUniquePtrBool>(R"({"flag": false})", 
        [](const WithUniquePtrBool& obj) {
            return obj.flag != nullptr && *obj.flag == false;
        }),
    "Unique ptr bool: false"
);

static_assert(
    TestParse(R"({"flag": null})", 
        WithUniquePtrBool{nullptr}),
    "Unique ptr bool: null"
);

// Test 3: std::unique_ptr<std::array<char, N>> - null vs string
struct WithUniquePtrString {
    std::unique_ptr<std::array<char, 32>> name;
};

static_assert(
    TestParse<WithUniquePtrString>(R"({"name": "Alice"})", 
        [](const WithUniquePtrString& obj) {
            return obj.name != nullptr 
                && (*obj.name)[0] == 'A'
                && (*obj.name)[1] == 'l'
                && (*obj.name)[2] == 'i'
                && (*obj.name)[3] == 'c'
                && (*obj.name)[4] == 'e';
        }),
    "Unique ptr string: present"
);

static_assert(
    TestParse(R"({"name": null})", 
        WithUniquePtrString{nullptr}),
    "Unique ptr string: null"
);

// Test 4: Multiple unique_ptrs in same struct
struct WithMultipleUniquePtrs {
    std::unique_ptr<int> id;
    std::unique_ptr<bool> enabled;
    std::unique_ptr<std::array<char, 16>> tag;
};

static_assert(
    TestParse<WithMultipleUniquePtrs>(R"({"id": 1, "enabled": true, "tag": "test"})", 
        [](const WithMultipleUniquePtrs& obj) {
            return obj.id != nullptr && *obj.id == 1
                && obj.enabled != nullptr && *obj.enabled == true
                && obj.tag != nullptr && (*obj.tag)[0] == 't';
        }),
    "Multiple unique_ptrs: all present"
);

static_assert(
    TestParse(R"({"id": null, "enabled": null, "tag": null})", 
        WithMultipleUniquePtrs{nullptr, nullptr, nullptr}),
    "Multiple unique_ptrs: all null"
);

static_assert(
    TestParse<WithMultipleUniquePtrs>(R"({"id": 42, "enabled": null, "tag": "active"})", 
        [](const WithMultipleUniquePtrs& obj) {
            return obj.id != nullptr && *obj.id == 42
                && obj.enabled == nullptr
                && obj.tag != nullptr && (*obj.tag)[0] == 'a';
        }),
    "Multiple unique_ptrs: mixed present/null"
);

// Test 5: Unique ptr in various positions (first, middle, last field)
struct UniquePtrFirst {
    std::unique_ptr<int> first;
    int second;
    bool third;
};

static_assert(
    TestParse<UniquePtrFirst>(R"({"first": 10, "second": 20, "third": true})", 
        [](const UniquePtrFirst& obj) {
            return obj.first != nullptr && *obj.first == 10
                && obj.second == 20
                && obj.third == true;
        }),
    "Unique ptr in first position"
);

struct UniquePtrMiddle {
    int first;
    std::unique_ptr<int> middle;
    bool third;
};

static_assert(
    TestParse<UniquePtrMiddle>(R"({"first": 10, "middle": 20, "third": true})", 
        [](const UniquePtrMiddle& obj) {
            return obj.first == 10
                && obj.middle != nullptr && *obj.middle == 20
                && obj.third == true;
        }),
    "Unique ptr in middle position"
);

struct UniquePtrLast {
    int first;
    bool second;
    std::unique_ptr<int> last;
};

static_assert(
    TestParse<UniquePtrLast>(R"({"first": 10, "second": true, "last": 30})", 
        [](const UniquePtrLast& obj) {
            return obj.first == 10
                && obj.second == true
                && obj.last != nullptr && *obj.last == 30;
        }),
    "Unique ptr in last position"
);

