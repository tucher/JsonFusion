
#include <JsonFusion/parser.hpp>
#include <JsonFusion/validators.hpp>
#include <string_view>

using namespace JsonFusion;
using namespace JsonFusion::options;
using std::array;
using std::optional;

// Model matching typical embedded config needs:
// - Fixed-size arrays (static allocation)
// - Validation constraints
// - Nested structures
// - Optional fields

namespace {
using namespace JsonFusion::validators;
static constexpr std::size_t kMult = 2;
using SmallStr  = array<char, 16 * kMult>;
using MediumStr = array<char, 32 * kMult>;
using LargeStr  = array<char, 64 * kMult>;

using FPLike32 = float;
using FPLike64 = double;

// using FPLike32 = int32_t;
// using FPLike64 = int64_t;

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
    optional<Network> fallback_network_conf;

    struct Controller {
        MediumStr name;
        A<int, range<10, 10000>> loop_hz;

        struct Motor {
            int64_t  id;
            SmallStr name;

            // Position [x,y,z] with validation
            A<array<A<FPLike64, range<-1000, 1000>>, 3>, min_items<3>> position;

            // Velocity limits [vx,vy,vz] with validation
            A<array<A<FPLike32, range<-1000, 1000>>, 3>, min_items<3>> vel_limits;

            bool inverted;
        };

        A<array<Motor, kMaxMotors>, min_items<1>> motors;

        struct Sensor {
            SmallStr  type;
            MediumStr model;
            A<FPLike32,  range<-100,  100000>> range_min;
            A<FPLike64, range<-1000, 100000>> range_max;
            bool      active;
        };

        A<array<Sensor, kMaxSensors>, min_items<1>> sensors;
    };

    Controller controller;

    struct Logging {
        bool     enabled;
        LargeStr path;
        uint32_t max_files;
    };

    Logging logging;
};

// RPC Command structure with validation and required/optional field specifications
struct RpcCommand_ {
    static constexpr std::size_t kMaxParams = 8;
    static constexpr std::size_t kMaxTargets = 4;

    // Command identification
    SmallStr  command_id;       // Required
    uint64_t  timestamp_us;     // Required
    uint16_t  sequence;         // Optional - server can auto-assign
    A<uint8_t, range<0, 10>> priority;  // Optional - has default
    
    // Target specification
    struct Target_ {
        SmallStr device_id;     // Required
        SmallStr subsystem;     // Optional - defaults to whole device
    };
    using Target = A<Target_, required<"device_id">>;
    
    A<array<Target, kMaxTargets>, min_items<1>> targets;  // Required
    
    // Command parameters
    struct Parameter_ {
        SmallStr key;           // Required
        
        // Union-like value storage (all optional by default)
        optional<int64_t> int_value;
        A<optional<FPLike64>, range<-1000000, 1000000>> float_value;
        optional<bool> bool_value;
        optional<SmallStr> string_value;
    };
    using Parameter = A<Parameter_, required<"key">>;
    
    A<array<Parameter, kMaxParams>, min_items<1>> params;  // Required
    
    // Execution constraints (entire section optional)
    struct ExecutionOptions {
        A<uint32_t, range<0, 300000>> timeout_ms;  // Required if section present
        bool     retry_on_failure;                  // Optional - defaults to false
        A<uint8_t, range<0, 5>> max_retries;       // Optional - defaults to 0
    };
    
    A<optional<ExecutionOptions>, required<"timeout_ms">> execution;  // Optional section
    
    // Callback/response configuration (entire section optional)
    struct ResponseConfig {
        SmallStr callback_url;  // Optional - can use default endpoint
        bool     acknowledge;   // Required if section present
        bool     send_result;   // Required if section present
    };
    
    A<optional<ResponseConfig>, required<"acknowledge", "send_result">> response_config;  // Optional section
};

using RpcCommand = A<RpcCommand_, required<"command_id", "timestamp_us", "targets", "params">>;

}  // namespace

// Global config instance (will go in .bss section)
EmbeddedConfig g_config;

// Parse function - instantiates JsonFusion parser for this model
// This is what gets measured for code size
extern "C" __attribute__((used)) bool parse_config(const char* data, size_t size) {
    auto result = JsonFusion::Parse(g_config, std::string_view(data, size));
    return !!result;
}

extern "C" __attribute__((used)) bool parse_rpc_command(const char* data, size_t size) {
    RpcCommand cmd;
    auto result = JsonFusion::Parse(cmd, std::string_view(data, size));
    return !!result;
}
// Entry point - ensures parse_config is not eliminated by linker
// In a real embedded system, this would be your main loop
int main() {
    // Call parse_config to ensure it's included in binary
    volatile bool result = parse_config("", 0);
    volatile bool rpc_result = parse_rpc_command("", 0);
    (void)result;
    (void)rpc_result;
    
    // Infinite loop (typical for embedded without OS)
    while(1) {}
    return 0;
}

