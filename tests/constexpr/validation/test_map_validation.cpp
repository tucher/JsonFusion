#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/validators.hpp>
#include <array>

using namespace JsonFusion;
using namespace JsonFusion::validators;
using namespace JsonFusion::options;

// ============================================================================
// Map Entry for Testing
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
// Test: min_properties - Minimum Entry Count
// ============================================================================

constexpr bool test_min_properties_pass() {
    using ValidatedMap = Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_properties<2>
    >;
    
    ValidatedMap map;
    const char* json = R"({"a": 1, "b": 2, "c": 3})";  // 3 entries >= 2
    
    auto result = Parse(map, json);
    return result;  // Should pass
}
static_assert(test_min_properties_pass());

constexpr bool test_min_properties_fail() {
    using ValidatedMap = Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_properties<3>
    >;
    
    ValidatedMap map;
    const char* json = R"({"a": 1, "b": 2})";  // 2 entries < 3
    
    auto result = Parse(map, json);
    return !result;  // Should fail
}
static_assert(test_min_properties_fail());

constexpr bool test_min_properties_exact() {
    using ValidatedMap = Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_properties<2>
    >;
    
    ValidatedMap map;
    const char* json = R"({"a": 1, "b": 2})";  // 2 entries == 2
    
    auto result = Parse(map, json);
    return result;  // Should pass (exact boundary)
}
static_assert(test_min_properties_exact());

constexpr bool test_min_properties_empty_map() {
    using ValidatedMap = Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_properties<1>
    >;
    
    ValidatedMap map;
    const char* json = R"({})";  // 0 entries < 1
    
    auto result = Parse(map, json);
    return !result;  // Should fail
}
static_assert(test_min_properties_empty_map());

constexpr bool test_min_properties_zero() {
    using ValidatedMap = Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_properties<0>
    >;
    
    ValidatedMap map;
    const char* json = R"({})";  // 0 entries >= 0
    
    auto result = Parse(map, json);
    return result;  // Should pass
}
static_assert(test_min_properties_zero());

// ============================================================================
// Test: max_properties - Maximum Entry Count
// ============================================================================

constexpr bool test_max_properties_pass() {
    using ValidatedMap = Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        max_properties<5>
    >;
    
    ValidatedMap map;
    const char* json = R"({"a": 1, "b": 2, "c": 3})";  // 3 entries <= 5
    
    auto result = Parse(map, json);
    return result;  // Should pass
}
static_assert(test_max_properties_pass());

constexpr bool test_max_properties_fail() {
    using ValidatedMap = Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        max_properties<2>
    >;
    
    ValidatedMap map;
    const char* json = R"({"a": 1, "b": 2, "c": 3})";  // 3 entries > 2
    
    auto result = Parse(map, json);
    return !result;  // Should fail
}
static_assert(test_max_properties_fail());

constexpr bool test_max_properties_exact() {
    using ValidatedMap = Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        max_properties<3>
    >;
    
    ValidatedMap map;
    const char* json = R"({"a": 1, "b": 2, "c": 3})";  // 3 entries == 3
    
    auto result = Parse(map, json);
    return result;  // Should pass (exact boundary)
}
static_assert(test_max_properties_exact());

constexpr bool test_max_properties_empty() {
    using ValidatedMap = Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        max_properties<5>
    >;
    
    ValidatedMap map;
    const char* json = R"({})";  // 0 entries <= 5
    
    auto result = Parse(map, json);
    return result;  // Should pass
}
static_assert(test_max_properties_empty());

// ============================================================================
// Test: Combined min_properties + max_properties
// ============================================================================

constexpr bool test_properties_range_valid() {
    using ValidatedMap = Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_properties<2>,
        max_properties<5>
    >;
    
    ValidatedMap map;
    const char* json = R"({"a": 1, "b": 2, "c": 3})";  // 3 in [2, 5]
    
    auto result = Parse(map, json);
    return result;  // Should pass
}
static_assert(test_properties_range_valid());

constexpr bool test_properties_range_too_few() {
    using ValidatedMap = Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_properties<3>,
        max_properties<5>
    >;
    
    ValidatedMap map;
    const char* json = R"({"a": 1, "b": 2})";  // 2 < 3
    
    auto result = Parse(map, json);
    return !result;  // Should fail
}
static_assert(test_properties_range_too_few());

