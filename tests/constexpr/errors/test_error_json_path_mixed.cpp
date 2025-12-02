#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <array>

using namespace TestHelpers;
using namespace JsonFusion;

// ============================================================================
// Test: JSON Path Tracking for Complex Real-World Structures
// ============================================================================

// Realistic nested structure (inspired by twitter.json)
struct Url {
    std::array<char, 128> display_url;
    std::array<char, 256> expanded_url;
};

struct Entities {
    std::array<Url, 3> urls;
};

struct User {
    int id;
    std::array<char, 64> name;
    std::array<char, 64> screen_name;
};

struct Status {
    int id;
    std::array<char, 512> text;
    User user;
    Entities entities;
};

struct TwitterData {
    std::array<Status, 3> statuses;
};

// Test 1: Deep nested path - error in user name within status  
static_assert([]() constexpr {
    TwitterData data{};
    auto result = Parse(data, std::string_view(R"({
        "statuses": [
            {
                "id": 1,
                "text": "First tweet",
                "user": {"id": 10, "name": null, "screen_name": "alice"},
                "entities": {"urls": []}
            }
        ]
    })"));
    
    return !result
        && result.error() == ParseError::NULL_IN_NON_OPTIONAL;
        // Path includes: statuses -> [0] -> user -> name
}(), "Complex path: error in nested struct within array");

// Test 2: Error deep in URL array
static_assert([]() constexpr {
    TwitterData data{};
    auto result = Parse(data, std::string_view(R"({
        "statuses": [
            {
                "id": 1,
                "text": "Tweet",
                "user": {"id": 10, "name": "Alice", "screen_name": "alice"},
                "entities": {"urls": [
                    {"display_url": "test.com", "expanded_url": null}
                ]}
            }
        ]
    })"));
    
    return !result
        && result.error() == ParseError::NULL_IN_NON_OPTIONAL;
        // Path includes: statuses -> [0] -> entities -> urls -> [0] -> expanded_url
}(), "Complex path: 5-level deep nested error");

// Test 3: Using generic path comparison for complex structure
static_assert(
    TestParseErrorWithJsonPath<TwitterData>(
        R"({
            "statuses": [
                {
                    "id": 1,
                    "text": "Tweet",
                    "user": {"id": 10, "name": "Alice", "screen_name": "alice"},
                    "entities": {"urls": [
                        {"display_url": "ok.com", "expanded_url": "http://ok.com"},
                        {"display_url": "test.com", "expanded_url": false}
                    ]}
                }
            ]
        })",
        ParseError::NON_STRING_IN_STRING_STORAGE,
        "statuses", 0, "entities", "urls", 1, "expanded_url"
    ),
    "Generic path: complex 6-level path"
);

// ============================================================================
// Arrays of Maps (if using map streamers)
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

struct Item {
    int id;
    MapConsumer<std::array<char, 32>, int, 5> metadata;
};

struct ItemList {
    std::array<Item, 3> items;
};

// Test 4: Array of structs with maps - error in map value
static_assert(
    TestParseErrorWithJsonPath<ItemList>(
        R"({
            "items": [
                {"id": 1, "metadata": {"key1": 10}},
                {"id": 2, "metadata": {"key2": "bad"}},
                {"id": 3, "metadata": {"key3": 30}}
            ]
        })",
        ParseError::ILLFORMED_NUMBER,
        "items", 1, "metadata", "key2"  // Expected path: $.items[1].metadata."key2"
    ),
    "Generic path: array element error"
);

// Test 5: Generic comparison for array-map-value path
static_assert(
    TestParseErrorWithJsonPath<ItemList>(
        R"({"items": [{"id": 1, "metadata": {"a": 1}}, {"id": 2, "metadata": {"b": null}}]})",
        ParseError::NULL_IN_NON_OPTIONAL,
        "items", 1, "metadata", "b"
    ),
    "Generic path: array[1] -> struct.map -> key"
);

// ============================================================================
// Maps of Arrays
// ============================================================================

struct MapOfArrays {
    MapConsumer<std::array<char, 32>, std::array<int, 2>, 5> data;
};

// Test 6: Map values are arrays - error in array element
static_assert(
    TestParseErrorWithJsonPath<MapOfArrays>(
        R"({
            "data": {
                "first": [1, 2],
                "second": [3, "bad"]
            }
        })",
        ParseError::ILLFORMED_NUMBER,
        "data", "second", 1  // Expected path: $.data."second"[1]
    ),
    "Mixed: map -> array -> element"
);

// Test 7: Generic path for map-array-element
static_assert(
    TestParseErrorWithJsonPath<MapOfArrays>(
        R"({"data": {"key": [10, null]}})",
        ParseError::NULL_IN_NON_OPTIONAL,
        "data", "key", 1
    ),
    "Generic path: map.key[1]"
);

// ============================================================================
// Deeply Nested Mixed Structures
// ============================================================================

struct Handler {
    std::array<char, 32> type;
    std::array<char, 128> path;
};

struct Config {
    struct Database {
        std::array<char, 64> host;
        struct Pool {
            int min_connections;
            int max_connections;
        } pool;
    } database;
    
    struct Logging {
        std::array<char, 16> level;
        std::array<Handler, 2> handlers;
    } logging;
};

// Test 8: Error in deeply nested config structure
static_assert(
    TestParseErrorWithJsonPath<Config>(
        R"({
            "database": {
                "host": "localhost",
                "pool": {"min_connections": 5, "max_connections": "bad"}
            },
            "logging": {
                "level": "info",
                "handlers": []
            }
        })",
        ParseError::ILLFORMED_NUMBER,
        "database", "pool", "max_connections"  // Expected path: $.database.pool.max_connections
    ),
    "Complex config: nested struct path"
);

// Test 9: Error in array within nested config
static_assert(
    TestParseErrorWithJsonPath<Config>(
       R"({
        "database": {
            "host": "localhost",
            "pool": {"min_connections": 5, "max_connections": 10}
        },
        "logging": {
            "level": "info",
            "handlers": [
                {"type": "file", "path": "/var/log/app.log"},
                {"type": "console", "path": null}
            ]
        }
        })",
        ParseError::NULL_IN_NON_OPTIONAL,
        "logging", "handlers", 1, "path"
    ),
    "Generic path: config.logging.handlers[0].path"
);

// Test 10: Generic path for complex config error
static_assert(
    TestParseErrorWithJsonPath<Config>(
        R"({
            "database": {"host": "localhost", "pool": {"min_connections": 5, "max_connections": 10}},
            "logging": {"level": "info", "handlers": [{"type": "file", "path": null}]}
        })",
        ParseError::NULL_IN_NON_OPTIONAL,
        "logging", "handlers", 0, "path"
    ),
    "Generic path: config.logging.handlers[0].path"
);

// ============================================================================
// Path Depth Verification for Mixed Structures
// ============================================================================

// Note: Exact path depth can vary based on how the library counts elements.
// The important thing is that paths are correctly constructed and usable.
// Individual path depth tests are in other test files.

// Test 13: Comprehensive path - all path element types
// This demonstrates: struct field -> array index -> struct field -> array index -> struct field
static_assert(
    TestParseErrorWithJsonPath<TwitterData>(
        R"({"statuses": [{"id": 1, "text": "t", "user": {"id": 1, "name": null, "screen_name": "s"}, 
            "entities": {"urls": []}}]})",
        ParseError::NULL_IN_NON_OPTIONAL,
        "statuses", 0, "user", "name"
    ),
    "Comprehensive: field->[index]->field->field path"
);

