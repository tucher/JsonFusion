#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/static_schema.hpp>
#include <array>

using namespace TestHelpers;
using namespace JsonFusion;
using namespace JsonFusion::static_schema;

// ============================================================================
// Test: Consuming Map Streamers
// ============================================================================

// Map entry structure required by ConsumingMapStreamerLike
template<typename K, typename V>
struct MapEntry {
    K key;
    V value;
    
    constexpr bool operator==(const MapEntry& other) const {
        // Simple comparison for testing
        if (key.size() != other.key.size()) return false;
        for (std::size_t i = 0; i < key.size(); ++i) {
            if (key[i] != other.key[i]) return false;
            if (key[i] == '\0') break;
        }
        return value == other.value;
    }
};

// Simple map consumer with context
struct IntMapConsumer {
    using value_type = MapEntry<std::array<char, 32>, int>;
    
    std::array<MapEntry<std::array<char, 32>, int>, 10> entries{};
    std::size_t count = 0;
    mutable int* context = nullptr;  // For context passing
    
    constexpr bool consume(const value_type& entry) {
        if (count >= 10) return false;
        entries[count++] = entry;
        // Use context if available
        if (context) {
            (*context) += entry.value;  // Sum values via context
        }
        return true;
    }
    
    constexpr bool finalize(bool success) {
        return success;
    }
    
    constexpr void reset() {
        count = 0;
    }
    
    // Context passing - high-level interface
    constexpr void set_json_fusion_context(int* ctx) const {
        context = ctx;
    }
};

static_assert(ConsumingMapStreamerLike<IntMapConsumer>);
static_assert(JsonParsableMap<IntMapConsumer>);

// Test 1: Map consumer as first-class type (direct parsing)
static_assert(
    []() constexpr {
        IntMapConsumer consumer{};
        std::string json = R"({"key1": 10, "key2": 20, "key3": 30})";
        auto result = JsonFusion::Parse(consumer, json);
        if (!result) return false;
        return consumer.count == 3;
    }(),
    "Map consumer works as first-class type (direct parsing)"
);

// Test 2: Map consumer works transparently in place of map (as struct field)
struct WithIntMapConsumer {
    IntMapConsumer consumer;
};

static_assert(
    []() constexpr {
        WithIntMapConsumer obj{};
        std::string json = R"({"consumer": {"key1": 10, "key2": 20}})";
        auto result = JsonFusion::Parse(obj, json);
        if (!result) return false;
        return obj.consumer.count == 2;
    }(),
    "Map consumer works transparently in place of map (as struct field)"
);

// Test 3: Map consumer with context passing
// Note: Context passing may work differently for maps - verify basic functionality
static_assert(
    []() constexpr {
        IntMapConsumer consumer{};
        int context_value = 0;
        std::string json = R"({"a": 10, "b": 20})";
        auto result = JsonFusion::Parse(consumer, json, &context_value);
        if (!result) return false;
        // At minimum, parsing should succeed with context
        return consumer.count == 2  && context_value == 30 ;
    }(),
    "Map consumer with context passing"
);

// Test 4: Empty map
static_assert(
    []() constexpr {
        IntMapConsumer consumer{};
        std::string json = R"({})";
        auto result = JsonFusion::Parse(consumer, json);
        return result && consumer.count == 0;
    }(),
    "Empty map parsing succeeds"
);

// Test 5: Single entry
static_assert(
    []() constexpr {
        IntMapConsumer consumer{};
        std::string json = R"({"single": 42})";
        auto result = JsonFusion::Parse(consumer, json);
        if (!result) return false;
        return consumer.count == 1;
    }(),
    "Single entry"
);

// Test 6: Many entries
static_assert(
    []() constexpr {
        IntMapConsumer consumer{};
        std::string json = R"({"a": 1, "b": 2, "c": 3, "d": 4, "e": 5})";
        auto result = JsonFusion::Parse(consumer, json);
        if (!result) return false;
        return consumer.count == 5;
    }(),
    "Many entries"
);

// Test 7: Duplicate key detection
struct DuplicateDetectingConsumer {
    using value_type = MapEntry<std::array<char, 32>, int>;
    
    std::array<MapEntry<std::array<char, 32>, int>, 10> entries{};
    std::size_t count = 0;
    bool duplicate_found = false;
    
    constexpr bool consume(const value_type& entry) {
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
        
        if (count >= 10) return false;
        entries[count++] = entry;
        return true;
    }
    
    constexpr bool finalize(bool success) {
        return success && !duplicate_found;
    }
    
    constexpr void reset() {
        count = 0;
        duplicate_found = false;
    }
};

static_assert(ConsumingMapStreamerLike<DuplicateDetectingConsumer>);

