#include "../test_helpers.hpp"
#include <JsonFusion/json_schema.hpp>
#include <JsonFusion/annotated.hpp>
#include <JsonFusion/validators.hpp>
#include <JsonFusion/json.hpp>
#include <string>
#include <vector>
#include <map>
#include <optional>

using namespace JsonFusion;
using namespace JsonFusion::options;
using namespace JsonFusion::validators;

// ============================================================================
// JSON Schema Test Helpers
// ============================================================================

namespace {
struct limitless_sentinel {};

constexpr bool operator==(const std::back_insert_iterator<std::string>&,
                         const limitless_sentinel&) noexcept {
    return false;
}

constexpr bool operator==(const limitless_sentinel&,
                         const std::back_insert_iterator<std::string>&) noexcept {
    return false;
}

constexpr bool operator!=(const std::back_insert_iterator<std::string>& it,
                         const limitless_sentinel& s) noexcept {
    return !(it == s);
}

constexpr bool operator!=(const limitless_sentinel& s,
                         const std::back_insert_iterator<std::string>& it) noexcept {
    return !(it == s);
}

/// Test JSON schema generation (inline, no metadata)
template <typename T>
    requires JsonFusion::static_schema::JsonParsableValue<T>
constexpr bool TestSchemaInline(std::string_view expected) {
    std::string out;
    auto it = std::back_inserter(out);
    limitless_sentinel end{};
    
    JsonFusion::JsonIteratorWriter<decltype(it), limitless_sentinel> writer(it, end);
    if (!JsonFusion::json_schema::WriteSchemaInline<T>(writer)) return false;
    if (!writer.finish()) return false;
    
    return out == expected;
}

/// Test JSON schema generation (with metadata wrapper)
template <typename T>
    requires JsonFusion::static_schema::JsonParsableValue<T>
constexpr bool TestSchema(std::string_view expected,
                          const char* title = nullptr,
                          const char* schema_uri = "https://json-schema.org/draft/2020-12/schema") {
    std::string out;
    auto it = std::back_inserter(out);
    limitless_sentinel end{};
    
    JsonFusion::JsonIteratorWriter<decltype(it), limitless_sentinel> writer(it, end);
    if (!JsonFusion::json_schema::WriteSchema<T>(writer, title, schema_uri)) return false;
    if (!writer.finish()) return false;
    
    return out == expected;
}
} // anonymous namespace

// ============================================================================
// Test Type Definitions - Demonstrating ALL Validators and Options
// ============================================================================

// Nested structure demonstrating various validators and options
struct Address {
    std::string street;
    A<std::string, min_length<1>, max_length<100>> city;
    A<int, key<"zip_code">> zipCode;  // Option: key<> - custom key name
    A<std::string, enum_values<"house", "apartment", "office">> type;
    
    // Option: as_array - serialize as tuple [lat, lon]
    struct Coordinates {
        float latitude;
        float longitude;
    };
    A<Coordinates, as_array> coordinates;
    
    // Option: not_json - excluded from schema and serialization
    A<std::string, not_json> internal_id;
    
    // Option: json_sink - accepts any JSON value
    A<std::string, json_sink<>> metadata;
};

// Structure with arrays and optional fields
struct Person_ {
    std::string name;
    A<int, range<0, 100>> age;
    std::optional<std::string> email;
    A<std::vector<Address>, min_items<1>, max_items<10>> addresses;
};
// Validator: required<...> - specifies required fields
using Person = A<Person_, required<"name", "email">>;

// Structure with maps and various validators
struct Configuration_ {
    // Map validators: min_properties, max_properties, min_key_length, max_key_length
    A<std::map<std::string, std::string>, min_properties<1>, max_properties<10>, min_key_length<1>, max_key_length<10>> settings;
    
    // Map validators: allowed_keys, required_keys
    A<std::map<std::string, std::optional<bool>>,  allowed_keys<"key1", "key2">, required_keys<"key1">> flags1;
    
    // Map validators: forbidden_keys, required_keys
    A<std::map<std::string, bool>,  forbidden_keys<"key1", "key2">, required_keys<"key3">> flags2;
    
    bool enabled;
    
    // Validator: string_constant<...>
    A<std::string, string_constant<"configuration">> object_type;
    
