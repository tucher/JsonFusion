#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>

using namespace TestHelpers;

// ============================================================================
// Test: Nested Structs - Flat Nesting (No Arrays/Maps)
// ============================================================================

// Test 1: One level of nesting
struct Outer1 {
    int id;
    struct Inner1 {
        int value;
    } inner;
};

static_assert(
    TestParse(R"({"id": 42, "inner": {"value": 100}})", 
        Outer1{42, {100}}),
    "One level of nesting"
);

// Test 2: Two levels of nesting
struct Level2 {
    int data;
};

struct Level1 {
    int id;
    Level2 nested;
};

static_assert(
    TestParse(R"({"id": 1, "nested": {"data": 42}})", 
        Level1{1, {42}}),
    "Two levels of nesting"
);

// Test 3: Three levels of nesting
struct Deep3 {
    int value;
};

struct Deep2 {
    Deep3 deeper;
};

struct Deep1 {
    int id;
    Deep2 middle;
};

static_assert(
    TestParse(R"({"id": 1, "middle": {"deeper": {"value": 999}}})", 
        Deep1{1, {{999}}}),
    "Three levels of nesting"
);

// Test 4: Four levels of nesting
struct L4 {
    int x;
};

struct L3 {
    L4 level4;
};

struct L2 {
    L3 level3;
};

struct L1 {
    int id;
    L2 level2;
};

static_assert(
    TestParse(R"({"id": 1, "level2": {"level3": {"level4": {"x": 42}}}})", 
        L1{1, {{{42}}}}),
    "Four levels of nesting"
);

// Test 5: Multiple nested fields at same level
struct MultiNested {
    int id;
    struct A {
        int a;
    } field_a;
    struct B {
        int b;
    } field_b;
    struct C {
        int c;
    } field_c;
};

static_assert(
    TestParse(R"({
        "id": 1,
        "field_a": {"a": 10},
        "field_b": {"b": 20},
        "field_c": {"c": 30}
    })", 
        MultiNested{1, {10}, {20}, {30}}),
    "Multiple nested structs at same level"
);

// Test 6: Mixed primitive and nested fields
struct Mixed {
    int primitive1;
    struct Inner {
        int nested_value;
    } nested;
    bool primitive2;
    struct Inner2 {
        int nested_int;
    } nested2;
};

static_assert(
    TestParse(R"({
        "primitive1": 1,
        "nested": {"nested_value": 42},
        "primitive2": true,
        "nested2": {"nested_int": 100}
    })", 
        Mixed{1, {42}, true, {100}}),
    "Mixed primitive and nested fields"
);

