// Embedded Systems Example: JSON/CBOR Roundtrip Verification
// 
// Demonstrates:
// - Fixed-size arrays (std::array<char, N>, std::array<T, N>) - stack allocation only
// - Validation constraints (range, min_items, max_items, required fields)
// - JSON roundtrip (parse -> serialize -> parse)
// - CBOR roundtrip (parse JSON -> serialize CBOR -> parse CBOR)
//


#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/validators.hpp>
#include <JsonFusion/cbor.hpp>
#include <array>
#include <optional>
#include <cstdint>

using namespace JsonFusion;
using namespace JsonFusion::options;
using namespace JsonFusion::validators;


// Embedded Configuration Model
struct EmbeddedConfig {
    std::array<char, 32> device_name;
    uint16_t version_major;
    uint16_t version_minor;

    struct Network {
        std::array<char, 16> name{};
        std::array<char, 24> address{};
        A<uint16_t, range<1024, 65535>> port = 2000;
        bool enabled = false;
    };
    
    Network network;
    std::optional<Network> fallback_network;

    struct Motor {
        uint8_t id = 0;
        std::array<char, 16> name{};
        A<std::array<A<double, range<-1000, 1000>>, 3>, min_items<3>> position;  // [x,y,z]
        bool active = false;
    };
    
    A<std::array<Motor, 4>, min_items<1>, max_items<4>> motors;
};

// RPC Command Model
struct RpcCommand {
    std::array<char, 16> command_id{};
    uint64_t timestamp_us = 0;
    A<uint8_t, range<0, 10>> priority = 0;

    struct Target {
        std::array<char, 16> device_id{};
    };
    
    A<std::array<Target, 4>, min_items<1>, max_items<4>> targets;

    struct Parameter {
        std::array<char, 16> key{};
        std::optional<int64_t> int_value;
        A<std::optional<double>, range<-1000000, 1000000>> float_value;
    };
    
    A<std::array<Parameter, 8>, min_items<1>, max_items<8>> params;
};


// Verification helpers - forward declarations
constexpr bool verify_config_initial_parse(const EmbeddedConfig& config);
constexpr bool verify_config_roundtrip(const EmbeddedConfig& c1, const EmbeddedConfig& c2);
constexpr bool verify_rpc_initial_parse(const RpcCommand& cmd);
constexpr bool verify_rpc_roundtrip(const RpcCommand& cmd1, const RpcCommand& cmd2);

// Test functions
constexpr bool test1() {
    std::string_view json_input = R"({
        "device_name": "Controller-01",
        "version_major": 1,
        "version_minor": 2,
        "network": {
            "name": "eth0",
            "address": "192.168.1.100",
            "port": 8080,
            "enabled": true
        },
        "fallback_network": {
            "name": "wlan0",
            "address": "192.168.2.100",
            "port": 8081,
            "enabled": false
        },
        "motors": [
            {
                "id": 1,
                "name": "Motor-A",
                "position": [10.5, 20.3, -5.7],
                "active": true
            },
            {
                "id": 2,
                "name": "Motor-B",
                "position": [-15.2, 30.1, 8.9],
                "active": false
            }
        ]
    })";

    // Parse JSON
    EmbeddedConfig config1;
    if(!Parse(config1, json_input)) return false;
    if(!verify_config_initial_parse(config1)) return false;

    // Serialize back to JSON
    std::array<char, 512> json_output{};
    auto res = Serialize(config1, json_output.data(), json_output.data() + json_output.size());
    if(!res) return false;
    json_output[res.bytesWritten()] = '\0';

    // Parse serialized JSON (roundtrip verification)
    EmbeddedConfig config2;
    if(!Parse(config2, json_output.data())) return false;
    if(!verify_config_roundtrip(config1, config2)) return false;

    return true;
}

constexpr bool test2() {
    std::string_view json_input = R"({
        "command_id": "SET_MOTOR",
        "timestamp_us": 1234567890,
        "priority": 5,
        "targets": [
            {"device_id": "MOTOR-01"},
            {"device_id": "MOTOR-02"}
        ],
        "params": [
            {"key": "speed", "int_value": 1500},
            {"key": "position", "float_value": 45.5}
        ]
    })";

    // Parse from JSON
    RpcCommand cmd1;
    if(!Parse(cmd1, json_input)) return false;
    if(!verify_rpc_initial_parse(cmd1)) return false;

    // Serialize to CBOR
    std::array<char, 512> cbor_buffer{};
    CborWriter writer(cbor_buffer.data(), cbor_buffer.data() + cbor_buffer.size());
    auto serialize_result = SerializeWithWriter(cmd1, writer);
    if(!serialize_result) return false;

    // Parse from CBOR
    RpcCommand cmd2;
    CborReader reader(cbor_buffer.data(), cbor_buffer.data() + serialize_result.bytesWritten());
    if(!ParseWithReader(cmd2, reader)) return false;
    if(!verify_rpc_roundtrip(cmd1, cmd2)) return false;

    return true;
}

