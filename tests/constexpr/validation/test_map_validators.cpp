#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/validators.hpp>
#include <array>
#include <string_view>

using namespace JsonFusion;
using namespace JsonFusion::static_schema;
using namespace JsonFusion::validators;

// ============================================================================
// Map Entry Structure for Streaming
// ============================================================================

template<typename K, typename V>
struct MapEntry {
    K key;
    V value;
};

// ============================================================================
// Map Consumer for Constexpr Testing
// ============================================================================

template<typename K, typename V, std::size_t MaxEntries>
struct MapConsumer {
    using value_type = MapEntry<K, V>;
    
    std::array<MapEntry<K, V>, MaxEntries> entries{};
    std::size_t count = 0;
    bool duplicate_found = false;
    
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
                return false;
            }
        }
        
        if (count >= MaxEntries) {
            return false;
        }
        
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

static_assert(ConsumingMapStreamerLike<MapConsumer<std::array<char, 32>, int, 10>>);

// ============================================================================
// SECTION 1: Basic Map Validators (Properties Count)
// ============================================================================

// Test: min_properties
constexpr bool test_min_properties_pass() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_properties<2>
    > consumer;
    
    const char* json = R"({"a": 1, "b": 2, "c": 3})";  // 3 >= 2
    return Parse(consumer, json);
}
static_assert(test_min_properties_pass());

constexpr bool test_min_properties_fail() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_properties<3>
    > consumer;
    
    const char* json = R"({"a": 1, "b": 2})";  // 2 < 3
    return !Parse(consumer, json);
}
static_assert(test_min_properties_fail());

constexpr bool test_min_properties_exact() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_properties<2>
    > consumer;
    
    const char* json = R"({"a": 1, "b": 2})";  // 2 == 2
    return Parse(consumer, json);
}
static_assert(test_min_properties_exact());

constexpr bool test_min_properties_empty_map() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_properties<1>
    > consumer;
    
    const char* json = R"({})";  // 0 < 1
    return !Parse(consumer, json);
}
static_assert(test_min_properties_empty_map());

// Test: max_properties
constexpr bool test_max_properties_pass() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        max_properties<5>
    > consumer;
    
    const char* json = R"({"a": 1, "b": 2, "c": 3})";  // 3 <= 5
    return Parse(consumer, json);
}
static_assert(test_max_properties_pass());

constexpr bool test_max_properties_fail() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        max_properties<2>
    > consumer;
    
    const char* json = R"({"a": 1, "b": 2, "c": 3})";  // 3 > 2
    return !Parse(consumer, json);
}
static_assert(test_max_properties_fail());

constexpr bool test_max_properties_exact() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        max_properties<3>
    > consumer;
    
    const char* json = R"({"a": 1, "b": 2, "c": 3})";  // 3 == 3
    return Parse(consumer, json);
}
static_assert(test_max_properties_exact());

// Test: Combined min/max properties
constexpr bool test_properties_range_valid() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_properties<2>,
        max_properties<5>
    > consumer;
    
    const char* json = R"({"a": 1, "b": 2, "c": 3})";  // 3 in [2, 5]
    return Parse(consumer, json);
}
static_assert(test_properties_range_valid());

constexpr bool test_properties_range_too_few() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_properties<3>,
        max_properties<5>
    > consumer;
    
    const char* json = R"({"a": 1, "b": 2})";  // 2 < 3
    return !Parse(consumer, json);
}
static_assert(test_properties_range_too_few());

constexpr bool test_properties_range_too_many() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_properties<1>,
        max_properties<2>
    > consumer;
    
    const char* json = R"({"a": 1, "b": 2, "c": 3})";  // 3 > 2
    return !Parse(consumer, json);
}
static_assert(test_properties_range_too_many());

// ============================================================================
// SECTION 2: Basic Map Validators (Key Length)
// ============================================================================

// Test: min_key_length
constexpr bool test_min_key_length_pass() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_key_length<2>
    > consumer;
    
    const char* json = R"({"ab": 1, "xyz": 2})";  // All keys >= 2 chars
    return Parse(consumer, json);
}
static_assert(test_min_key_length_pass());