static_assert(
    []() constexpr {
        DuplicateDetectingConsumer consumer{};
        std::string json = R"({"key1": 10, "key2": 20})";
        auto result = JsonFusion::Parse(consumer, json);
        if (!result) return false;
        return consumer.count == 2 && !consumer.duplicate_found;
    }(),
    "Duplicate detection: accept unique keys"
);

static_assert(
    []() constexpr {
        DuplicateDetectingConsumer consumer{};
        std::string json = R"({"key1": 10, "key1": 20})";
        auto result = JsonFusion::Parse(consumer, json);
        // Should fail because duplicate key is rejected
        return !result && consumer.duplicate_found;
    }(),
    "Duplicate detection: reject duplicate keys"
);

// Test 8: Map consumer with string values
struct StringMapConsumer {
    using value_type = MapEntry<std::array<char, 32>, std::string>;
    
    std::array<MapEntry<std::array<char, 32>, std::string>, 5> entries{};
    std::size_t count = 0;
    
    constexpr bool consume(const value_type& entry) {
        if (count >= 5) return false;
        entries[count++] = entry;
        return true;
    }
    
    constexpr bool finalize(bool success) {
        return success;
    }
    
    constexpr void reset() {
        count = 0;
    }
};

static_assert(ConsumingMapStreamerLike<StringMapConsumer>);

static_assert(
    []() constexpr {
        StringMapConsumer consumer{};
        std::string json = R"({"name": "Alice", "city": "NYC"})";
        auto result = JsonFusion::Parse(consumer, json);
        if (!result) return false;
        return consumer.count == 2;
    }(),
    "Map consumer with string values"
);

// Test 9: Map consumer with boolean values
struct BoolMapConsumer {
    using value_type = MapEntry<std::array<char, 32>, bool>;
    
    std::array<MapEntry<std::array<char, 32>, bool>, 5> entries{};
    std::size_t count = 0;
    
    constexpr bool consume(const value_type& entry) {
        if (count >= 5) return false;
        entries[count++] = entry;
        return true;
    }
    
    constexpr bool finalize(bool success) {
        return success;
    }
    
    constexpr void reset() {
        count = 0;
    }
};

static_assert(ConsumingMapStreamerLike<BoolMapConsumer>);

static_assert(
    []() constexpr {
        BoolMapConsumer consumer{};
        std::string json = R"({"flag1": true, "flag2": false})";
        auto result = JsonFusion::Parse(consumer, json);
        if (!result) return false;
        return consumer.count == 2;
    }(),
    "Map consumer with boolean values"
);

// Test 10: Map consumer in nested structures (transparency test)
struct Nested {
    struct Inner {
        IntMapConsumer map_consumer;
    } inner;
};

static_assert(
    []() constexpr {
        Nested obj{};
        std::string json = R"({"inner": {"map_consumer": {"x": 100, "y": 200}}})";
        auto result = JsonFusion::Parse(obj, json);
        if (!result) return false;
        return obj.inner.map_consumer.count == 2;
    }(),
    "Map consumer in nested structure"
);

// Test 11: Multiple map consumers in same struct
struct MultipleMapConsumers {
    IntMapConsumer ints;
    StringMapConsumer strings;
};

static_assert(
    []() constexpr {
        MultipleMapConsumers obj{};
        std::string json = R"({"ints": {"a": 1, "b": 2}, "strings": {"x": "hello", "y": "world"}})";
        auto result = JsonFusion::Parse(obj, json);
        if (!result) return false;
        return obj.ints.count == 2 && obj.strings.count == 2;
    }(),
    "Multiple map consumers in same struct"
);

// Test 12: Early termination (returning false from consume())
struct LimitedMapConsumer {
    using value_type = MapEntry<std::array<char, 32>, int>;
    
    std::array<MapEntry<std::array<char, 32>, int>, 5> entries{};
    std::size_t count = 0;
    std::size_t max_count = 2;  // Stop after 2 entries
    
    constexpr bool consume(const value_type& entry) {
        if (count >= max_count) return false;  // Early termination
        entries[count++] = entry;
        return true;
    }
    
    constexpr bool finalize(bool success) {
        // Should receive false because consume() returned false
        return !success;  // Expect failure due to early termination
    }
    
    constexpr void reset() {
        count = 0;
    }
};

static_assert(ConsumingMapStreamerLike<LimitedMapConsumer>);

static_assert(
    []() constexpr {
        LimitedMapConsumer consumer{};
        std::string json = R"({"a": 1, "b": 2, "c": 3, "d": 4})";
        auto result = JsonFusion::Parse(consumer, json);
        // Should fail because consume() returned false after 2 entries
        return !result && consumer.count == 2;
    }(),
    "Early termination via consume() returning false"
);

