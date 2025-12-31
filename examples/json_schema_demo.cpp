/**
 * JSON Schema Generation Demo
 * 
 * This example demonstrates comprehensive JSON Schema generation from C++ types
 * using JsonFusion. It covers ALL available validators and options.
 * 
 * For constexpr tests, see: tests/constexpr/json_schema/test_json_schema_combined.cpp
 */

#include <JsonFusion/json_schema.hpp>
#include <JsonFusion/annotated.hpp>
#include <JsonFusion/validators.hpp>
#include <JsonFusion/json.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <optional>

using namespace JsonFusion;
using namespace JsonFusion::options;
using namespace JsonFusion::validators;
// Helper for std::string output
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

template <typename T>
    requires JsonFusion::static_schema::ParsableValue<T>
bool WriteSchema(std::string& out, 
                 const char* title = nullptr,
                 const char* schema_uri = "https://json-schema.org/draft/2020-12/schema") {
    out.clear();
    auto it = std::back_inserter(out);
    limitless_sentinel end{};
    
    JsonFusion::JsonIteratorWriter<decltype(it), limitless_sentinel> writer(it, end);
    return JsonFusion::json_schema::WriteSchema<T>(writer, title, schema_uri) && writer.finish();
}

template <typename T, bool Pretty = false>
    requires JsonFusion::static_schema::ParsableValue<T>
bool WriteSchemaInline(std::string& out) {
    out.clear();
    auto it = std::back_inserter(out);
    limitless_sentinel end{};
    
    JsonFusion::JsonIteratorWriter<decltype(it), limitless_sentinel, Pretty> writer(it, end);
    return JsonFusion::json_schema::WriteSchemaInline<T>(writer) && writer.finish();
}
} // anonymous namespace

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
    
    // Option: exclude - excluded from schema and serialization
    A<std::string, exclude> internal_id;
    
    // Option: wire_sink - accepts any JSON value
    A<std::string, wire_sink<>> metadata;
};

// Structure with arrays and optional fields
struct Person_ {
    std::string name;
    A<int, range<0, 100>> age;
    std::optional<std::string> email;
    A<std::vector<Address>, min_items<1>, max_items<10>> addresses;
};
using Person = A<Person_, required<"name", "email">>;
// Structure with maps
struct Configuration_ {
    A<std::map<std::string, std::string>, min_properties<1>, max_properties<10>, min_key_length<1>, max_key_length<10>> settings;
    A<std::map<std::string, std::optional<bool>>,  allowed_keys<"key1", "key2">, required_keys<"key1">> flags1;
    A<std::map<std::string, bool>,  forbidden_keys<"key1", "key2">, required_keys<"key3">> flags2;
    bool enabled;
    A<std::string, string_constant<"configuration">> object_type;
    A<int, constant<14>> version;
};
using Configuration = A<Configuration_, not_required<"settings">, allow_excess_fields<>>;

// Option: indexes_as_keys with int_key<N> - for CBOR-style numeric keys
struct IndexedData_ {
    int field0;                    // Auto: index 0
    A<int, int_key<10>> field10;   // Explicit: index 10
    int field11;                   // Auto: index 11 (10+1, enum-like)
    A<int, int_key<100>> field100; // Explicit: index 100
    int field101;                  // Auto: index 101 (100+1)
};
using IndexedData = A<IndexedData_, indexes_as_keys>;

void print_schema(const std::string& title, const std::string& schema) {
    std::cout << "\n=== " << title << " ===\n";
    std::cout << schema << "\n";
}

struct TreeModel {
    std::string data;
    A<std::vector<TreeModel>, max_items<10>> children;
};

// Struct with forbidden fields - demonstrates forbidden validator
struct LegacyAPI_ {
    std::string username;
    std::string email;
    int user_id;
    // These fields are deprecated/forbidden but we want to allow other excess fields
};
using LegacyAPI = A<LegacyAPI_, forbidden<"password", "ssn">, allow_excess_fields<>>;