constexpr bool test_properties_range_too_many() {
    using ValidatedMap = Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_properties<1>,
        max_properties<2>
    >;
    
    ValidatedMap map;
    const char* json = R"({"a": 1, "b": 2, "c": 3})";  // 3 > 2
    
    auto result = Parse(map, json);
    return !result;  // Should fail
}
static_assert(test_properties_range_too_many());

// ============================================================================
// Test: min_key_length - Minimum Key String Length
// ============================================================================

constexpr bool test_min_key_length_pass() {
    using ValidatedMap = Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_key_length<2>
    >;
    
    ValidatedMap map;
    const char* json = R"({"ab": 1, "xyz": 2})";  // All keys >= 2 chars
    
    auto result = Parse(map, json);
    return result;  // Should pass
}
static_assert(test_min_key_length_pass());

constexpr bool test_min_key_length_fail() {
    using ValidatedMap = Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_key_length<3>
    >;
    
    ValidatedMap map;
    const char* json = R"({"a": 1, "bcd": 2})";  // "a" has 1 char < 3
    
    auto result = Parse(map, json);
    return !result;  // Should fail
}
static_assert(test_min_key_length_fail());

constexpr bool test_min_key_length_exact() {
    using ValidatedMap = Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_key_length<3>
    >;
    
    ValidatedMap map;
    const char* json = R"({"abc": 1, "defg": 2})";  // "abc" has 3 chars == 3
    
    auto result = Parse(map, json);
    return result;  // Should pass (exact boundary)
}
static_assert(test_min_key_length_exact());

// ============================================================================
// Test: max_key_length - Maximum Key String Length
// ============================================================================

constexpr bool test_max_key_length_pass() {
    using ValidatedMap = Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        max_key_length<5>
    >;
    
    ValidatedMap map;
    const char* json = R"({"ab": 1, "xyz": 2})";  // All keys <= 5 chars
    
    auto result = Parse(map, json);
    return result;  // Should pass
}
static_assert(test_max_key_length_pass());

constexpr bool test_max_key_length_fail() {
    using ValidatedMap = Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        max_key_length<3>
    >;
    
    ValidatedMap map;
    const char* json = R"({"ab": 1, "toolong": 2})";  // "toolong" has 7 chars > 3
    
    auto result = Parse(map, json);
    return !result;  // Should fail
}
static_assert(test_max_key_length_fail());

constexpr bool test_max_key_length_exact() {
    using ValidatedMap = Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        max_key_length<3>
    >;
    
    ValidatedMap map;
    const char* json = R"({"abc": 1, "xy": 2})";  // "abc" has 3 chars == 3
    
    auto result = Parse(map, json);
    return result;  // Should pass (exact boundary)
}
static_assert(test_max_key_length_exact());

// ============================================================================
// Test: Combined Key Length Constraints
// ============================================================================

constexpr bool test_key_length_range_valid() {
    using ValidatedMap = Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_key_length<2>,
        max_key_length<5>
    >;
    
    ValidatedMap map;
    const char* json = R"({"ab": 1, "xyz": 2, "test": 3})";  // All in [2, 5]
    
    auto result = Parse(map, json);
    return result;  // Should pass
}
static_assert(test_key_length_range_valid());

constexpr bool test_key_length_range_too_short() {
    using ValidatedMap = Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_key_length<3>,
        max_key_length<5>
    >;
    
    ValidatedMap map;
    const char* json = R"({"ab": 1, "xyz": 2})";  // "ab" has 2 < 3
    
    auto result = Parse(map, json);
    return !result;  // Should fail
}
static_assert(test_key_length_range_too_short());

constexpr bool test_key_length_range_too_long() {
    using ValidatedMap = Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_key_length<2>,
        max_key_length<4>
    >;
    
    ValidatedMap map;
    const char* json = R"({"ab": 1, "toolong": 2})";  // "toolong" has 7 > 4
    
    auto result = Parse(map, json);
    return !result;  // Should fail
}
static_assert(test_key_length_range_too_long());

// ============================================================================
// Test: All Map Constraints Together
// ============================================================================