constexpr bool test_min_key_length_fail() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_key_length<3>
    > consumer;
    
    const char* json = R"({"a": 1, "bcd": 2})";  // "a" has 1 char < 3
    return !Parse(consumer, json);
}
static_assert(test_min_key_length_fail());

constexpr bool test_min_key_length_exact() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_key_length<3>
    > consumer;
    
    const char* json = R"({"abc": 1, "defg": 2})";  // "abc" has 3 chars == 3
    return Parse(consumer, json);
}
static_assert(test_min_key_length_exact());

// Test: max_key_length
constexpr bool test_max_key_length_pass() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        max_key_length<5>
    > consumer;
    
    const char* json = R"({"ab": 1, "xyz": 2})";  // All keys <= 5 chars
    return Parse(consumer, json);
}
static_assert(test_max_key_length_pass());

constexpr bool test_max_key_length_fail() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        max_key_length<3>
    > consumer;
    
    const char* json = R"({"ab": 1, "toolong": 2})";  // "toolong" has 7 chars > 3
    return !Parse(consumer, json);
}
static_assert(test_max_key_length_fail());

constexpr bool test_max_key_length_exact() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        max_key_length<3>
    > consumer;
    
    const char* json = R"({"abc": 1, "xy": 2})";  // "abc" has 3 chars == 3
    return Parse(consumer, json);
}
static_assert(test_max_key_length_exact());

// Test: Combined key length range
constexpr bool test_key_length_range_valid() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_key_length<2>,
        max_key_length<5>
    > consumer;
    
    const char* json = R"({"ab": 1, "xyz": 2, "test": 3})";  // All in [2, 5]
    return Parse(consumer, json);
}
static_assert(test_key_length_range_valid());

constexpr bool test_key_length_range_too_short() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_key_length<3>,
        max_key_length<5>
    > consumer;
    
    const char* json = R"({"ab": 1, "xyz": 2})";  // "ab" has 2 < 3
    return !Parse(consumer, json);
}
static_assert(test_key_length_range_too_short());

constexpr bool test_key_length_range_too_long() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_key_length<2>,
        max_key_length<4>
    > consumer;
    
    const char* json = R"({"ab": 1, "toolong": 2})";  // "toolong" has 7 > 4
    return !Parse(consumer, json);
}
static_assert(test_key_length_range_too_long());

// ============================================================================
// SECTION 3: Key Set Validators (allowed_keys)
// ============================================================================

constexpr bool test_allowed_keys_valid() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 5>,
        allowed_keys<"a", "b", "c">
    > consumer;
    
    const char* json = R"({"a": 1, "b": 2, "c": 3})";
    
    auto result = Parse(consumer, json);
    return result && consumer->count == 3;
}
static_assert(test_allowed_keys_valid());

constexpr bool test_allowed_keys_reject() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 5>,
        allowed_keys<"x", "y">
    > consumer;
    
    const char* json = R"({"x": 1, "z": 2})";  // "z" not allowed
    
    auto result = Parse(consumer, json);
    return !result;  // Should fail
}
static_assert(test_allowed_keys_reject());

constexpr bool test_allowed_keys_incremental() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 5>,
        allowed_keys<"alpha", "beta">
    > consumer;
    
    // "gamma" should be rejected incrementally when we reach 'g'
    const char* json = R"({"alpha": 1, "gamma": 2})";
    
    auto result = Parse(consumer, json);
    return !result;  // Should fail early
}
static_assert(test_allowed_keys_incremental());

constexpr bool test_allowed_keys_partial_match() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 5>,
        allowed_keys<"key">
    > consumer;
    
    const char* json = R"({"keyExtra": 1})";  // "keyExtra" != "key"
    return !Parse(consumer, json);
}
static_assert(test_allowed_keys_partial_match());

constexpr bool test_empty_allowed_list() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 5>,
        allowed_keys<>  // No keys allowed!
    > consumer;
    
    const char* json = R"({"anything": 1})";
    return !Parse(consumer, json);
}
static_assert(test_empty_allowed_list());

