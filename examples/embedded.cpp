// Embedded Systems Example: JSON/CBOR Roundtrip Verification
//
// Demonstrates:
// - Fixed-size models (std::array / std::optional) — no dynamic containers in the schema
// - Validation constraints (range, min_items/max_items, required fields)
// - JSON roundtrip   (parse -> serialize -> parse)
// - JSON -> CBOR -> model roundtrip (parse JSON -> serialize CBOR -> parse CBOR)
//
// Notes:
// - JSON inputs are string literals for readability; real embedded input is typically a byte stream.
// - Output buffers are fixed-size and explicitly NUL-terminated for convenience.
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

// ============================================================================
// Models
// ============================================================================

struct EmbeddedConfig_ {
    std::array<char, 32> device_name{};
    std::uint16_t version_major{};
    std::uint16_t version_minor{};

    struct Network {
        std::array<char, 16> name{};
        std::array<char, 24> address{};
        A<std::uint16_t, range<1024, 65535>> port{2000};
        bool enabled{false};
    };

    Network network{};
    std::optional<Network> fallback_network{};

    struct Motor {
        std::uint8_t id{0};
        std::array<char, 16> name{};
        // Fixed-size position vector: [x,y,z], each component range-validated.
        std::array<A<double, range<-1000, 1000>>, 3> position{};
        bool active{false};
    };

    // Fixed-size motor table, but still show min/max constraints API.
    A<std::array<Motor, 4>, min_items<1>, max_items<4>> motors{};
};

// "Required" fields validator 
// - device_name must be present
// - network must be present
using EmbeddedConfig = A<EmbeddedConfig_, required<"device_name", "network">>;

struct RpcCommand_ {
    std::array<char, 16> command_id{};
    std::uint64_t timestamp_us{0};
    A<std::uint8_t, range<0, 10>> priority{0};

    struct Target {
        std::array<char, 16> device_id{};
    };
    A<std::array<Target, 4>, min_items<1>, max_items<4>> targets{};

    struct Parameter {
        std::array<char, 16> key{};
        std::optional<std::int64_t> int_value{};
        // Optional float with range constraint when present.
        A<std::optional<double>, range<-1000000, 1000000>> float_value{};
    };
    A<std::array<Parameter, 8>, min_items<1>, max_items<8>> params{};
};

// At least command_id must be present.
using RpcCommand = A<RpcCommand_, required<"command_id">>;

// ============================================================================
// Forward declarations (keep checks below the demo)
// ============================================================================

constexpr bool verify_config_initial_parse(const EmbeddedConfig_& config);
constexpr bool verify_config_roundtrip(const EmbeddedConfig_& c1, const EmbeddedConfig_& c2);
constexpr bool verify_rpc_initial_parse(const RpcCommand_& cmd);
constexpr bool verify_rpc_roundtrip(const RpcCommand_& cmd1, const RpcCommand_& cmd2);

template<std::size_t N>
constexpr bool str_eq(const std::array<char, N>& arr, const char* s);

// ============================================================================
// Demo / tests
// ============================================================================

constexpr bool test_json_roundtrip_config() {
    // Use a literal array for maximal constexpr portability.
    constexpr const char json_input[] = R"({
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
    EmbeddedConfig config1{};
    if (!Parse(config1, json_input)) return false;
    if (!verify_config_initial_parse(config1)) return false;

    // Serialize back to JSON (leave 1 byte for '\0')
    std::array<char, 512> json_output{};
    auto res = Serialize(config1, json_output.data(), json_output.data() + (json_output.size() - 1));
    if (!res) return false;

    const std::size_t n = res.bytesWritten();
    if (n >= json_output.size()) return false; // paranoia guard
    json_output[n] = '\0';

    // Parse serialized JSON (roundtrip verification)
    EmbeddedConfig config2{};
    if (!Parse(config2, json_output.data())) return false;
    if (!verify_config_roundtrip(config1, config2)) return false;

    return true;
}

