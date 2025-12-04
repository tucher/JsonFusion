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
// Test: Combined Map Validators - min_properties + max_properties
// ============================================================================

// Test: min_properties<2> + max_properties<5> - all constraints pass
constexpr bool test_combined_properties_valid() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_properties<2>,
        max_properties<5>
    > consumer;
    
    std::string_view json = R"({"a": 1, "b": 2, "c": 3})";  // 3 properties, within range
    return Parse(consumer, json);
}
static_assert(test_combined_properties_valid(), "min_properties<2> + max_properties<5> - all constraints pass");

// Test: min_properties<2> + max_properties<5> - fails min_properties
constexpr bool test_combined_properties_fails_min() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_properties<2>,
        max_properties<5>
    > consumer;
    
    std::string_view json = R"({"a": 1})";  // 1 property < 2
    return !Parse(consumer, json);
}
static_assert(test_combined_properties_fails_min(), "min_properties<2> + max_properties<5> - fails min_properties");

// Test: min_properties<2> + max_properties<5> - fails max_properties
constexpr bool test_combined_properties_fails_max() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_properties<2>,
        max_properties<5>
    > consumer;
    
    std::string_view json = R"({"a": 1, "b": 2, "c": 3, "d": 4, "e": 5, "f": 6})";  // 6 properties > 5
    return !Parse(consumer, json);
}
static_assert(test_combined_properties_fails_max(), "min_properties<2> + max_properties<5> - fails max_properties");

// ============================================================================
// Test: Combined Map Validators - required_keys + allowed_keys
// ============================================================================

// Test: required_keys + allowed_keys - all constraints pass
constexpr bool test_combined_required_allowed_valid() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        required_keys<"id", "name">,
        allowed_keys<"id", "name", "age">
    > consumer;
    
    std::string_view json = R"({"id": 1, "name": 2, "age": 25})";  // All required present, all allowed
    auto result = Parse(consumer, json);
    return result && consumer.get().count == 3;
}
static_assert(test_combined_required_allowed_valid(), "required_keys + allowed_keys - all constraints pass");

// Test: required_keys + allowed_keys - missing required key
constexpr bool test_combined_required_allowed_missing_required() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        required_keys<"id", "name">,
        allowed_keys<"id", "name", "age">
    > consumer;
    
    std::string_view json = R"({"id": 1, "age": 25})";  // "name" missing
    return !Parse(consumer, json);
}
static_assert(test_combined_required_allowed_missing_required(), "required_keys + allowed_keys - missing required key");

// Test: required_keys + allowed_keys - not allowed key
constexpr bool test_combined_required_allowed_not_allowed() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        required_keys<"id", "name">,
        allowed_keys<"id", "name", "age">
    > consumer;
    
    std::string_view json = R"({"id": 1, "name": "test", "extra": 100})";  // "extra" not allowed
    return !Parse(consumer, json);
}
static_assert(test_combined_required_allowed_not_allowed(), "required_keys + allowed_keys - not allowed key");

// ============================================================================
// Test: Combined Map Validators - allowed_keys + forbidden_keys
// ============================================================================

// Test: allowed_keys + forbidden_keys - forbidden takes precedence
constexpr bool test_combined_allowed_forbidden_forbidden_wins() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        allowed_keys<"id", "name", "__proto__">,
        forbidden_keys<"__proto__">
    > consumer;
    
    std::string_view json = R"({"id": 1, "name": "test", "__proto__": 100})";  // "__proto__" is in allowed but forbidden
    return !Parse(consumer, json);
}
static_assert(test_combined_allowed_forbidden_forbidden_wins(), "allowed_keys + forbidden_keys - forbidden takes precedence");

// Test: allowed_keys + forbidden_keys - valid keys pass
constexpr bool test_combined_allowed_forbidden_valid() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        allowed_keys<"id", "name", "__proto__">,
        forbidden_keys<"__proto__">
    > consumer;
    
    std::string_view json = R"({"id": 1, "name": 2})";  // Only allowed keys, no forbidden
    auto result = Parse(consumer, json);
    return result && consumer.get().count == 2;
}
static_assert(test_combined_allowed_forbidden_valid(), "allowed_keys + forbidden_keys - valid keys pass");

// ============================================================================
// Test: Combined Map Validators - Key Length + Key Sets
// ============================================================================

// Test: min_key_length + max_key_length + required_keys - all constraints pass
constexpr bool test_combined_key_length_required_valid() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_key_length<2>,
        max_key_length<10>,
        required_keys<"id", "name">
    > consumer;
    
    std::string_view json = R"({"id": 1, "name": 2})";  // Keys length 2, all required present
    auto result = Parse(consumer, json);
    return result && consumer.get().count == 2;
}
static_assert(test_combined_key_length_required_valid(), "min_key_length + max_key_length + required_keys - all pass");

// Test: min_key_length + required_keys - key too short
constexpr bool test_combined_key_length_required_key_too_short() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_key_length<3>,
        required_keys<"id", "name">
    > consumer;
    
    std::string_view json = R"({"id": 1, "name": "test"})";  // "id" is only 2 chars < 3
    return !Parse(consumer, json);
}
static_assert(test_combined_key_length_required_key_too_short(), "min_key_length + required_keys - key too short");

// Test: max_key_length + allowed_keys - key too long
constexpr bool test_combined_key_length_allowed_key_too_long() {
    Annotated<
        MapConsumer<std::array<char, 32>, int, 10>,
        max_key_length<5>,
        allowed_keys<"id", "name", "verylongkey">
    > consumer;
    
    std::string_view json = R"({"id": 1, "verylongkey": 100})";  // "verylongkey" is 11 chars > 5
    return !Parse(consumer, json);
}
static_assert(test_combined_key_length_allowed_key_too_long(), "max_key_length + allowed_keys - key too long");