    // Validator: constant<N>
    A<int, constant<14>> version;
};
// Validator: not_required<...> - specifies fields that are NOT required (all others become required)
// Option: allow_excess_fields<> - allows additional properties not defined in schema
using Configuration = A<Configuration_, not_required<"settings">, allow_excess_fields<>>;

// Option: indexes_as_keys with int_key<N> - for CBOR-style numeric keys
struct IndexedData_ {
    int field0;                    // Auto: index 0
    A<int, int_key<10>> field10;   // Explicit: index 10 (Option: int_key<N>)
    int field11;                   // Auto: index 11 (10+1, enum-like)
    A<int, int_key<100>> field100; // Explicit: index 100
    int field101;                  // Auto: index 101 (100+1)
};
// Option: indexes_as_keys - use numeric indices as property names
using IndexedData = A<IndexedData_, indexes_as_keys>;

// Recursive type - tree structure (tests cycle detection and $ref generation)
struct TreeModel {
    std::string data;
    A<std::vector<TreeModel>, max_items<10>> children;
};

// Struct with forbidden fields - demonstrates forbidden validator
struct LegacyAPI_ {
    std::string username;
    std::string email;
    int user_id;
};
using LegacyAPI = A<LegacyAPI_, forbidden<"password", "ssn">, allow_excess_fields<>>;

// ============================================================================
// Tests - Comprehensive Coverage of All Validators and Options
// ============================================================================

// Test 1: Address - demonstrates key<>, enum_values, min_length, max_length, as_array, not_json, json_sink
static_assert(TestSchemaInline<Address>(
    R"({"additionalProperties":false,"type":"object","properties":{"street":{"type":"string"},"city":{"type":"string","minLength":1,"maxLength":100},"zip_code":{"type":"integer"},"type":{"enum":["apartment","house","office"]},"coordinates":{"type":"array","prefixItems":[{"type":"number"},{"type":"number"}],"minItems":2,"maxItems":2},"metadata":{}}})"
));

// Test 2: Person - demonstrates range, min_items, max_items, required<...>, optional fields (oneOf with null)
static_assert(TestSchemaInline<Person>(
    R"({"additionalProperties":false,"type":"object","properties":{"name":{"type":"string"},"age":{"type":"integer","minimum":0,"maximum":100},"email":{"oneOf":[{"type":"string"},{"type":"null"}]},"addresses":{"type":"array","minItems":1,"maxItems":10,"items":{"additionalProperties":false,"type":"object","properties":{"street":{"type":"string"},"city":{"type":"string","minLength":1,"maxLength":100},"zip_code":{"type":"integer"},"type":{"enum":["apartment","house","office"]},"coordinates":{"type":"array","prefixItems":[{"type":"number"},{"type":"number"}],"minItems":2,"maxItems":2},"metadata":{}}}}},"required":["name","email"]})"
));

// Test 3: Configuration - demonstrates min_properties, max_properties, min_key_length, max_key_length,
//         allowed_keys, required_keys, forbidden_keys, string_constant, constant, not_required<...>, allow_excess_fields<>
static_assert(TestSchemaInline<Configuration>(
    R"({"type":"object","properties":{"settings":{"type":"object","minProperties":1,"maxProperties":10,"propertyNames":{"minLength":1,"maxLength":10},"additionalProperties":{"type":"string"}},"flags1":{"type":"object","properties":{"key1":{"oneOf":[{"type":"boolean"},{"type":"null"}]},"key2":{"oneOf":[{"type":"boolean"},{"type":"null"}]}},"required":["key1"],"additionalProperties":false},"flags2":{"type":"object","propertyNames":{"not":{"enum":["key1","key2"]}},"properties":{"key3":{"type":"boolean"}},"required":["key3"],"additionalProperties":{"type":"boolean"}},"enabled":{"type":"boolean"},"object_type":{"const":"configuration"},"version":{"const":14}},"required":["flags1","flags2","enabled","object_type","version"]})"
));

