#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>
#include <array>
#include <string_view>

using namespace JsonFusion;
using namespace JsonFusion::static_schema;

// ============================================================================
// Map Entry Structure
// ============================================================================

/// Simple key-value pair structure for map entries
/// Required by ConsumingMapStreamerLike/ProducingMapStreamerLike:
/// - Must have .key member (string-like type)
/// - Must have .value member (any JSON value type)
template<typename K, typename V>
struct MapEntry {
    K key;
    V value;
    
    constexpr bool operator==(const MapEntry& other) const {
        return key == other.key && value == other.value;
    }
};

// ============================================================================
// Map Consumer - High-Level Interface (ConsumingMapStreamerLike)
// ============================================================================

/// High-level map consumer for parsing - NO cursor specialization needed!
/// The library automatically provides map_write_cursor for this type.
template<typename K, typename V, std::size_t MaxEntries>
struct MapConsumer {
    using value_type = MapEntry<K, V>;  // Required: entry type with .key and .value
    
    std::array<MapEntry<K, V>, MaxEntries> entries{};
    std::size_t count = 0;
    bool duplicate_found = false;
    
    // Required method: consume one entry
    constexpr bool consume(const MapEntry<K, V>& entry) {
        // Check for duplicates
        for (std::size_t i = 0; i < count; ++i) {
            bool equal = true;
            for (std::size_t j = 0; j < entry.key.size(); ++j) {
                if (j >= entries[i].key.size() || entries[i].key[j] != entry.key[j]) {
                    equal = false;
                    break;
                }
                if (entries[i].key[j] == '\0' || entry.key[j] == '\0') {
                    equal = (entries[i].key[j] == entry.key[j]);
                    break;
                }
            }
            if (equal) {
                duplicate_found = true;
                return false;  // Reject duplicate
            }
        }
        
        if (count >= MaxEntries) {
            return false;  // Overflow
        }
        
        entries[count++] = entry;
        return true;
    }
    
    // Required method: finalize parsing
    constexpr bool finalize(bool success) {
        return success && !duplicate_found;
    }
    
    // Required method: reset state
    constexpr void reset() {
        count = 0;
        duplicate_found = false;
    }
    
    // Helper: Find entry by key
    constexpr const V* find(const K& key) const {
        for (std::size_t i = 0; i < count; ++i) {
            bool equal = true;
            for (std::size_t j = 0; j < key.size(); ++j) {
                if (j >= entries[i].key.size() || entries[i].key[j] != key[j]) {
                    equal = false;
                    break;
                }
                if (entries[i].key[j] == '\0' || key[j] == '\0') {
                    equal = (entries[i].key[j] == key[j]);
                    break;
                }
            }
            if (equal) return &entries[i].value;
        }
        return nullptr;
    }
};

// Verify the high-level interface is recognized
static_assert(ConsumingMapStreamerLike<MapConsumer<std::array<char, 32>, int, 10>>);
static_assert(ParsableMapLike<MapConsumer<std::array<char, 32>, int, 10>>);

// ============================================================================
// Map Producer - High-Level Interface (ProducingMapStreamerLike)
// ============================================================================

/// High-level map producer for serialization - NO cursor specialization needed!
/// The library automatically provides map_read_cursor for this type.
template<typename K, typename V, std::size_t N>
struct MapProducer {
    using value_type = MapEntry<K, V>;  // Required: entry type with .key and .value
    
    const std::array<MapEntry<K, V>, N>* entries;
    std::size_t count;
    mutable std::size_t index = 0;
    
    constexpr MapProducer(const std::array<MapEntry<K, V>, N>& arr, std::size_t cnt)
        : entries(&arr), count(cnt), index(0) {}
    
    // Required method: read next entry
    constexpr stream_read_result read(MapEntry<K, V>& entry) const {
        if (index >= count) {
            return stream_read_result::end;
        }
        
        entry = (*entries)[index++];
        return stream_read_result::value;
    }
    
    // Required method: reset state
    constexpr void reset() const {
        index = 0;
    }
};

// Verify the high-level interface is recognized
static_assert(ProducingMapStreamerLike<MapProducer<std::array<char, 32>, int, 10>>);
static_assert(JsonSerializableMap<MapProducer<std::array<char, 32>, int, 10>>);

// ============================================================================
// Test: Parse Simple String->Int Map
// ============================================================================

