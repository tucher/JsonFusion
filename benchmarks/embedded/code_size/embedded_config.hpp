#pragma once

#include <array>
#include <optional>
#include <cstdint>

// Shared embedded config model for both JsonFusion and ArduinoJson benchmarks
// This represents a realistic embedded system configuration

namespace embedded_benchmark {

static constexpr std::size_t kMult = 2;
using SmallStr  = std::array<char, 16 * kMult>;
using MediumStr = std::array<char, 32 * kMult>;
using LargeStr  = std::array<char, 64 * kMult>;

struct EmbeddedConfig {
    static constexpr std::size_t kMaxMotors  = 16;
    static constexpr std::size_t kMaxSensors = 16;

    MediumStr app_name;
    uint16_t  version_major;
    int       version_minor;

    struct Network {
        SmallStr name;
        SmallStr address;  // e.g. "192.168.0.1/24"
        uint16_t port;
        bool     enabled;
    };

    Network           network;
    std::optional<Network> fallback_network_conf;

    struct Controller {
        MediumStr name;
        int       loop_hz;  // Validation: range<10, 10000>

        struct Motor {
            int64_t  id;
            SmallStr name;

            // Position [x,y,z] with validation: each in range<-1000, 1000>
            std::array<double, 3> position;

            // Velocity limits [vx,vy,vz] with validation: each in range<-1000, 1000>
            std::array<float, 3> vel_limits;

            bool inverted;
        };

        std::array<Motor, kMaxMotors> motors;
        size_t motors_count = 0;  // Actual number of motors (validation: min 1)

        struct Sensor {
            SmallStr  type;
            MediumStr model;
            float     range_min;  // Validation: range<-100, 100000>
            double    range_max;  // Validation: range<-1000, 100000>
            bool      active;
        };

        std::array<Sensor, kMaxSensors> sensors;
        size_t sensors_count = 0;  // Actual number of sensors (validation: min 1)
    };

    Controller controller;

    struct Logging {
        bool     enabled;
        LargeStr path;
        uint32_t max_files;
    };

    Logging logging;
};

// Realistic RPC command structure - device receives and parses these from server/gateway
struct RpcCommand {
    static constexpr std::size_t kMaxParams = 8;
    static constexpr std::size_t kMaxTargets = 4;

    // Command identification
    SmallStr  command_id;       // e.g. "CMD_SET_MOTOR", "CMD_READ_SENSOR"
    uint64_t  timestamp_us;     // When command was issued
    uint16_t  sequence;         // Validation: monotonic sequence number
    uint8_t   priority;         // Validation: range<0, 10>
    
    // Target specification
    struct Target {
        SmallStr device_id;     // e.g. "MOTOR_01", "SENSOR_02"
        SmallStr subsystem;     // e.g. "motor", "sensor", "controller"
    };
    
    std::array<Target, kMaxTargets> targets;
    size_t targets_count = 0;   // Validation: min 1, max kMaxTargets
    
    // Command parameters (generic key-value style for flexibility)
    struct Parameter {
        SmallStr  key;          // e.g. "speed", "position", "mode", "threshold"
        
        // Union-like value storage (only one field will be populated)
        std::optional<int64_t>  int_value;
        std::optional<double>   float_value;   // Validation: range<-1e6, 1e6>
        std::optional<bool>     bool_value;
        std::optional<SmallStr> string_value;
    };
    
    std::array<Parameter, kMaxParams> params;
    size_t params_count = 0;    // Validation: min 1, max kMaxParams
    
    // Execution constraints
    struct ExecutionOptions {
        uint32_t timeout_ms;    // Validation: range<0, 300000> (5 minutes max)
        bool     retry_on_failure;
        uint8_t  max_retries;   // Validation: range<0, 5>
    };
    
    std::optional<ExecutionOptions> execution;
    
    // Callback/response configuration
    struct ResponseConfig {
        SmallStr callback_url;  // Where to send command result
        bool     acknowledge;   // Send immediate ack before execution
        bool     send_result;   // Send execution result
    };
    
    std::optional<ResponseConfig> response_config;
};

} // namespace embedded_benchmark