// Test 4: Person with metadata wrapper - demonstrates WriteSchema (with $schema and title)
static_assert(TestSchema<Person>(
    R"({"$schema":"https://json-schema.org/draft/2020-12/schema","title":"Person Schema","definition":{"additionalProperties":false,"type":"object","properties":{"name":{"type":"string"},"age":{"type":"integer","minimum":0,"maximum":100},"email":{"oneOf":[{"type":"string"},{"type":"null"}]},"addresses":{"type":"array","minItems":1,"maxItems":10,"items":{"additionalProperties":false,"type":"object","properties":{"street":{"type":"string"},"city":{"type":"string","minLength":1,"maxLength":100},"zip_code":{"type":"integer"},"type":{"enum":["apartment","house","office"]},"coordinates":{"type":"array","prefixItems":[{"type":"number"},{"type":"number"}],"minItems":2,"maxItems":2},"metadata":{}}}}},"required":["name","email"]}})",
    "Person Schema"
));

// Test 5: IndexedData - demonstrates indexes_as_keys and int_key<N> with enum-like semantics
static_assert(TestSchemaInline<IndexedData>(
    R"({"additionalProperties":false,"type":"object","properties":{"0":{"type":"integer"},"10":{"type":"integer"},"11":{"type":"integer"},"100":{"type":"integer"},"101":{"type":"integer"}}})"
));

// Test 6: TreeModel - demonstrates recursive types with cycle detection and $ref
static_assert(TestSchemaInline<TreeModel>(
    R"({"additionalProperties":false,"type":"object","properties":{"data":{"type":"string"},"children":{"type":"array","maxItems":10,"items":{"$ref":"#"}}}})"
));

// Test 7: LegacyAPI - demonstrates forbidden<...> validator for structs
static_assert(TestSchemaInline<LegacyAPI>(
    R"({"type":"object","properties":{"username":{"type":"string"},"email":{"type":"string"},"user_id":{"type":"integer"}},"propertyNames":{"not":{"enum":["password","ssn"]}}})"
));

// ============================================================================
// Summary of Coverage
// ============================================================================
/*
 * VALIDATORS TESTED:
 * ✓ range<Min, Max>              - numeric range constraints
 * ✓ min_length<N>                - minimum string length
 * ✓ max_length<N>                - maximum string length
 * ✓ constant<N>                  - constant numeric value
 * ✓ string_constant<"value">     - constant string value
 * ✓ enum_values<"v1", "v2"...>   - enumeration of allowed string values
 * ✓ min_items<N>                 - minimum array length
 * ✓ max_items<N>                 - maximum array length
 * ✓ min_properties<N>            - minimum map size
 * ✓ max_properties<N>            - maximum map size
 * ✓ min_key_length<N>            - minimum map key length
 * ✓ max_key_length<N>            - maximum map key length
 * ✓ required<"f1", "f2"...>      - explicitly required struct fields
 * ✓ not_required<"f1", "f2"...>  - explicitly optional struct fields (others become required)
 * ✓ forbidden<"f1", "f2"...>     - forbidden struct fields (like deprecated ones)
 * ✓ required_keys<"k1", "k2"...> - required map keys
 * ✓ allowed_keys<"k1", "k2"...>  - allowed map keys (restrictive)
 * ✓ forbidden_keys<"k1"...>      - forbidden map keys
 * 
 * OPTIONS TESTED:
 * ✓ key<"custom_name">           - custom JSON property name
 * ✓ int_key<N>                   - custom numeric index (CBOR-oriented)
 * ✓ not_json                     - exclude field from schema/serialization
 * ✓ json_sink<>                  - accept any JSON value (no schema constraint)
 * ✓ allow_excess_fields<>        - allow additional properties
 * ✓ as_array                     - serialize struct as tuple (prefixItems)
 * ✓ indexes_as_keys              - use numeric indices as property names
 * 
 * OTHER FEATURES TESTED:
 * ✓ std::optional<T>             - nullable types (oneOf with null)
 * ✓ Nested structures            - recursive schema generation
 * ✓ Arrays and vectors           - items schema
 * ✓ Maps                         - additionalProperties schema
 * ✓ Metadata wrapper             - $schema and title properties
 * ✓ Enum-like index semantics    - int_key<N> follows C++ enum rules
 * ✓ additionalProperties:false   - default for objects without allow_excess_fields
 * ✓ Recursive types              - cycle detection with {"$ref": "#"}
 */

// Main function for running the test
int main() {
    // All tests are static_assert, so if we compile, we pass
    return 0;
}
