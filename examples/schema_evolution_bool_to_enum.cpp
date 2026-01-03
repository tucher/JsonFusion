#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>
// #include <JsonFusion/yyjson.hpp>
#include <iostream>
#include <cassert>
#include <functional>

using namespace JsonFusion;
using namespace JsonFusion::options;

// ============================================================================
// Generic Schema Backward Compatibility Helper
// ============================================================================

/// Generic backward compatible field: accepts OldWireT or NewWireT, stores as StorageT
// Always serializes as NewWireT (canonical form), even if parsed from OldWireT. 
/// 
/// This transformer uses WireSink to capture raw JSON, then tries parsing
/// as both the old and new wire types. Perfect for schema evolution scenarios.
///
/// Template parameters:
///   OldWireT       - Old JSON wire type (e.g., bool)
///   NewWireT       - New JSON wire type (e.g., int)
///   StorageT       - Internal storage type (e.g., enum, or same as NewWireT)
///   OldConvertFn   - Function pointer (OldWireT → StorageT)
///   NewConvertFn   - Function pointer (NewWireT → StorageT), often identity
///   ToWireFn       - Function pointer (StorageT → NewWireT) for serialization
///   BufferSize     - Size of WireSink buffer
template<class OldWireT, class NewWireT, class StorageT, 
         auto OldConvertFn, auto NewConvertFn, auto ToWireFn,
         std::size_t BufferSize = 64>
struct CompatibleField {
    using wire_type = WireSink<BufferSize>;
    StorageT value{};
    
    constexpr bool transform_from(const auto & parseFn) {
        // Try parsing as new wire type first
        NewWireT new_val;
        if (auto r = parseFn(new_val)) {
            return std::invoke(NewConvertFn, new_val, value);
        }

        // Try parsing as old wire type if new wire type fails, legacy is a fallback.
        OldWireT old_val;
        if (auto r = parseFn(old_val)) {
            return std::invoke(OldConvertFn, old_val, value);
        }
        
        return false;
    }
    
    constexpr bool transform_to(const auto & serializeFn) const {
        // Convert storage to wire, then serialize directly to WireSink
        NewWireT wire_val = std::invoke(ToWireFn, value);
        return !!serializeFn(wire_val);
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

// CompatibleField field with 3 distinct types:
// - Old JSON: bool (true/false)
// - New JSON: int (0/1/2)
// - Storage: type-safe enum

constexpr bool int_to_state(int i, State & out) {
    if (i < 0 || i > 2) return false;
    out = static_cast<State>(i); 
    return true; 
}
using BoolOrIntToEnum = CompatibleField<
    bool,   // Old wire: JSON bool
    int,    // New wire: JSON int
    State,  // Storage: C++ enum (type-safe!)
    [](bool b, State & out) { // bool → enum
        out = b ? State::Enabled : State::Disabled; 
        return true; 
    },
    int_to_state,
    [](State s) { return static_cast<int>(s); }  // enum → int for serialize
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
    
    // New JSON with int that is out of range
    if (Parse(config, R"({"name": "service", "enabled": 42})")) return false;
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

/*
// Runtime-only test using YyJSON (tests WireSink RAII with DOM handles)
bool test_migration_path_yyjson() {
    // Start with V1 JSON (bool)
    std::string json_v1 = R"({"name": "app", "enabled": true})";
    
    // Parse with V2 using YyJSON (migration schema)
    // This exercises WireSink with YyJSON node pointers + RAII cleanup
    ConfigV2 v2;
    {
        YyjsonReader reader(json_v1.data(), json_v1.size());
        if (!ParseWithReader(v2, reader)) return false;
    }
    if (v2.enabled != State::Enabled) return false;
    
    // Serialize with V2 using YyJSON
    // Transformer creates temporary YyjsonWriter → stores [doc*, node*] in WireSink
    // WireSink takes ownership via cleanup callback (no memory leaks!)
    std::string json_v2;
    {
        YyjsonWriter writer(json_v2);  // Serialize to string
        if (!SerializeWithWriter(v2, writer)) return false;
        if (!writer.finish()) return false;
    }
    
    // Verify output contains int (not bool)
    if (json_v2.find("\"enabled\":1") == std::string::npos) return false;
    
    // Parse with V3 using YyJSON (new schema, int wire format)
    ConfigV3 v3;
    {
        YyjsonReader reader(json_v2.data(), json_v2.size());
        if (!ParseWithReader(v3, reader)) return false;
    }
    if (v3.enabled != 1) return false;  // State::Enabled = 1
    
    // Test multiple serialization (WireSink immutability)
    // The WireSink in transformer should be reusable
    std::string json_v2_again;
    {
        YyjsonWriter writer(json_v2_again);
        if (!SerializeWithWriter(v2, writer)) return false;
        if (!writer.finish()) return false;
    }
    if (json_v2 != json_v2_again) return false;  // Should produce identical output
    
    return true;
}
*/

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
    
    /*
    std::cout << "\nTest 4: YyJSON migration path (WireSink RAII with DOM)\n";
    assert(test_migration_path_yyjson());
    std::cout << "  ✓ V1 (bool) → V2 (enum) → V3 (int) with YyJSON DOM\n";
    std::cout << "  ✓ WireSink stores [doc*, node*] with O(1) capture\n";
    std::cout << "  ✓ RAII cleanup frees documents automatically (no leaks!)\n";
    std::cout << "  ✓ Multiple serializations work (immutable WireSink)\n";
    */
    std::cout << "\n✅ All runtime tests passed!\n";
    std::cout << "\n=== Key Features ===\n";
    std::cout << "✓ Three distinct types: OldWire, NewWire, Storage\n";
    std::cout << "✓ Accepts JSON bool or int, stores type-safe enum\n";
    std::cout << "✓ Serializes enum as int for JSON compatibility\n";
    std::cout << "✓ Configurable buffer size via template parameter\n";
    std::cout << "✓ Works in constexpr contexts (proven by static_assert!)\n";
    // std::cout << "✓ YyJSON: O(1) WireSink capture with automatic resource cleanup\n";
    
    return 0;
}
