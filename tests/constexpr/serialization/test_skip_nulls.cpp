#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/annotated.hpp>
#include <JsonFusion/options.hpp>
#include <optional>
#include <string>

using namespace JsonFusion;
using namespace JsonFusion::options;

// ============================================================================
// skip_nulls omits null optional fields during serialization
// This is useful for sparse data structures and reducing output size
// ============================================================================

// Test structures
struct Config {
    int required_field;
    std::optional<int> optional_field1;
    std::optional<int> optional_field2;
    std::optional<int> optional_field3;

    constexpr bool operator==(const Config& other) const {
        return required_field == other.required_field
            && optional_field1 == other.optional_field1
            && optional_field2 == other.optional_field2
            && optional_field3 == other.optional_field3;
    }
};

// ============================================================================
// Test: skip_nulls omits null fields from output
// ============================================================================

constexpr bool test_skip_nulls_omits_null_fields() {
    A<Config, skip_nulls> c{};
    c.value.required_field = 42;
    c.value.optional_field1 = std::nullopt;
    c.value.optional_field2 = 100;
    c.value.optional_field3 = std::nullopt;

    std::string json;
    if (!Serialize(c, json)) return false;

    // Should NOT contain null fields
    if (json.find("null") != std::string::npos) return false;

    // Should contain the present optional field
    if (json.find("optional_field2") == std::string::npos) return false;
    if (json.find("100") == std::string::npos) return false;

    return true;
}
static_assert(test_skip_nulls_omits_null_fields(),
              "skip_nulls should omit null optional fields");

// ============================================================================
// Test: skip_nulls roundtrips correctly
// ============================================================================

constexpr bool test_skip_nulls_roundtrip() {
    A<Config, skip_nulls> original{};
    original.value.required_field = 42;
    original.value.optional_field1 = std::nullopt;
    original.value.optional_field2 = 100;
    original.value.optional_field3 = std::nullopt;

    std::string json;
    if (!Serialize(original, json)) return false;

    A<Config, skip_nulls> parsed{};
    if (!Parse(parsed, json)) return false;

    // Values should match
    if (parsed.value.required_field != 42) return false;
    if (parsed.value.optional_field1.has_value()) return false;
    if (parsed.value.optional_field2 != 100) return false;
    if (parsed.value.optional_field3.has_value()) return false;

    return true;
}
static_assert(test_skip_nulls_roundtrip(),
              "skip_nulls should roundtrip correctly");

// ============================================================================
// Test: skip_nulls produces smaller output than regular serialization
// ============================================================================

constexpr bool test_skip_nulls_is_smaller() {
    Config data{};
    data.required_field = 42;
    data.optional_field1 = std::nullopt;
    data.optional_field2 = std::nullopt;
    data.optional_field3 = std::nullopt;

    // With skip_nulls
    A<Config, skip_nulls> with_skip{};
    with_skip.value = data;

    std::string json_with_skip;
    if (!Serialize(with_skip, json_with_skip)) return false;

    // Without skip_nulls
    std::string json_without_skip;
    if (!Serialize(data, json_without_skip)) return false;

    // skip_nulls should produce smaller output (no null fields)
    return json_with_skip.size() < json_without_skip.size();
}
static_assert(test_skip_nulls_is_smaller(),
              "skip_nulls should produce smaller output");

// ============================================================================
// Test: skip_nulls with all fields present
// ============================================================================

constexpr bool test_skip_nulls_all_present() {
    A<Config, skip_nulls> c{};
    c.value.required_field = 1;
    c.value.optional_field1 = 2;
    c.value.optional_field2 = 3;
    c.value.optional_field3 = 4;

    std::string json;
    if (!Serialize(c, json)) return false;

    // All fields should be present
    if (json.find("required_field") == std::string::npos) return false;
    if (json.find("optional_field1") == std::string::npos) return false;
    if (json.find("optional_field2") == std::string::npos) return false;
    if (json.find("optional_field3") == std::string::npos) return false;

    return true;
}
static_assert(test_skip_nulls_all_present(),
              "skip_nulls should keep all present fields");

// ============================================================================
// Test: skip_nulls with all fields null
// ============================================================================

constexpr bool test_skip_nulls_all_null() {
    A<Config, skip_nulls> c{};
    c.value.required_field = 99;
    c.value.optional_field1 = std::nullopt;
    c.value.optional_field2 = std::nullopt;
    c.value.optional_field3 = std::nullopt;

    std::string json;
    if (!Serialize(c, json)) return false;

    // Only required field should be present
    if (json.find("required_field") == std::string::npos) return false;
    if (json.find("optional_field1") != std::string::npos) return false;
    if (json.find("optional_field2") != std::string::npos) return false;
    if (json.find("optional_field3") != std::string::npos) return false;

    return true;
}
static_assert(test_skip_nulls_all_null(),
              "skip_nulls should omit all null fields");

// ============================================================================
// Test: skip_nulls with indexes_as_keys (sparse map)
// ============================================================================

struct SparseData {
    std::optional<int> field0;
    std::optional<int> field1;
    std::optional<int> field2;
    std::optional<int> field3;

    constexpr bool operator==(const SparseData& other) const {
        return field0 == other.field0
            && field1 == other.field1
            && field2 == other.field2
            && field3 == other.field3;
    }
};

constexpr bool test_skip_nulls_with_indexes_as_keys() {
    A<SparseData, indexes_as_keys, skip_nulls> sparse{};
    sparse.value.field0 = std::nullopt;
    sparse.value.field1 = 100;
    sparse.value.field2 = std::nullopt;
    sparse.value.field3 = 200;

    std::string json;
    if (!Serialize(sparse, json)) return false;

    // Should have indices as keys, no nulls
    // Expected: {"1":100,"3":200}
    if (json.find("null") != std::string::npos) return false;
    if (json.find("\"1\"") == std::string::npos) return false;
    if (json.find("\"3\"") == std::string::npos) return false;

    return true;
}
static_assert(test_skip_nulls_with_indexes_as_keys(),
              "skip_nulls should work with indexes_as_keys");

// ============================================================================
// Test: skip_nulls roundtrip with indexes_as_keys
// ============================================================================

constexpr bool test_skip_nulls_indexes_roundtrip() {
    A<SparseData, indexes_as_keys, skip_nulls> original{};
    original.value.field0 = std::nullopt;
    original.value.field1 = 100;
    original.value.field2 = std::nullopt;
    original.value.field3 = 200;

    std::string json;
    if (!Serialize(original, json)) return false;

    A<SparseData, indexes_as_keys, skip_nulls> parsed{};
    if (!Parse(parsed, json)) return false;

    // Should match original
    if (parsed.value.field0.has_value()) return false;
    if (parsed.value.field1 != 100) return false;
    if (parsed.value.field2.has_value()) return false;
    if (parsed.value.field3 != 200) return false;

    return true;
}
static_assert(test_skip_nulls_indexes_roundtrip(),
              "skip_nulls with indexes_as_keys should roundtrip");

