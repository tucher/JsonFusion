#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>
#include <iostream>
#include <cassert>

using namespace JsonFusion;
using namespace JsonFusion::options;

// ============================================================================
// Generic Schema Migration Helper
// ============================================================================

/// Generic migration field: accepts OldWireT or NewWireT, stores as StorageT
/// 
/// This transformer uses json_sink to capture raw JSON, then tries parsing
/// as both the old and new wire types. Perfect for schema evolution scenarios.
///
/// Template parameters:
///   OldWireT       - Old JSON wire type (e.g., bool)
///   NewWireT       - New JSON wire type (e.g., int)
///   StorageT       - Internal storage type (e.g., enum, or same as NewWireT)
///   OldConvertFn   - Lambda (OldWireT → StorageT)
///   NewConvertFn   - Lambda (NewWireT → StorageT), often identity
///   ToWireFn       - Lambda (StorageT → NewWireT) for serialization
///   BufferSize     - Size of json_sink buffer
template<class OldWireT, class NewWireT, class StorageT, 
         auto OldConvertFn, auto NewConvertFn, auto ToWireFn,
         std::size_t BufferSize = 64>
struct MigrationField {
    using wire_type = A<std::array<char, BufferSize>, json_sink<>>;
    StorageT value{};
    
    constexpr bool transform_from(const std::array<char, BufferSize>& json_buf) {
        // Try parsing as old wire type
        OldWireT old_val;
        if (auto r = Parse(old_val, json_buf.data())) {
            value = std::invoke(OldConvertFn, old_val);
            return true;
        }
        
        // Try parsing as new wire type
        NewWireT new_val;
        if (auto r = Parse(new_val, json_buf.data())) {
            value = std::invoke(NewConvertFn, new_val);
            return true;
        }
        
        return false;
    }
    
    constexpr bool transform_to(std::array<char, BufferSize>& out) const {
        // Convert storage to wire, then serialize
        NewWireT wire_val = std::invoke(ToWireFn, value);
        auto it = out.begin();
        return !!Serialize(wire_val, it, out.end());
    }
    
    // Ergonomics: implicit conversions and comparisons
    constexpr operator const StorageT&() const { return value; }
    constexpr operator StorageT&() { return value; }
    constexpr auto operator<=>(const StorageT& other) const { return value <=> other; }
};

// ============================================================================
// Example: bool → enum (serialized as int)
// ============================================================================

// The schema evolution: bool → enum with multiple states
enum class State { Disabled = 0, Enabled = 1, Debug = 2 };

// Migration field with 3 distinct types:
// - Old JSON: bool (true/false)
// - New JSON: int (0/1/2)
// - Storage: type-safe enum
using BoolOrIntToEnum = MigrationField<
    bool,   // Old wire: JSON bool
    int,    // New wire: JSON int
    State,  // Storage: C++ enum (type-safe!)
    [](bool b) { return b ? State::Enabled : State::Disabled; },  // bool → enum
    [](int i) { return static_cast<State>(i); },                  // int → enum
    [](State s) { return static_cast<int>(s); }                   // enum → int for serialize
>;


// Schema versions
struct ConfigV1 {
    std::string name;
    bool enabled;  // Original: simple bool (true/false)
};

struct ConfigV2 {
    std::string name;
    BoolOrIntToEnum enabled;  // Migration: accepts bool OR int, stores as enum
};

struct ConfigV3 {
    std::string name;
    int enabled;  // New JSON format: int (0/1/2/...)
    // Note: In real code, you'd wrap this in your own type with enum internally
};

// ============================================================================
// Tests (constexpr for compile-time validation)
// ============================================================================

constexpr bool test_parse_both_types() {
    ConfigV2 config;
    
    // Old JSON with bool
    if (!Parse(config, R"({"name": "service", "enabled": true})")) return false;
    if (config.enabled != State::Enabled) return false;
    
    if (!Parse(config, R"({"name": "service", "enabled": false})")) return false;
    if (config.enabled != State::Disabled) return false;
    
    // New JSON with int
    if (!Parse(config, R"({"name": "service", "enabled": 2})")) return false;
    if (config.enabled != State::Debug) return false;
    
    return true;
}

constexpr bool test_serialization() {
    ConfigV2 config;
    
    // Parse bool, should serialize as int
    if (!Parse(config, R"({"name": "test", "enabled": true})")) return false;
    std::string output;
    if (!Serialize(config, output)) return false;
    if (output.find("\"enabled\":1") == std::string::npos) return false;
    
    // Parse int for Debug state, should serialize as int 2
    if (!Parse(config, R"({"name": "test", "enabled": 2})")) return false;
    if (!Serialize(config, output)) return false;
    if (output.find("\"enabled\":2") == std::string::npos) return false;
    
    return true;
}

constexpr bool test_migration_path() {
    // Start with V1 JSON (bool)
    std::string json_v1 = R"({"name": "app", "enabled": true})";
    
    // Parse with V2 (migration schema)
    ConfigV2 v2;
    if (!Parse(v2, json_v1)) return false;
    if (v2.enabled != State::Enabled) return false;
    
    // Serialize with V2 (enum serializes as int)
    std::string json_v2;
    if (!Serialize(v2, json_v2)) return false;
    
    // Parse with V3 (new schema, int wire format)
    ConfigV3 v3;
    if (!Parse(v3, json_v2)) return false;
    if (v3.enabled != 1) return false;  // State::Enabled = 1
    
    return true;
}

// ============================================================================
// Main - both compile-time and runtime validation
// ============================================================================

int main() {
    std::cout << "=== JsonFusion Schema Evolution: bool → enum ===\n\n";
    
    // Compile-time checks (validated at compilation)
    static_assert(test_parse_both_types(), "Compile-time: parse both types failed");
    static_assert(test_serialization(), "Compile-time: serialization failed");
    static_assert(test_migration_path(), "Compile-time: migration path failed");
    
    std::cout << "✅ Compile-time checks passed!\n\n";
    
    // Runtime checks (with output)
    std::cout << "Test 1: Parse both bool and int values\n";
    assert(test_parse_both_types());
    std::cout << "  ✓ Parses bool true → State::Enabled\n";
    std::cout << "  ✓ Parses bool false → State::Disabled\n";
    std::cout << "  ✓ Parses int 2 → State::Debug\n";
    
    std::cout << "\nTest 2: Serialization (enum serializes as int)\n";
    assert(test_serialization());
    std::cout << "  ✓ State::Enabled serializes as int 1\n";
    std::cout << "  ✓ State::Debug serializes as int 2\n";
    
    std::cout << "\nTest 3: Full migration path V1 → V2 → V3\n";
    assert(test_migration_path());
    std::cout << "  ✓ V1 (bool) → V2 (enum storage) → V3 (int wire) works correctly\n";
    
    std::cout << "\n✅ All runtime tests passed!\n";
    std::cout << "\n=== Key Features ===\n";
    std::cout << "✓ Three distinct types: OldWire, NewWire, Storage\n";
    std::cout << "✓ Accepts JSON bool or int, stores type-safe enum\n";
    std::cout << "✓ Serializes enum as int for JSON compatibility\n";
    std::cout << "✓ Configurable buffer size via template parameter\n";
    std::cout << "✓ Works in constexpr contexts (proven by static_assert!)\n";
    
    return 0;
}