constexpr bool test_parse_simple_string_int_map() {
    MapConsumer<std::array<char, 32>, int, 5> consumer;
    
    std::string_view json = R"({"a": 1, "b": 2, "c": 3})";
    
    auto result = Parse(consumer, json);
    if (!result) return false;
    if (consumer.count != 3) return false;
    
    // Check entries (order may vary)
    auto a_val = consumer.find(std::array<char, 32>{"a"});
    auto b_val = consumer.find(std::array<char, 32>{"b"});
    auto c_val = consumer.find(std::array<char, 32>{"c"});
    
    if (!a_val || *a_val != 1) return false;
    if (!b_val || *b_val != 2) return false;
    if (!c_val || *c_val != 3) return false;
    
    return true;
}
static_assert(test_parse_simple_string_int_map());

// ============================================================================
// Test: Parse Empty Map
// ============================================================================

constexpr bool test_parse_empty_map() {
    MapConsumer<std::array<char, 32>, int, 5> consumer;
    
    std::string_view json = R"({})";
    
    auto result = Parse(consumer, json);
    if (!result) return false;
    if (consumer.count != 0) return false;
    
    return true;
}
static_assert(test_parse_empty_map());

// ============================================================================
// Test: Parse Map with String Values
// ============================================================================

constexpr bool test_parse_map_string_values() {
    MapConsumer<std::array<char, 32>, std::array<char, 32>, 5> consumer;
    
    std::string_view json = R"({"greeting": "hello", "name": "world"})";
    
    auto result = Parse(consumer, json);
    if (!result) return false;
    if (consumer.count != 2) return false;
    
    auto greeting = consumer.find(std::array<char, 32>{"greeting"});
    auto name = consumer.find(std::array<char, 32>{"name"});
    
    if (!greeting || !TestHelpers::CStrEqual(*greeting, "hello")) return false;
    if (!name || !TestHelpers::CStrEqual(*name, "world")) return false;
    
    return true;
}
static_assert(test_parse_map_string_values());

// ============================================================================
// Test: Parse Nested Map (Map of Maps)
// ============================================================================