constexpr bool test_many_allowed_keys() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 20>,
        allowed_keys<
            "a", "b", "c", "d", "e", "f", "g", "h", "i", "j",
            "k", "l", "m", "n", "o", "p", "q", "r", "s", "t"
        >
    > consumer;
    
    const char* json = R"({"a": 1, "t": 20, "m": 13})";
    
    auto result = Parse(consumer, json);
    return result && consumer->count == 3;
}
static_assert(test_many_allowed_keys());

// ============================================================================
// SECTION 4: Key Set Validators (forbidden_keys)
// ============================================================================

constexpr bool test_forbidden_keys_valid() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 5>,
        forbidden_keys<"bad", "evil">
    > consumer;
    
    const char* json = R"({"good": 1, "nice": 2})";
    
    auto result = Parse(consumer, json);
    return result && consumer->count == 2;
}
static_assert(test_forbidden_keys_valid());

constexpr bool test_forbidden_keys_reject() {
    Annotated<
        MapConsumer<std::array<char, 32>, int, 5>,
        forbidden_keys<"__proto__", "constructor">
    > consumer;
    
    const char* json = R"({"name": 1, "__proto__": 2})";
    
    auto result = Parse(consumer, json);
    return !result;  // Should fail
}
static_assert(test_forbidden_keys_reject());

constexpr bool test_forbidden_keys_similar() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 5>,
        forbidden_keys<"bad">
    > consumer;
    
    const char* json = R"({"badge": 1, "badminton": 2})";  // "badge" != "bad"
    
    auto result = Parse(consumer, json);
    return result && consumer->count == 2;
}
static_assert(test_forbidden_keys_similar());

constexpr bool test_empty_forbidden_list() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 5>,
        forbidden_keys<>  // Nothing forbidden
    > consumer;
    
    const char* json = R"({"anything": 1, "goes": 2})";
    
    auto result = Parse(consumer, json);
    return result && consumer->count == 2;
}
static_assert(test_empty_forbidden_list());

// ============================================================================
// SECTION 5: Key Set Validators (required_keys)
// ============================================================================

constexpr bool test_required_keys_valid() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 5>,
        required_keys<"id", "name">
    > consumer;
    
    const char* json = R"({"id": 1, "name": 2, "optional": 3})";
    
    auto result = Parse(consumer, json);
    return result && consumer->count == 3;
}
static_assert(test_required_keys_valid());

constexpr bool test_required_keys_missing() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 5>,
        required_keys<"id", "email", "age">
    > consumer;
    
    const char* json = R"({"id": 1, "email": 2})";  // Missing "age"
    
    auto result = Parse(consumer, json);
    return !result;  // Should fail
}
static_assert(test_required_keys_missing());

constexpr bool test_required_keys_empty_map() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 5>,
        required_keys<"a", "b">
    > consumer;
    
    const char* json = R"({})";
    return !Parse(consumer, json);
}
static_assert(test_required_keys_empty_map());

constexpr bool test_required_keys_with_duplicates() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 5>,
        required_keys<"a">
    > consumer;
    
    const char* json = R"({"a": 1, "a": 2})";  // Duplicate key
    
    auto result = Parse(consumer, json);
    return !result;  // Should fail on duplicate
}
static_assert(test_required_keys_with_duplicates());

// ============================================================================
// SECTION 6: Combined Validators
// ============================================================================

// All basic validators together
constexpr bool test_all_basic_constraints() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_properties<2>,
        max_properties<4>,
        min_key_length<2>,
        max_key_length<6>
    > consumer;
    
    const char* json = R"({"name": 1, "age": 2, "city": 3})";
    // 3 entries in [2, 4] ✓
    // Keys: "name"(4), "age"(3), "city"(4) all in [2, 6] ✓
    
    return Parse(consumer, json);
}
static_assert(test_all_basic_constraints());