constexpr bool test_all_map_constraints() {
    using ValidatedMap = Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_properties<2>,
        max_properties<4>,
        min_key_length<2>,
        max_key_length<6>
    >;
    
    ValidatedMap map;
    const char* json = R"({"name": 1, "age": 2, "city": 3})";
    // 3 entries in [2, 4] ✓
    // Keys: "name"(4), "age"(3), "city"(4) all in [2, 6] ✓
    
    auto result = Parse(map, json);
    return result;  // Should pass
}
static_assert(test_all_map_constraints());

constexpr bool test_all_map_constraints_fail_count() {
    using ValidatedMap = Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_properties<2>,
        max_properties<4>,
        min_key_length<2>,
        max_key_length<6>
    >;
    
    ValidatedMap map;
    const char* json = R"({"a": 1})";  // 1 entry < 2
    
    auto result = Parse(map, json);
    return !result;  // Should fail (too few entries)
}
static_assert(test_all_map_constraints_fail_count());

constexpr bool test_all_map_constraints_fail_key() {
    using ValidatedMap = Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_properties<2>,
        max_properties<4>,
        min_key_length<2>,
        max_key_length<6>
    >;
    
    ValidatedMap map;
    const char* json = R"({"ab": 1, "verylongkey": 2})";  // "verylongkey" = 11 > 6
    
    auto result = Parse(map, json);
    return !result;  // Should fail (key too long)
}
static_assert(test_all_map_constraints_fail_key());

// ============================================================================
// Test: Validators Work with Nested Maps
// ============================================================================

constexpr bool test_nested_map_validation() {
    using InnerMap = Annotated<
        MapConsumer<std::array<char, 8>, int, 5>,
        min_properties<1>
    >;
    
    using OuterMap = MapConsumer<std::array<char, 16>, InnerMap, 3>;
    
    OuterMap outer;
    const char* json = R"({"m1": {"a": 1}, "m2": {"b": 2, "c": 3}})";
    
    auto result = Parse(outer, json);
    return result;  // Both inner maps have >= 1 entry
}
static_assert(test_nested_map_validation());

constexpr bool test_nested_map_validation_inner_fail() {
    using InnerMap = Annotated<
        MapConsumer<std::array<char, 8>, int, 5>,
        min_properties<2>  // Requires at least 2 entries
    >;
    
    using OuterMap = MapConsumer<std::array<char, 16>, InnerMap, 3>;
    
    OuterMap outer;
    const char* json = R"({"m1": {"a": 1}, "m2": {"b": 2, "c": 3}})";
    // m1 has only 1 entry < 2
    
    auto result = Parse(outer, json);
    return !result;  // Should fail
}
static_assert(test_nested_map_validation_inner_fail());

// ============================================================================
// Test: Map Validators with Complex Value Types
// ============================================================================

struct Point {
    int x;
    int y;
};

constexpr bool test_map_with_struct_values() {
    using ValidatedMap = Annotated<
        MapConsumer<std::array<char, 16>, Point, 5>,
        min_properties<1>,
        max_properties<3>
    >;
    
    ValidatedMap map;
    const char* json = R"({"p1": {"x": 10, "y": 20}, "p2": {"x": 30, "y": 40}})";
    
    auto result = Parse(map, json);
    return result;  // 2 entries in [1, 3]
}
static_assert(test_map_with_struct_values());

constexpr bool test_map_with_array_values() {
    using ValidatedMap = Annotated<
        MapConsumer<std::array<char, 16>, std::array<int, 3>, 5>,
        min_properties<1>,
        max_key_length<10>
    >;
    
    ValidatedMap map;
    const char* json = R"({"arr1": [1, 2, 3], "arr2": [4, 5, 6]})";
    
    auto result = Parse(map, json);
    return result;  // 2 entries >= 1, keys "arr1"(4), "arr2"(4) <= 10
}
static_assert(test_map_with_array_values());

// ============================================================================
// Test: Validators with High-Level Map Streamer Interface
// ============================================================================

// Using the high-level map streamer interface (ConsumingMapStreamerLike)
template<typename K, typename V, std::size_t MaxEntries>
struct SimpleMapStreamer {
    struct Entry {
        K key;
        V value;
    };
    using value_type = Entry;
    
