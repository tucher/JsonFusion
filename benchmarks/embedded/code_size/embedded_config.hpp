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

} // namespace embedded_benchmark