// ============================================================================
// Test: Combined Map Validators - All Three Key Validators Together
// ============================================================================

// Test: required_keys + allowed_keys + forbidden_keys - all constraints pass
constexpr bool test_combined_all_three_key_validators_valid() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        required_keys<"id", "name">,
        allowed_keys<"id", "name", "age", "email">,
        forbidden_keys<"__proto__", "constructor">
    > consumer;
    
    std::string_view json = R"({"id": 1, "name": 2, "age": 25})";  // All required, all allowed, no forbidden
    auto result = Parse(consumer, json);
    return result && consumer.get().count == 3;
}
static_assert(test_combined_all_three_key_validators_valid(), "required_keys + allowed_keys + forbidden_keys - all pass");

// Test: required_keys + allowed_keys + forbidden_keys - missing required
constexpr bool test_combined_all_three_missing_required() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        required_keys<"id", "name">,
        allowed_keys<"id", "name", "age">,
        forbidden_keys<"__proto__">
    > consumer;
    
    std::string_view json = R"({"id": 1, "age": 25})";  // "name" missing
    return !Parse(consumer, json);
}
static_assert(test_combined_all_three_missing_required(), "All three key validators - missing required");

// Test: required_keys + allowed_keys + forbidden_keys - not allowed
constexpr bool test_combined_all_three_not_allowed() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        required_keys<"id", "name">,
        allowed_keys<"id", "name", "age">,
        forbidden_keys<"__proto__">
    > consumer;
    
    std::string_view json = R"({"id": 1, "name": "test", "extra": 100})";  // "extra" not allowed
    return !Parse(consumer, json);
}
static_assert(test_combined_all_three_not_allowed(), "All three key validators - not allowed");

// Test: required_keys + allowed_keys + forbidden_keys - forbidden key
constexpr bool test_combined_all_three_forbidden() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        required_keys<"id", "name">,
        allowed_keys<"id", "name", "__proto__">,
        forbidden_keys<"__proto__">
    > consumer;
    
    std::string_view json = R"({"id": 1, "name": "test", "__proto__": 100})";  // "__proto__" forbidden
    return !Parse(consumer, json);
}
static_assert(test_combined_all_three_forbidden(), "All three key validators - forbidden key");

// ============================================================================
// Test: Combined Map Validators - Property Count + Key Validators
// ============================================================================

// Test: min_properties + max_properties + required_keys + allowed_keys - all pass
constexpr bool test_combined_properties_keys_valid() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_properties<2>,
        max_properties<5>,
        required_keys<"id", "name">,
        allowed_keys<"id", "name", "age", "email">
    > consumer;
    
    std::string_view json = R"({"id": 1, "name": 2, "age": 25})";  // 3 properties, all required, all allowed
    auto result = Parse(consumer, json);
    return result && consumer.get().count == 3;
}
static_assert(test_combined_properties_keys_valid(), "min_properties + max_properties + required_keys + allowed_keys - all pass");

// Test: min_properties + required_keys - fails property count
constexpr bool test_combined_properties_keys_fails_count() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        min_properties<3>,
        required_keys<"id", "name">,
        allowed_keys<"id", "name", "age">
    > consumer;
    
    std::string_view json = R"({"id": 1, "name": "test"})";  // Only 2 properties < 3
    return !Parse(consumer, json);
}
static_assert(test_combined_properties_keys_fails_count(), "min_properties + required_keys - fails property count");

// Test: max_properties + allowed_keys - fails property count
constexpr bool test_combined_properties_keys_fails_max_count() {
    Annotated<
        MapConsumer<std::array<char, 16>, int, 10>,
        max_properties<2>,
        allowed_keys<"id", "name", "age">
    > consumer;
    
    std::string_view json = R"({"id": 1, "name": "test", "age": 25})";  // 3 properties > 2
    return !Parse(consumer, json);
}
static_assert(test_combined_properties_keys_fails_max_count(), "max_properties + allowed_keys - fails property count");

// ============================================================================
// Test: Combined Map Validators - Key Length + All Key Validators
// ============================================================================

// Test: min_key_length + max_key_length + required_keys + allowed_keys + forbidden_keys - all pass
constexpr bool test_combined_all_map_validators_valid() {
    Annotated<
        MapConsumer<std::array<char, 32>, int, 10>,
        min_key_length<2>,
        max_key_length<10>,
        required_keys<"id", "name">,
        allowed_keys<"id", "name", "age", "email">,
        forbidden_keys<"__proto__">
    > consumer;
    
    std::string_view json = R"({"id": 1, "name": 2, "age": 25})";  // All constraints satisfied
    auto result = Parse(consumer, json);
    return result && consumer.get().count == 3;
}
static_assert(test_combined_all_map_validators_valid(), "All map validators combined - all pass");

// Test: All map validators - key length fails
constexpr bool test_combined_all_map_validators_key_length_fails() {
    Annotated<
        MapConsumer<std::array<char, 32>, int, 10>,
        min_key_length<3>,
        max_key_length<10>,
        required_keys<"id", "name">,
        allowed_keys<"id", "name", "age">
    > consumer;
    
    std::string_view json = R"({"id": 1, "name": "test"})";  // "id" is only 2 chars < 3
    return !Parse(consumer, json);
}
static_assert(test_combined_all_map_validators_key_length_fails(), "All map validators - key length fails");