// allowed_keys + required_keys
constexpr bool test_allowed_and_required() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        allowed_keys<"id", "name", "age", "email">,
        required_keys<"id", "name">
    > consumer;
    
    const char* json = R"({"id": 1, "name": 2, "age": 3})";
    
    auto result = Parse(consumer, json);
    return result && consumer->count == 3;
}
static_assert(test_allowed_and_required());

constexpr bool test_allowed_and_required_missing() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        allowed_keys<"id", "name", "age">,
        required_keys<"id", "name">
    > consumer;
    
    const char* json = R"({"id": 1, "age": 3})";  // Missing "name"
    return !Parse(consumer, json);
}
static_assert(test_allowed_and_required_missing());

constexpr bool test_allowed_and_required_not_allowed() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        allowed_keys<"id", "name">,
        required_keys<"id", "name">
    > consumer;
    
    const char* json = R"({"id": 1, "name": 2, "extra": 3})";  // "extra" not allowed
    return !Parse(consumer, json);
}
static_assert(test_allowed_and_required_not_allowed());

// All three key validators
constexpr bool test_all_three_validators() {
    Annotated<
        MapConsumer<std::array<char, 32>, int, 10>,
        allowed_keys<"id", "name", "email", "age">,
        required_keys<"id", "name">,
        forbidden_keys<"__proto__">
    > consumer;
    
    const char* json = R"({"id": 1, "name": 2, "email": 3})";
    
    auto result = Parse(consumer, json);
    return result && consumer->count == 3;
}
static_assert(test_all_three_validators());

constexpr bool test_all_three_forbidden_detected() {
    Annotated<
        MapConsumer<std::array<char, 32>, int, 10>,
        allowed_keys<"id", "name", "__proto__">,  // Allow it in whitelist
        required_keys<"id">,
        forbidden_keys<"__proto__">  // But blacklist it!
    > consumer;
    
    const char* json = R"({"id": 1, "__proto__": 666})";
    
    auto result = Parse(consumer, json);
    return !result;  // forbidden_keys should reject
}
static_assert(test_all_three_forbidden_detected());

// Basic + Key Set validators
constexpr bool test_min_properties_and_required() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_properties<3>,
        required_keys<"a", "b">
    > consumer;
    
    const char* json = R"({"a": 1, "b": 2, "c": 3})";
    
    auto result = Parse(consumer, json);
    return result && consumer->count == 3;
}
static_assert(test_min_properties_and_required());

constexpr bool test_max_properties_and_allowed() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        max_properties<2>,
        allowed_keys<"x", "y", "z">
    > consumer;
    
    const char* json = R"({"x": 1, "y": 2})";
    
    auto result = Parse(consumer, json);
    return result && consumer->count == 2;
}
static_assert(test_max_properties_and_allowed());

constexpr bool test_key_length_and_allowed() {
    Annotated<
        MapConsumer<std::array<char, 32>, int, 5>,
        allowed_keys<"ab", "abc", "abcd">,
        min_key_length<2>,
        max_key_length<4>
    > consumer;
    
    const char* json = R"({"ab": 1, "abc": 2, "abcd": 3})";
    
    auto result = Parse(consumer, json);
    return result && consumer->count == 3;
}
static_assert(test_key_length_and_allowed());

// ============================================================================
// SECTION 7: Edge Cases and Special Scenarios
// ============================================================================

constexpr bool test_case_sensitivity() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 5>,
        allowed_keys<"Name", "AGE">
    > consumer;
    
    const char* json = R"({"name": 1})";  // Lowercase "name"
    return !Parse(consumer, json);  // Should fail - case sensitive
}
static_assert(test_case_sensitivity());

// ============================================================================
// SECTION 8: Nested Maps and Complex Values
// ============================================================================

struct InnerConsumer {
    using value_type = MapEntry<std::array<char, 16>, int>;
    
    std::array<MapEntry<std::array<char, 16>, int>, 3> entries{};
    std::size_t count = 0;
    
    constexpr bool consume(const MapEntry<std::array<char, 16>, int>& entry) {
        if (count >= 3) return false;
        entries[count++] = entry;
        return true;
    }
    