// Test 13: Entry validation in consume()
struct ValidatingMapConsumer {
    using value_type = MapEntry<std::array<char, 32>, int>;
    
    std::array<MapEntry<std::array<char, 32>, int>, 10> entries{};
    std::size_t count = 0;
    
    constexpr bool consume(const value_type& entry) {
        // Validate: only accept positive values
        if (entry.value <= 0) return false;
        if (count >= 10) return false;
        entries[count++] = entry;
        return true;
    }
    
    constexpr bool finalize(bool success) {
        return success;
    }
    
    constexpr void reset() {
        count = 0;
    }
};

static_assert(ConsumingMapStreamerLike<ValidatingMapConsumer>);

static_assert(
    []() constexpr {
        ValidatingMapConsumer consumer{};
        std::string json = R"({"a": 1, "b": 2, "c": 3})";
        auto result = JsonFusion::Parse(consumer, json);
        if (!result) return false;
        return consumer.count == 3;
    }(),
    "Validation: accept valid values"
);

static_assert(
    []() constexpr {
        ValidatingMapConsumer consumer{};
        std::string json = R"({"a": 1, "b": -5, "c": 3})";
        auto result = JsonFusion::Parse(consumer, json);
        // Should fail because -5 is rejected by consume()
        return !result && consumer.count == 1;  // Only first entry consumed
    }(),
    "Validation: reject invalid values"
);

// ============================================================================
// Tests with std::string keys (instead of std::array<char, N>)
// ============================================================================

// Test 14: Map consumer with std::string keys - basic functionality
struct StringKeyMapConsumer {
    using value_type = MapEntry<std::string, int>;
    
    std::array<MapEntry<std::string, int>, 10> entries{};
    std::size_t count = 0;
    
    constexpr bool consume(const value_type& entry) {
        if (count >= 10) return false;
        entries[count++] = entry;
        return true;
    }
    
    constexpr bool finalize(bool success) {
        return success;
    }
    
    constexpr void reset() {
        count = 0;
    }
};

static_assert(ConsumingMapStreamerLike<StringKeyMapConsumer>);

static_assert(
    []() constexpr {
        StringKeyMapConsumer consumer{};
        std::string json = R"({"first_key": 10, "second_key": 20, "third_key": 30})";
        auto result = JsonFusion::Parse(consumer, json);
        if (!result) return false;
        return consumer.count == 3;
    }(),
    "Map consumer with std::string keys - basic functionality"
);

// Test 15: Map consumer with std::string keys - duplicate detection
struct StringKeyDuplicateConsumer {
    using value_type = MapEntry<std::string, int>;
    
    std::array<MapEntry<std::string, int>, 10> entries{};
    std::size_t count = 0;
    bool duplicate_found = false;
    
    constexpr bool consume(const value_type& entry) {
        // Check for duplicates using std::string comparison
        for (std::size_t i = 0; i < count; ++i) {
            if (entries[i].key == entry.key) {
                duplicate_found = true;
                return false;  // Reject duplicate
            }
        }
        
        if (count >= 10) return false;
        entries[count++] = entry;
        return true;
    }
    
    constexpr bool finalize(bool success) {
        return success && !duplicate_found;
    }
    
    constexpr void reset() {
        count = 0;
        duplicate_found = false;
    }
};

static_assert(ConsumingMapStreamerLike<StringKeyDuplicateConsumer>);

static_assert(
    []() constexpr {
        StringKeyDuplicateConsumer consumer{};
        std::string json = R"({"unique1": 10, "unique2": 20})";
        auto result = JsonFusion::Parse(consumer, json);
        if (!result) return false;
        return consumer.count == 2 && !consumer.duplicate_found;
    }(),
    "std::string keys: accept unique keys"
);

static_assert(
    []() constexpr {
        StringKeyDuplicateConsumer consumer{};
        std::string json = R"({"duplicate": 10, "duplicate": 20})";
        auto result = JsonFusion::Parse(consumer, json);
        // Should fail because duplicate key is rejected
        return !result && consumer.duplicate_found;
    }(),
    "std::string keys: reject duplicate keys"
);

// Test 16: Map consumer with std::string keys - many entries
static_assert(
    []() constexpr {
        StringKeyMapConsumer consumer{};
        std::string json = R"({"alpha": 1, "beta": 2, "gamma": 3, "delta": 4, "epsilon": 5})";
        auto result = JsonFusion::Parse(consumer, json);
        if (!result) return false;
        return consumer.count == 5;
    }(),
    "Map consumer with std::string keys - many entries"
);

