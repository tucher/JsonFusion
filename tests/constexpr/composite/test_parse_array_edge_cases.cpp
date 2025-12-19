#include "test_helpers.hpp"
using namespace TestHelpers;
using JsonFusion::A;
using namespace JsonFusion::validators;

// ============================================================================
// Empty Array with Validators
// ============================================================================

struct ConfigEmptyArrayValidators {
    A<std::vector<int>, min_items<0>, max_items<0>> values;
};

// Empty array - satisfies both min/max of 0
static_assert(TestParse(R"({"values":[]})", ConfigEmptyArrayValidators{{}}));

// Non-empty array - should FAIL (violates max_items<0>)
static_assert(TestParseError<ConfigEmptyArrayValidators>(
    R"({"values":[1]})",
    JsonFusion::ParseError::SCHEMA_VALIDATION_ERROR
));

// ============================================================================
// Array Overflow - Fixed Size Array
// ============================================================================

struct ConfigFixedArray {
    std::array<int, 2> values;
};

// Exactly 2 elements - should PASS
static_assert(TestParse(R"({"values":[1,2]})", ConfigFixedArray{{1, 2}}));

// Only 1 element - should PASS (rest default-initialized)
static_assert(TestParse(R"({"values":[1]})", ConfigFixedArray{{1, 0}}));

// 3 elements - should FAIL (overflow)
static_assert(TestParseError<ConfigFixedArray>(
    R"({"values":[1,2,3]})",
    JsonFusion::ParseError::FIXED_SIZE_CONTAINER_OVERFLOW
));

// ============================================================================
// Invalid JSON - Comma Errors
// ============================================================================

struct ConfigIntArray {
    std::vector<int> values;
};

// Trailing comma - invalid JSON
static_assert(TestParseError<ConfigIntArray>(
    R"({"values":[1,2,]})",
    JsonFusion::ParseError::ILLFORMED_ARRAY
));

// Leading comma - invalid JSON (parser sees ',1' as malformed number)
static_assert(TestParseError<ConfigIntArray>(
    R"({"values":[,1,2]})",
    JsonFusion::ParseError::ILLFORMED_ARRAY
));

// Double comma - invalid JSON (parser sees ',2' as malformed number)
static_assert(TestParseError<ConfigIntArray>(
    R"({"values":[1,,2]})",
    JsonFusion::ParseError::ILLFORMED_ARRAY
));

// ============================================================================
// Array of Empty Strings
// ============================================================================

struct ConfigStringArray {
    std::vector<std::string> items;
};

// Multiple empty strings
static_assert(TestParse(R"({"items":["","",""]})", ConfigStringArray{{"", "", ""}}));

// Mixed empty and non-empty
static_assert(TestParse(R"({"items":["","hello",""]})", ConfigStringArray{{"", "hello", ""}}));

// ============================================================================
// Array with Mixed Valid/Invalid Elements
// ============================================================================

// Valid elements
static_assert(TestParse(R"({"values":[1,2,3]})", ConfigIntArray{{1, 2, 3}}));

// One invalid element (string in int array) - should FAIL
static_assert(TestParseError<ConfigIntArray>(
    R"({"values":[1,"bad",3]})",
    JsonFusion::ParseError::ILLFORMED_NUMBER
));

// ============================================================================
// Array Size Boundaries - Fixed Array
// ============================================================================

struct ConfigBoundary3 {
    std::array<int, 3> values;
};

// N-1 elements (2 elements for size 3) - should PASS
static_assert(TestParse(R"({"values":[10,20]})", ConfigBoundary3{{10, 20, 0}}));

// Exactly N elements (3 elements for size 3) - should PASS
static_assert(TestParse(R"({"values":[10,20,30]})", ConfigBoundary3{{10, 20, 30}}));

// N+1 elements (4 elements for size 3) - should FAIL
static_assert(TestParseError<ConfigBoundary3>(
    R"({"values":[10,20,30,40]})",
    JsonFusion::ParseError::FIXED_SIZE_CONTAINER_OVERFLOW
));

// ============================================================================
// Array Size Boundaries - Vector with Validators
// ============================================================================

struct ConfigBoundaryVector {
    A<std::vector<int>, min_items<2>, max_items<4>> values;
};

// Below minimum (1 element) - should FAIL
static_assert(TestParseError<ConfigBoundaryVector>(
    R"({"values":[1]})",
    JsonFusion::ParseError::SCHEMA_VALIDATION_ERROR
));

// At minimum (2 elements) - should PASS
static_assert(TestParse(R"({"values":[1,2]})", ConfigBoundaryVector{std::vector<int>{1, 2}}));

// Between min and max (3 elements) - should PASS
static_assert(TestParse(R"({"values":[1,2,3]})", ConfigBoundaryVector{std::vector<int>{1, 2, 3}}));

// At maximum (4 elements) - should PASS
static_assert(TestParse(R"({"values":[1,2,3,4]})", ConfigBoundaryVector{std::vector<int>{1, 2, 3, 4}}));

// Above maximum (5 elements) - should FAIL
static_assert(TestParseError<ConfigBoundaryVector>(
    R"({"values":[1,2,3,4,5]})",
    JsonFusion::ParseError::SCHEMA_VALIDATION_ERROR
));