constexpr bool test_json_to_cbor_roundtrip_rpc() {
    constexpr const char json_input[] = R"({
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
    RpcCommand cmd1{};
    if (!Parse(cmd1, json_input)) return false;
    if (!verify_rpc_initial_parse(cmd1)) return false;

    // Serialize to CBOR (fixed buffer)
    std::array<std::uint8_t, 512> cbor_buffer{};
    CborWriter writer(cbor_buffer.data(), cbor_buffer.data() + cbor_buffer.size());
    auto serialize_result = SerializeWithWriter(cmd1, writer);
    if (!serialize_result) return false;

    // Parse from CBOR
    RpcCommand cmd2{};
    CborReader reader(cbor_buffer.data(), cbor_buffer.data() + serialize_result.bytesWritten());
    if (!ParseWithReader(cmd2, reader)) return false;

    if (!verify_rpc_roundtrip(cmd1, cmd2)) return false;
    return true;
}

constexpr bool test_validation_fail_port_range() {
    // Invalid: port 100 < 1024 (below minimum)
    constexpr const char invalid_json[] = R"({
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

    EmbeddedConfig config{};
    return !Parse(config, invalid_json);
}

constexpr bool test_validation_fail_motor_position_range() {
    // Invalid: position[2] = 2000 > 1000 (above maximum)
    constexpr const char invalid_json[] = R"({
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

    EmbeddedConfig config{};
    return !Parse(config, invalid_json);
}

// ============================================================================
// Checks / helpers (kept below the main demo)
// ============================================================================

// Compare fixed-size char arrays against a NUL-terminated string.
// Never reads past the end of `s`.
template<std::size_t N>
constexpr bool str_eq(const std::array<char, N>& arr, const char* s) {
    for (std::size_t i = 0;; ++i) {
        if (i == N) {
            return s[i] == '\0';
        }
        if (arr[i] != s[i]) return false;
        if (s[i] == '\0') return true;
    }
}

constexpr bool verify_config_initial_parse(const EmbeddedConfig_& config) {
    if (!str_eq(config.device_name, "Controller-01")) return false;

    if (config.version_major != 1) return false;
    if (config.version_minor != 2) return false;

    if (!str_eq(config.network.name, "eth0")) return false;
    if (!str_eq(config.network.address, "192.168.1.100")) return false;
    if (config.network.port != 8080) return false;
    if (!config.network.enabled) return false;

    if (!config.fallback_network.has_value()) return false;
    if (!str_eq(config.fallback_network->name, "wlan0")) return false;
    if (config.fallback_network->port != 8081) return false;

    if (config.motors[0].id != 1) return false;
    if (!str_eq(config.motors[0].name, "Motor-A")) return false;
    if (config.motors[0].position[0] != 10.5) return false;

    if (config.motors[1].active) return false;

    return true;
}

constexpr bool verify_config_roundtrip(const EmbeddedConfig_& c1, const EmbeddedConfig_& c2) {
    if (c2.version_major != c1.version_major) return false;
    if (c2.version_minor != c1.version_minor) return false;
    if (c2.network.port != c1.network.port) return false;
    if (c2.motors[0].position[2] != c1.motors[0].position[2]) return false;
    return true;
}

constexpr bool verify_rpc_initial_parse(const RpcCommand_& cmd) {
    if (!str_eq(cmd.command_id, "SET_MOTOR")) return false;

    if (cmd.timestamp_us != 1234567890ULL) return false;
    if (cmd.priority != 5) return false;

    if (!str_eq(cmd.targets[0].device_id, "MOTOR-01")) return false;
    if (!str_eq(cmd.targets[1].device_id, "MOTOR-02")) return false;

    if (!str_eq(cmd.params[0].key, "speed")) return false;
    if (!cmd.params[0].int_value.has_value() || *cmd.params[0].int_value != 1500) return false;

    if (!str_eq(cmd.params[1].key, "position")) return false;
    if (!cmd.params[1].float_value->has_value() || cmd.params[1].float_value != 45.5) return false;

    return true;
}

constexpr bool verify_rpc_roundtrip(const RpcCommand_& cmd1, const RpcCommand_& cmd2) {
    if (cmd2.timestamp_us != cmd1.timestamp_us) return false;
    if (cmd2.priority != cmd1.priority) return false;
    if (cmd2.params[0].int_value != cmd1.params[0].int_value) return false;
    if (cmd2.params[1].float_value != cmd1.params[1].float_value) return false;
    return true;
}

// Run all tests at compile time
static_assert(test_json_roundtrip_config(), "JSON roundtrip test failed");
static_assert(test_json_to_cbor_roundtrip_rpc(), "JSON->CBOR->parse roundtrip test failed");
static_assert(test_validation_fail_port_range(), "Expected port-range validation failure");
static_assert(test_validation_fail_motor_position_range(), "Expected motor position validation failure");
