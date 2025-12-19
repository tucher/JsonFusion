#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/validators.hpp>
#include <array>

using namespace TestHelpers;
using namespace JsonFusion;
using namespace JsonFusion::validators;

// ============================================================================
// Map Entry and Consumer (for constexpr testing)
// ============================================================================

template<typename K, typename V>
struct MapEntry {
    K key;
    V value;
};

template<typename K, typename V, std::size_t MaxEntries>
struct MapConsumer {
    using value_type = MapEntry<K, V>;
    
    std::array<MapEntry<K, V>, MaxEntries> entries{};
    std::size_t count = 0;
    
    constexpr bool consume(const MapEntry<K, V>& entry) {
        if (count >= MaxEntries) return false;
        entries[count++] = entry;
        return true;
    }
    
    constexpr bool finalize(bool success) { return success; }
    constexpr void reset() { count = 0; }
};

// ============================================================================
// Test: JSON Path Tracking for Map Errors
// ============================================================================

// Note: Maps require SchemaHasMaps=true, which affects path storage and uses
// inline key buffers. This demonstrates dynamic key tracking in error paths.

// Simple struct with map
struct WithMap {
    int id;
    MapConsumer<std::array<char, 32>, int, 10> data;
};

// Test 1: Error in map value (first entry)
static_assert(
    TestParseErrorWithJsonPath<WithMap>(
        R"({"id": 1, "data": {"key1": "bad", "key2": 20}})",
        JsonIteratorReaderError::ILLFORMED_NUMBER,
        "data", "key1"  // Expected path: $.data."key1"
    ),
    "Map path: error in first entry value"
);

// Test 2: Error in different map entry
static_assert(
    TestParseErrorWithJsonPath<WithMap>(
        R"({"id": 1, "data": {"key1": 10, "key2": null}})",
        ParseError::NULL_IN_NON_OPTIONAL,
        "data", "key2"  // Expected path: $.data."key2"
    ),
    "Map path: error in second entry value"
);

// Test 3: Error with special characters in map key
static_assert(
    TestParseErrorWithJsonPath<WithMap>(
        R"({"id": 1, "data": {"key-with-dash": "bad"}})",
        JsonIteratorReaderError::ILLFORMED_NUMBER,
        "data", "key-with-dash"  // Expected path: $.data."key-with-dash"
    ),
    "Map path: error with special chars in key"
);

// ============================================================================
// Nested Maps
// ============================================================================

struct NestedMapValue {
    int x;
};

struct WithNestedMap {
    MapConsumer<std::array<char, 32>, NestedMapValue, 10> data;
};

// Test 4: Error in nested map value's field
static_assert(
    TestParseErrorWithJsonPath<WithNestedMap>(
        R"({"data": {"item1": {"x": "bad"}}})",
        JsonIteratorReaderError::ILLFORMED_NUMBER,
        "data", "item1", "x"  // Expected path: $.data."item1".x
    ),
    "Nested map: error in value struct field"
);

// ============================================================================
// Map in Nested Struct
// ============================================================================

struct Inner {
    MapConsumer<std::array<char, 32>, int, 10> values;
};

struct OuterWithMap {
    int id;
    Inner inner;
};

// Test 5: Error in map within nested struct
static_assert(
    TestParseErrorWithJsonPath<OuterWithMap>(
        R"({"id": 1, "inner": {"values": {"a": 1, "b": false}}})",
        JsonIteratorReaderError::ILLFORMED_NUMBER,
        "inner", "values", "b"  // Expected path: $.inner.values."b"
    ),
    "Map in nested struct: error path"
);

// ============================================================================
// Map with Array Values
// ============================================================================

struct WithMapOfArrays {
    MapConsumer<std::array<char, 32>, std::array<int, 2>, 10> data;
};

// Test 6: Error in array element within map value
static_assert(
    TestParseErrorWithJsonPath<WithMapOfArrays>(
        R"({"data": {"key1": [1, 2], "key2": [3, "bad"]}})",
        JsonIteratorReaderError::ILLFORMED_NUMBER,
        "data", "key2", 1  // Expected path: $.data."key2"[1]
    ),
    "Map of arrays: error in array element"
);

// ============================================================================
// Using Generic Path Comparison
// ============================================================================

// Test 7: Generic comparison for map error
static_assert(
    TestParseErrorWithJsonPath<WithMap>(
        R"({"id": 1, "data": {"alpha": "bad"}})",
        JsonIteratorReaderError::ILLFORMED_NUMBER,
        "data", "alpha"  // Field name + dynamic map key
    ),
    "Generic path: map key error"
);

// Test 8: Generic comparison for nested map
static_assert(
    TestParseErrorWithJsonPath<WithNestedMap>(
        R"({"data": {"item1": {"x": null}}})",
        ParseError::NULL_IN_NON_OPTIONAL,
        "data", "item1", "x"  // field -> map key -> nested field
    ),
    "Generic path: nested map value error"
);

// Test 9: Map in nested struct with generic comparison
static_assert(
    TestParseErrorWithJsonPath<OuterWithMap>(
        R"({"id": 1, "inner": {"values": {"key": "bad"}}})",
        JsonIteratorReaderError::ILLFORMED_NUMBER,
        "inner", "values", "key"  // struct field -> map field -> map key
    ),
    "Generic path: map in nested struct"
);

// ============================================================================
// Map Key Validation Errors (Note: Tested separately in validation/ tests)
// ============================================================================

// Note: Map key validation (min_key_length, max_key_length, required_keys, etc.)
// is extensively tested in tests/constexpr/validation/test_map_validators.cpp.
// Those tests verify the validation behavior itself.
// 
// Here we focus on verifying that error PATHS are correct when validation fails,
// not on testing the validators themselves.

// ============================================================================
// Unicode Map Keys
// ============================================================================

// Test 10: Error with Unicode map key
static_assert(
    TestParseErrorWithJsonPath<WithMap>(
        R"({"id": 1, "data": {"ключ": "bad", "key": 10}})",
        JsonIteratorReaderError::ILLFORMED_NUMBER,
        "data", "ключ"  // Expected path: $.data."ключ" (Unicode key)
    ),
    "Map path: Unicode key name"
);

// ============================================================================
// Map Key Path Storage
// ============================================================================

// Test 11: Verify map keys are stored in path elements
static_assert(
    TestParseErrorWithJsonPath<WithMap>(
        R"({"id": 1, "data": {"testkey": false}})",
        JsonIteratorReaderError::ILLFORMED_NUMBER,
        "data", "testkey"  // Expected path: $.data."testkey" (key stored in inline buffer)
    ),
    "Map path: verify key stored in path element"
);

// ============================================================================
// Path Depth with Maps
// ============================================================================

// Test 12: Verify correct path depth for map errors
static_assert(
    TestParseErrorWithPathDepth<WithMap>(
        R"({"id": 1, "data": {"key": "bad"}})",
        JsonIteratorReaderError::ILLFORMED_NUMBER,
        2  // "data" + "key"
    ),
    "Map path depth: correct for simple map"
);

// Test 13: Path depth for nested structures with maps
static_assert(
    TestParseErrorWithPathDepth<OuterWithMap>(
        R"({"id": 1, "inner": {"values": {"k": null}}})",
        ParseError::NULL_IN_NON_OPTIONAL,
        3  // "inner" + "values" + "k"
    ),
    "Map path depth: correct for nested map"
);