int main() {
    std::string schema;
    
    std::cout << "JSON Schema Generation Demo\n";
    std::cout << "============================\n";
    std::cout << "\nThis demo showcases ALL validators and options available in JsonFusion.\n";
    std::cout << "Generated schemas conform to JSON Schema Draft 2020-12.\n\n";
    
    // Example 1: Address - String validators, enums, as_array, key<>, exclude, wire_sink
    WriteSchemaInline<Address>(schema);
    print_schema("Address Schema (Inline)", schema);
    std::cout << "  ✓ min_length, max_length - string length constraints\n";
    std::cout << "  ✓ enum_values - enumeration of allowed values\n";
    std::cout << "  ✓ key<> - custom JSON property name\n";
    std::cout << "  ✓ as_array - tuple-like array schema (prefixItems)\n";
    std::cout << "  ✓ exclude - field excluded from schema\n";
    std::cout << "  ✓ wire_sink - accepts any JSON value\n";
    
    // Example 2: Person - Numeric validators, arrays, optional, required
    schema.clear();
    WriteSchemaInline<Person>(schema);
    print_schema("Person Schema (with Optional & Arrays)", schema);
    std::cout << "  ✓ range<Min, Max> - numeric range constraints\n";
    std::cout << "  ✓ min_items, max_items - array length constraints\n";
    std::cout << "  ✓ std::optional<T> - nullable types (oneOf with null)\n";
    std::cout << "  ✓ required<...> - explicitly required fields\n";
    
    // Example 3: Configuration - Map validators, constants, allow_excess_fields
    schema.clear();
    WriteSchemaInline<Configuration>(schema);
    print_schema("Configuration Schema (with Maps)", schema);
    std::cout << "  ✓ min_properties, max_properties - map size constraints\n";
    std::cout << "  ✓ min_key_length, max_key_length - map key length constraints\n";
    std::cout << "  ✓ allowed_keys, required_keys - restrictive key set\n";
    std::cout << "  ✓ forbidden_keys - prohibited keys\n";
    std::cout << "  ✓ string_constant<\"value\"> - constant string\n";
    std::cout << "  ✓ constant<N> - constant number\n";
    std::cout << "  ✓ not_required<...> - explicitly optional fields\n";
    std::cout << "  ✓ allow_excess_fields - allows additional properties\n";
    
    // Example 4: With metadata wrapper ($schema and title)
    schema.clear();
    WriteSchema<Person>(schema, "Person Schema", 
                        "https://json-schema.org/draft/2020-12/schema");
    print_schema("Person Schema (with Metadata)", schema);
    std::cout << "  ✓ WriteSchema (vs WriteSchemaInline) - adds $schema and title\n";
    
    // Example 5: indexes_as_keys and int_key (CBOR-oriented)
    schema.clear();
    WriteSchemaInline<IndexedData>(schema);
    print_schema("IndexedData Schema (indexes_as_keys + int_key)", schema);
    std::cout << "  ✓ indexes_as_keys - numeric property names\n";
    std::cout << "  ✓ int_key<N> - custom index (follows C++ enum semantics)\n";
    std::cout << "  Note: Primarily for CBOR serialization\n";
    
    // Example 6: Pretty-printed output
    schema.clear();
    WriteSchemaInline<Address, true>(schema);  // Pretty = true
    print_schema("Address Schema (Pretty-Printed)", schema);
    std::cout << "  ✓ Pretty-printing with JsonIteratorWriter<It, Sent, Pretty=true>\n";
    std::cout << "  ✓ Automatic indentation and newlines\n";
    
    schema.clear();
    WriteSchemaInline<TreeModel, true>(schema);  // Pretty = true
    print_schema("TreeModel Schema (Pretty-Printed)", schema);
    std::cout << "  ✓ Recursive types with cycle detection (uses $ref)\n";
    
    // Example 7: Forbidden fields validator
    schema.clear();
    WriteSchemaInline<LegacyAPI, true>(schema);
    print_schema("LegacyAPI Schema (with Forbidden Fields)", schema);
    std::cout << "  ✓ forbidden<...> - prohibits specific fields (like deprecated ones)\n";
    std::cout << "  ✓ Works with allow_excess_fields to accept unknown fields but reject specific ones\n";

    std::cout << "\n✅ All validators and options demonstrated!\n";
    std::cout << "See tests/constexpr/json_schema/test_json_schema_combined.cpp for constexpr tests.\n";
    return 0;
}