// Inner map consumer for nested maps
template<typename K, typename V, std::size_t MaxEntries>
struct InnerMapConsumer {
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

// Verify inner map also uses high-level interface
static_assert(ConsumingMapStreamerLike<InnerMapConsumer<std::array<char, 16>, int, 3>>);

constexpr bool test_parse_nested_map() {
    using InnerMap = InnerMapConsumer<std::array<char, 16>, int, 3>;
    MapConsumer<std::array<char, 16>, InnerMap, 3> outer;
    
    std::string_view json = R"({"m1": {"a": 1, "b": 2}, "m2": {"c": 3}})";
    
    auto result = Parse(outer, json);
    if (!result) return false;
    if (outer.count != 2) return false;
    
    // Find m1
    const auto* m1 = outer.find(std::array<char, 16>{"m1"});
    if (!m1 || m1->count != 2) return false;
    
    // Find m2
    const auto* m2 = outer.find(std::array<char, 16>{"m2"});
    if (!m2 || m2->count != 1) return false;
    
    return true;
}
static_assert(test_parse_nested_map());

// ============================================================================
// Test: Duplicate Key Detection
// ============================================================================

constexpr bool test_duplicate_key_error() {
    MapConsumer<std::array<char, 32>, int, 5> consumer;
    
    std::string_view json = R"({"key": 1, "key": 2})";
    
    auto result = Parse(consumer, json);
    
    // Should fail - finalize returns false due to duplicate
    if (result) return false;
    
    return true;
}
static_assert(test_duplicate_key_error());

// ============================================================================
// Test: Map Overflow
// ============================================================================

constexpr bool test_map_overflow() {
    MapConsumer<std::array<char, 16>, int, 2> consumer;  // Only 2 slots
    
    std::string_view json = R"({"a": 1, "b": 2, "c": 3})";  // 3 entries
    
    auto result = Parse(consumer, json);
    
    // Should fail due to overflow
    return !result;
}
static_assert(test_map_overflow());

// ============================================================================
// Test: Serialize Simple Map
// ============================================================================

constexpr bool test_serialize_simple_map() {
    std::array<MapEntry<std::array<char, 8>, int>, 3> entries{{
        {{"x"}, 10},
        {{"y"}, 20},
        {{"z"}, 30}
    }};
    
    MapProducer<std::array<char, 8>, int, 3> producer(entries, 3);
    

    std::string output;
    auto result = Serialize(producer, output);
    if (!result) return false;
    
    
    // Check that output is valid JSON object
    if (output.empty() || output.front() != '{' || output.back() != '}') return false;
    
    // Check that all keys are present
    if (output.find("\"x\"") == std::string_view::npos) return false;
    if (output.find("\"y\"") == std::string_view::npos) return false;
    if (output.find("\"z\"") == std::string_view::npos) return false;
    
    return true;
}
static_assert(test_serialize_simple_map());

// ============================================================================
// Test: Serialize Empty Map
// ============================================================================

constexpr bool test_serialize_empty_map() {
    std::array<MapEntry<std::array<char, 8>, int>, 1> entries{};
    
    MapProducer<std::array<char, 8>, int, 1> producer(entries, 0);  // Count is 0
    
    std::string output;
    auto result = Serialize(producer, output);
    if (!result) return false;
    
    return output == "{}";
}
static_assert(test_serialize_empty_map());

// ============================================================================
// Test: Round-Trip String->Int Map
// ============================================================================

constexpr bool test_roundtrip_map() {
    // Serialize
    std::array<MapEntry<std::array<char, 8>, int>, 2> entries{{
        {{"a"}, 100},
        {{"b"}, 200}
    }};
    
    MapProducer<std::array<char, 8>, int, 2> producer(entries, 2);
    
    std::string output;
    auto ser_result = Serialize(producer, output);
    if (!ser_result) return false;
    
    // Parse back
    std::string_view json(output.data(), output.size());
    MapConsumer<std::array<char, 8>, int, 5> consumer;
    
    auto parse_result = Parse(consumer, json);
    if (!parse_result) return false;
    if (consumer.count != 2) return false;
    
    // Verify values
    auto a_val = consumer.find(std::array<char, 8>{"a"});
    auto b_val = consumer.find(std::array<char, 8>{"b"});
    
    if (!a_val || *a_val != 100) return false;
    if (!b_val || *b_val != 200) return false;
    
    return true;
}
static_assert(test_roundtrip_map());

// ============================================================================
// Test: Map with Boolean Values
// ============================================================================

constexpr bool test_map_bool_values() {
    MapConsumer<std::array<char, 16>, bool, 3> consumer;
    
    std::string_view json = R"({"active": true, "enabled": false})";
    
    auto result = Parse(consumer, json);
    if (!result) return false;
    if (consumer.count != 2) return false;
    
    auto active = consumer.find(std::array<char, 16>{"active"});
    auto enabled = consumer.find(std::array<char, 16>{"enabled"});
    
    if (!active || *active != true) return false;
    if (!enabled || *enabled != false) return false;
    
    return true;
}
static_assert(test_map_bool_values());

// ============================================================================
// Test: Map with Struct Values
// ============================================================================

struct Point {
    int x;
    int y;
    