constexpr bool test3() {
    // Invalid: port 100 < 1024 (below minimum)
    std::string_view invalid_json = R"({
        "device_name": "Test",
        "version_major": 1,
        "version_minor": 0,
        "network": {
            "name": "eth0",
            "address": "192.168.1.1",
            "port": 100,
            "enabled": true
        },
        "motors": [
            {"id": 1, "name": "M1", "position": [0, 0, 0], "active": true}
        ]
    })";

    EmbeddedConfig config;
    return !Parse(config, invalid_json);  // Should fail validation
}

constexpr bool test4() {
    // Invalid: position[2] = 2000 > 1000 (above maximum)
    std::string_view invalid_json = R"({
        "device_name": "Test",
        "version_major": 1,
        "version_minor": 0,
        "network": {
            "name": "eth0",
            "address": "192.168.1.1",
            "port": 8080,
            "enabled": true
        },
        "motors": [
            {"id": 1, "name": "M1", "position": [0, 0, 2000], "active": true}
        ]
    })";

    EmbeddedConfig config;
    return !Parse(config, invalid_json);  // Should fail validation
}



// Helper: Compare fixed-size char arrays as null-terminated strings
template<std::size_t N>
constexpr bool str_eq(const std::array<char, N>& arr, const char* str) {
    for (std::size_t i = 0; i < N; ++i) {
        if (arr[i] != str[i]) return false;
        if (arr[i] == '\0') return true;
    }
    return str[N] == '\0';
}

// Verification function implementations
constexpr bool verify_config_initial_parse(const EmbeddedConfig& config) {
    if(!str_eq(config.device_name, "Controller-01")) return false;
    if(config.version_major != 1) return false;
    if(config.version_minor != 2) return false;
    if(!str_eq(config.network.name, "eth0")) return false;
    if(!str_eq(config.network.address, "192.168.1.100")) return false;
    if(config.network.port != 8080) return false;
    if(!config.network.enabled) return false;
    if(!config.fallback_network.has_value()) return false;
    if(!str_eq(config.fallback_network->name, "wlan0")) return false;
    if(config.fallback_network->port != 8081) return false;
    if(config.motors[0].id != 1) return false;
    if(!str_eq(config.motors[0].name, "Motor-A")) return false;
    if(config.motors[0].position[0] != 10.5) return false;
    if(config.motors[1].active) return false;
    return true;
}

constexpr bool verify_config_roundtrip(const EmbeddedConfig& c1, const EmbeddedConfig& c2) {
    if(c2.version_major != c1.version_major) return false;
    if(c2.network.port != c1.network.port) return false;
    if(c2.motors[0].position[2] != c1.motors[0].position[2]) return false;
    return true;
}

constexpr bool verify_rpc_initial_parse(const RpcCommand& cmd) {
    if(!str_eq(cmd.command_id, "SET_MOTOR")) return false;
    if(cmd.timestamp_us != 1234567890) return false;
    if(cmd.priority != 5) return false;
    if(!str_eq(cmd.targets[0].device_id, "MOTOR-01")) return false;
    if(!str_eq(cmd.targets[1].device_id, "MOTOR-02")) return false;
    if(!str_eq(cmd.params[0].key, "speed")) return false;
    if(cmd.params[0].int_value != 1500) return false;
    if(!str_eq(cmd.params[1].key, "position")) return false;
    if(cmd.params[1].float_value != 45.5) return false;
    return true;
}

constexpr bool verify_rpc_roundtrip(const RpcCommand& cmd1, const RpcCommand& cmd2) {
    if(cmd2.timestamp_us != cmd1.timestamp_us) return false;
    if(cmd2.priority != cmd1.priority) return false;
    if(cmd2.params[0].int_value != cmd1.params[0].int_value) return false;
    if(cmd2.params[1].float_value != cmd1.params[1].float_value) return false;
    return true;
}


// Run all tests at compile time
static_assert(test1(), "JSON Roundtrip test failed");
static_assert(test2(), "CBOR Roundtrip test failed");
static_assert(test3(), "Validation test failed");
static_assert(test4(), "Validation test failed");

