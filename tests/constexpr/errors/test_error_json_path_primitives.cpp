#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/validators.hpp>

using namespace TestHelpers;
using namespace JsonFusion;
using namespace JsonFusion::validators;

// ============================================================================
// Test: JSON Path Tracking for Primitive Field Errors
// ============================================================================

// Simple flat struct
struct Flat {
    int x;
    bool flag;
    int y;
};

// Test 1: Error in first field - complete path verification
static_assert(
    TestParseErrorWithJsonPath<Flat>(
        R"({"x": "bad", "flag": true, "y": 10})",
        JsonIteratorReaderError::ILLFORMED_NUMBER,
        "x"  // Expected path: $.x
    ),
    "Path tracking: error in first primitive field"
);

// Test 2: Error in middle field
static_assert(
    TestParseErrorWithJsonPath<Flat>(
        R"({"x": 42, "flag": "not_bool", "y": 10})",
        ParseError::NON_BOOL_JSON_IN_BOOL_VALUE,
        "flag"  // Expected path: $.flag
    ),
    "Path tracking: error in middle primitive field"
);

// Test 3: Error in last field
static_assert(
    TestParseErrorWithJsonPath<Flat>(
        R"({"x": 42, "flag": true, "y": [1,2,3]})",
        JsonIteratorReaderError::ILLFORMED_NUMBER,
        "y"  // Expected path: $.y
    ),
    "Path tracking: error in last primitive field"
);

// ============================================================================
// Nested Structs - Path Depth > 1
// ============================================================================

struct Inner {
    int value;
    bool enabled;
};

struct Outer {
    int id;
    Inner inner;
    int count;
};

// Test 4: Error in nested field - 2 levels deep
static_assert(
    TestParseErrorWithJsonPath<Outer>(
        R"({"id": 1, "inner": {"value": "bad", "enabled": true}, "count": 10})",
        JsonIteratorReaderError::ILLFORMED_NUMBER,
        "inner", "value"  // Expected path: $.inner.value
    ),
    "Path tracking: error in nested struct field ($.inner.value)"
);

// Test 5: Error in different nested field
static_assert(
    TestParseErrorWithJsonPath<Outer>(
        R"({"id": 1, "inner": {"value": 42, "enabled": 123}, "count": 10})",
        ParseError::NON_BOOL_JSON_IN_BOOL_VALUE,
        "inner", "enabled"  // Expected path: $.inner.enabled
    ),
    "Path tracking: error in nested bool field ($.inner.enabled)"
);

// ============================================================================
// Deep Nesting - 3+ Levels
// ============================================================================

struct Level3 {
    int data;
};

struct Level2 {
    Level3 deep;
};

struct Level1 {
    Level2 middle;
};

// Test 6: Error at 3rd nesting level
static_assert(
    TestParseErrorWithJsonPath<Level1>(
        R"({"middle": {"deep": {"data": null}}})",
        ParseError::NULL_IN_NON_OPTIONAL,
        "middle", "deep", "data"  // Expected path: $.middle.deep.data
    ),
    "Path tracking: deep nesting ($.middle.deep.data)"
);

// ============================================================================
// Using Helper Functions
// ============================================================================

// Test 7: Using TestParseErrorWithPathDepth helper
static_assert(
    TestParseErrorWithPathDepth<Flat>(
        R"({"x": "bad", "flag": true, "y": 10})",
        JsonIteratorReaderError::ILLFORMED_NUMBER,
        1  // Expected path depth
    ),
    "Helper: TestParseErrorWithPathDepth for primitive field"
);

// Test 8: Using TestParseErrorWithPath helper to verify full path
static_assert(
    TestParseErrorWithPath<Outer>(
        R"({"id": 1, "inner": {"value": "bad", "enabled": true}, "count": 10})",
        JsonIteratorReaderError::ILLFORMED_NUMBER,
        "inner", "value"  // Expected path: $.inner.value
    ),
    "Helper: TestParseErrorWithPath for nested field"
);