    std::array<Entry, MaxEntries> entries{};
    std::size_t count = 0;
    
    constexpr bool consume(const Entry& entry) {
        if (count >= MaxEntries) return false;
        entries[count++] = entry;
        return true;
    }
    
    constexpr bool finalize(bool success) { return success; }
    constexpr void reset() { count = 0; }
};

constexpr bool test_streamer_with_min_properties() {
    using ValidatedStreamer = Annotated<
        SimpleMapStreamer<std::array<char, 16>, int, 10>,
        min_properties<2>
    >;
    
    ValidatedStreamer streamer;
    const char* json = R"({"a": 1, "b": 2, "c": 3})";
    
    auto result = Parse(streamer, json);
    return result;  // 3 entries >= 2
}
static_assert(test_streamer_with_min_properties());

constexpr bool test_streamer_with_max_properties() {
    using ValidatedStreamer = Annotated<
        SimpleMapStreamer<std::array<char, 16>, int, 10>,
        max_properties<2>
    >;
    
    ValidatedStreamer streamer;
    const char* json = R"({"a": 1, "b": 2, "c": 3})";
    
    auto result = Parse(streamer, json);
    return !result;  // 3 entries > 2, should fail
}
static_assert(test_streamer_with_max_properties());

constexpr bool test_streamer_with_key_length() {
    using ValidatedStreamer = Annotated<
        SimpleMapStreamer<std::array<char, 16>, int, 10>,
        min_key_length<3>,
        max_key_length<10>
    >;
    
    ValidatedStreamer streamer;
    const char* json = R"({"abc": 1, "defgh": 2})";
    
    auto result = Parse(streamer, json);
    return result;  // Keys "abc"(3), "defgh"(5) in [3, 10]
}
static_assert(test_streamer_with_key_length());

constexpr bool test_streamer_with_all_validators() {
    using ValidatedStreamer = Annotated<
        SimpleMapStreamer<std::array<char, 32>, bool, 20>,
        min_properties<1>,
        max_properties<10>,
        min_key_length<5>,
        max_key_length<30>
    >;
    
    ValidatedStreamer streamer;
    const char* json = R"({
        "enable_feature_a": true,
        "enable_feature_b": false,
        "debug_mode": true
    })";
    
    auto result = Parse(streamer, json);
    return result;  // 3 entries in [1,10], all keys in [5,30]
}
static_assert(test_streamer_with_all_validators());

constexpr bool test_streamer_validator_fails_on_short_key() {
    using ValidatedStreamer = Annotated<
        SimpleMapStreamer<std::array<char, 16>, int, 10>,
        min_key_length<5>
    >;
    
    ValidatedStreamer streamer;
    const char* json = R"({"test": 1, "ok": 2})";  // "ok" is 2 chars < 5
    
    auto result = Parse(streamer, json);
    return !result;  // Should fail
}
static_assert(test_streamer_validator_fails_on_short_key());

// ============================================================================
// All Map Validation Tests Pass!
// ============================================================================

constexpr bool all_tests_pass() {
    return test_min_properties_pass() &&
           test_min_properties_fail() &&
           test_min_properties_exact() &&
           test_min_properties_empty_map() &&
           test_min_properties_zero() &&
           test_max_properties_pass() &&
           test_max_properties_fail() &&
           test_max_properties_exact() &&
           test_max_properties_empty() &&
           test_properties_range_valid() &&
           test_properties_range_too_few() &&
           test_properties_range_too_many() &&
           test_all_map_constraints() &&
           test_all_map_constraints_fail_count() &&
           test_all_map_constraints_fail_key() &&
           test_nested_map_validation() &&
           test_nested_map_validation_inner_fail() &&
           test_map_with_struct_values() &&
           test_map_with_array_values() &&
           test_streamer_with_min_properties() &&
           test_streamer_with_max_properties() &&
           test_streamer_with_key_length() &&
           test_streamer_with_all_validators() &&
           test_streamer_validator_fails_on_short_key();
}

static_assert(all_tests_pass(), "[[[ All map validation tests must pass at compile time ]]]");

int main() {
    static_assert(all_tests_pass());
    return 0;
}