    constexpr bool finalize(bool success) { return success; }
    constexpr void reset() { count = 0; }
};

constexpr bool test_nested_map_with_validators() {
    // Outer map with validators
    Annotated<
        MapConsumer<std::array<char, 16>, InnerConsumer, 3>,
        allowed_keys<"user", "admin">,
        required_keys<"user">
    > outer;
    
    const char* json = R"({"user": {"id": 1, "name": 2}})";
    
    auto result = Parse(outer, json);
    return result && outer->count == 1;
}
static_assert(test_nested_map_with_validators());

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

struct Point {
    int x;
    int y;
};

constexpr bool test_map_with_struct_values() {
    Annotated<
        MapConsumer<std::array<char, 16>, Point, 5>,
        min_properties<1>,
        max_properties<3>
    > consumer;
    
    const char* json = R"({"p1": {"x": 10, "y": 20}, "p2": {"x": 30, "y": 40}})";
    
    auto result = Parse(consumer, json);
    return result;  // 2 entries in [1, 3]
}
static_assert(test_map_with_struct_values());

constexpr bool test_map_with_array_values() {
    Annotated<
        MapConsumer<std::array<char, 16>, std::array<int, 3>, 5>,
        min_properties<1>,
        max_key_length<10>
    > consumer;
    
    const char* json = R"({"arr1": [1, 2, 3], "arr2": [4, 5, 6]})";
    
    auto result = Parse(consumer, json);
    return result;  // 2 entries >= 1, keys "arr1"(4), "arr2"(4) <= 10
}
static_assert(test_map_with_array_values());

// ============================================================================
// All Tests Summary
// ============================================================================

constexpr bool all_map_validator_tests_pass() {
    return 
        // Basic validators - properties count
        test_min_properties_pass() &&
        test_min_properties_fail() &&
        test_min_properties_exact() &&
        test_min_properties_empty_map() &&
        test_max_properties_pass() &&
        test_max_properties_fail() &&
        test_max_properties_exact() &&
        test_properties_range_valid() &&
        test_properties_range_too_few() &&
        test_properties_range_too_many() &&
        
        // Basic validators - key length
        test_min_key_length_pass() &&
        test_min_key_length_fail() &&
        test_min_key_length_exact() &&
        test_max_key_length_pass() &&
        test_max_key_length_fail() &&
        test_max_key_length_exact() &&
        test_key_length_range_valid() &&
        test_key_length_range_too_short() &&
        test_key_length_range_too_long() &&
        
        // Key set validators - allowed_keys
        test_allowed_keys_valid() &&
        test_allowed_keys_reject() &&
        test_allowed_keys_incremental() &&
        test_allowed_keys_partial_match() &&
        test_empty_allowed_list() &&
        test_many_allowed_keys() &&
        
        // Key set validators - forbidden_keys
        test_forbidden_keys_valid() &&
        test_forbidden_keys_reject() &&
        test_forbidden_keys_similar() &&
        test_empty_forbidden_list() &&
        
        // Key set validators - required_keys
        test_required_keys_valid() &&
        test_required_keys_missing() &&
        test_required_keys_empty_map() &&
        test_required_keys_with_duplicates() &&
        
        // Combined validators
        test_all_basic_constraints() &&
        test_allowed_and_required() &&
        test_allowed_and_required_missing() &&
        test_allowed_and_required_not_allowed() &&
        test_all_three_validators() &&
        test_all_three_forbidden_detected() &&
        test_min_properties_and_required() &&
        test_max_properties_and_allowed() &&
        test_key_length_and_allowed() &&
        
        // Edge cases and special scenarios
        test_case_sensitivity() &&
        
        // Nested maps and complex values
        test_nested_map_with_validators() &&
        test_nested_map_validation() &&
        test_nested_map_validation_inner_fail() &&
        test_map_with_struct_values() &&
        test_map_with_array_values();
}

static_assert(all_map_validator_tests_pass(), 
    "[[[ All map validator tests must pass at compile time ]]]");

int main() {
    // All tests verified at compile time
    return 0;
}