// Test 9: Deep path with helper
static_assert(
    TestParseErrorWithPath<Level1>(
        R"({"middle": {"deep": {"data": "bad"}}})",
        JsonIteratorReaderError::ILLFORMED_NUMBER,
        "middle", "deep", "data"  // Expected path: $.middle.deep.data
    ),
    "Helper: TestParseErrorWithPath for deep nesting"
);

// ============================================================================
// Root-Level Errors (Path Depth = 0)
// ============================================================================

// Test 10: Malformed JSON at root (unclosed object)
static_assert([]() constexpr {
    Flat obj{};
    auto result = Parse(obj, std::string_view(R"({"x": 42)"));
    return !result 
        && result.readerError() == JsonFusion::JsonIteratorReaderError::ILLFORMED_OBJECT;
        // Path depth could be 0 or 1 depending on how far parsing got
}(), "Root-level error: unclosed object");

// Test 11: Wrong type at root (array instead of object)
static_assert([]() constexpr {
    Flat obj{};
    auto result = Parse(obj, std::string_view(R"([1, 2, 3])"));
    return !result 
        && result.error() == ParseError::NON_MAP_IN_MAP_LIKE_VALUE;
}(), "Root-level error: wrong container type");

// ============================================================================
// Field Order Independence (Path Reflects JSON Order)
// ============================================================================

// Test 12: Fields in different order - path still correct
static_assert(
    TestParseErrorWithJsonPath<Flat>(
        R"({"y": 10, "x": 42, "flag": "bad"})",
        ParseError::NON_BOOL_JSON_IN_BOOL_VALUE,
        "flag"  // Expected path: $.flag (order-independent)
    ),
    "Path tracking: field order doesn't matter"
);

// ============================================================================
// Validation Errors with Path Tracking
// ============================================================================

struct Validated {
    int id;
    A<bool, constant<true>> flag;
    int other;
};

// Test 13: Validation error includes correct path
static_assert(
    TestValidationErrorWithJsonPath<Validated>(
        R"({"id": 1, "flag": false, "other": 42})",
        "flag"  // Expected path: $.flag
    ),
    "Path tracking: validation error (constant violation)"
);

struct NestedValidated {
    int id;
    struct Inner {
        int x;
        A<bool, constant<true>> enabled;
    } inner;
};

// Test 14: Nested validation error
static_assert(
    TestValidationErrorWithJsonPath<NestedValidated>(
        R"({"id": 1, "inner": {"x": 10, "enabled": false}})",
        "inner", "enabled"  // Expected path: $.inner.enabled
    ),
    "Path tracking: nested validation error"
);

// ============================================================================
// Using New Generic JsonPath Comparison (Cleaner API)
// ============================================================================

// Test 15: Compare paths generically - simple field error
static_assert(
    TestParseErrorWithJsonPath<Flat>(
        R"({"x": "bad", "flag": true, "y": 10})",
        JsonIteratorReaderError::ILLFORMED_NUMBER,
        "x"  // Expected path: $.x
    ),
    "Generic path comparison: simple field"
);

// Test 16: Generic comparison - nested path
static_assert(
    TestParseErrorWithJsonPath<Outer>(
        R"({"id": 1, "inner": {"value": "bad", "enabled": true}, "count": 10})",
        JsonIteratorReaderError::ILLFORMED_NUMBER,
        "inner", "value"  // Expected path: $.inner.value
    ),
    "Generic path comparison: nested field"
);

// Test 17: Generic comparison - deep nesting
static_assert(
    TestParseErrorWithJsonPath<Level1>(
        R"({"middle": {"deep": {"data": "bad"}}})",
        JsonIteratorReaderError::ILLFORMED_NUMBER,
        "middle", "deep", "data"  // Expected path: $.middle.deep.data
    ),
    "Generic path comparison: deep nesting"
);

// Test 18: Generic validation error comparison
static_assert(
    TestValidationErrorWithJsonPath<Validated>(
        R"({"id": 1, "flag": false, "other": 42})",
        "flag"  // Expected path: $.flag
    ),
    "Generic path comparison: validation error"
);