    constexpr bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

constexpr bool test_map_struct_values() {
    MapConsumer<std::array<char, 16>, Point, 3> consumer;
    
    std::string_view json = R"({"p1": {"x": 10, "y": 20}, "p2": {"x": 30, "y": 40}})";
    
    auto result = Parse(consumer, json);
    if (!result) return false;
    if (consumer.count != 2) return false;
    
    auto p1 = consumer.find(std::array<char, 16>{"p1"});
    auto p2 = consumer.find(std::array<char, 16>{"p2"});
    
    if (!p1 || p1->x != 10 || p1->y != 20) return false;
    if (!p2 || p2->x != 30 || p2->y != 40) return false;
    
    return true;
}
static_assert(test_map_struct_values());

// ============================================================================
// Test: Map with Array Values
// ============================================================================

constexpr bool test_map_array_values() {
    MapConsumer<std::array<char, 16>, std::array<int, 3>, 3> consumer;
    
    std::string_view json = R"({"arr1": [1, 2, 3], "arr2": [4, 5, 6]})";
    
    auto result = Parse(consumer, json);
    if (!result) return false;
    if (consumer.count != 2) return false;
    
    auto arr1 = consumer.find(std::array<char, 16>{"arr1"});
    auto arr2 = consumer.find(std::array<char, 16>{"arr2"});
    
    if (!arr1) return false;
    if ((*arr1)[0] != 1 || (*arr1)[1] != 2 || (*arr1)[2] != 3) return false;
    
    if (!arr2) return false;
    if ((*arr2)[0] != 4 || (*arr2)[1] != 5 || (*arr2)[2] != 6) return false;
    
    return true;
}
static_assert(test_map_array_values());

// ============================================================================
// Test: Whitespace Handling
// ============================================================================

constexpr bool test_map_whitespace() {
    MapConsumer<std::array<char, 16>, int, 3> consumer;
    
    std::string_view json = R"(  {  "a"  :  1  ,  "b"  :  2  }  )";
    
    auto result = Parse(consumer, json);
    if (!result) return false;
    if (consumer.count != 2) return false;
    
    auto a_val = consumer.find(std::array<char, 16>{"a"});
    auto b_val = consumer.find(std::array<char, 16>{"b"});
    
    if (!a_val || *a_val != 1) return false;
    if (!b_val || *b_val != 2) return false;
    
    return true;
}
static_assert(test_map_whitespace());

// ============================================================================
// Test: Map Malformed JSON Errors
// ============================================================================

constexpr bool test_map_missing_colon() {
    MapConsumer<std::array<char, 16>, int, 3> consumer;
    std::string_view json = R"({"key" 123})";
    return TestHelpers::ParseFailsWithReaderError(consumer, std::string_view(json), JsonIteratorReaderError::ILLFORMED_OBJECT);
}
static_assert(test_map_missing_colon());

constexpr bool test_map_missing_comma() {
    MapConsumer<std::array<char, 16>, int, 3> consumer;
    std::string_view json = R"({"a": 1 "b": 2})";
    return TestHelpers::ParseFailsWithReaderError(consumer, std::string_view(json), JsonIteratorReaderError::ILLFORMED_OBJECT);
}
static_assert(test_map_missing_comma());

constexpr bool test_map_trailing_comma() {
    MapConsumer<std::array<char, 16>, int, 3> consumer;
    std::string_view json = R"({"a": 1, "b": 2,})";
    return TestHelpers::ParseFailsWithReaderError(consumer, std::string_view(json), JsonIteratorReaderError::ILLFORMED_OBJECT);
}
static_assert(test_map_trailing_comma());

// ============================================================================
// Test: Single Entry Map
// ============================================================================

constexpr bool test_map_single_entry() {
    MapConsumer<std::array<char, 16>, int, 3> consumer;
    
    std::string_view json = R"({"only": 42})";
    
    auto result = Parse(consumer, json);
    if (!result) return false;
    if (consumer.count != 1) return false;
    
    auto val = consumer.find(std::array<char, 16>{"only"});
    if (!val || *val != 42) return false;
    
    return true;
}
static_assert(test_map_single_entry());

// ============================================================================
// Test: Map with Escaped Keys
// ============================================================================

constexpr bool test_map_escaped_keys() {
    MapConsumer<std::array<char, 32>, int, 3> consumer;
    
    std::string_view json = R"({"key\"with\"quotes": 100, "line\nbreak": 200})";
    
    auto result = Parse(consumer, json);
    if (!result) return false;
    if (consumer.count != 2) return false;
    
    // Keys should be unescaped
    auto val1 = consumer.find(std::array<char, 32>{"key\"with\"quotes"});
    auto val2 = consumer.find(std::array<char, 32>{"line\nbreak"});
    
    if (!val1 || *val1 != 100) return false;
    if (!val2 || *val2 != 200) return false;
    
    return true;
}
static_assert(test_map_escaped_keys());

// ============================================================================
// All Tests Passed!
// ============================================================================

constexpr bool all_tests_pass() {
    return test_parse_simple_string_int_map() &&
           test_parse_empty_map() &&
           test_parse_map_string_values() &&
           test_parse_nested_map() &&
           test_duplicate_key_error() &&
           test_map_overflow() &&
           test_serialize_simple_map() &&
           test_serialize_empty_map() &&
           test_roundtrip_map() &&
           test_map_bool_values() &&
           test_map_struct_values() &&
           test_map_array_values() &&
           test_map_whitespace() &&
           test_map_missing_colon() &&
           test_map_missing_comma() &&
           test_map_trailing_comma() &&
           test_map_single_entry() &&
           test_map_escaped_keys();
}

static_assert(all_tests_pass(), "[[[ All map streaming tests must pass at compile time ]]]");

int main() {
    // Runtime verification (optional, for debugging)
    static_assert(all_tests_pass());
    return 0;
}
