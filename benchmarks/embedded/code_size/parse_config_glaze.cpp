#define GLZ_DISABLE_ALWAYS_INLINE
#include <string_view>
// #include <glaze/json.hpp>
#include <cstdlib>
#include <glaze/json/read.hpp>

#include "embedded_config.hpp"

// #define JSON_FUSION_BENCHMARK_ADDITIONAL_MODELS
#ifdef JSON_FUSION_BENCHMARK_ADDITIONAL_MODELS
#include "additional_models.hpp"
#endif


// Global config instance (will go in .bss section)
embedded_benchmark::EmbeddedConfig g_config;

extern "C" __attribute__((used)) bool parse_config(const char* data, size_t size) {
    auto error = glz::read<glz::opts_size{}>(g_config, std::string_view(data, size));
    return !error;
}

extern "C" __attribute__((used)) bool parse_rpc_command(const char* data, size_t size) {
    
    embedded_benchmark::RpcCommand cmd;

    auto error = glz::read<glz::opts_size{}>(cmd, std::string_view(data, size));
    return !error;
}

#ifdef JSON_FUSION_BENCHMARK_ADDITIONAL_MODELS
// Single unified parsing function for all additional models
extern "C" __attribute__((used)) bool parse_additional_model(int model_id, const char* data, size_t size) {
    std::string_view json(data, size);
    
    switch(model_id) {
        case 1: {
            code_size_test::DeviceMetadata model;
            auto error = glz::read<glz::opts_size{}>(model, json);
            return !error;
        }
        case 2: {
            code_size_test::SensorReadings model;
            auto error = glz::read<glz::opts_size{}>(model, json);
            return !error;
        }
        case 3: {
            code_size_test::SystemStats model;
            auto error = glz::read<glz::opts_size{}>(model, json);
            return !error;
        }
        case 4: {
            code_size_test::NetworkPacket model;
            auto error = glz::read<glz::opts_size{}>(model, json);
            return !error;
        }
        case 5: {
            code_size_test::ImageDescriptor model;
            auto error = glz::read<glz::opts_size{}>(model, json);
            return !error;
        }
        case 6: {
            code_size_test::AudioConfig model;
            auto error = glz::read<glz::opts_size{}>(model, json);
            return !error;
        }
        case 7: {
            code_size_test::CacheEntry model;
            auto error = glz::read<glz::opts_size{}>(model, json);
            return !error;
        }
        case 8: {
            code_size_test::FileMetadata model;
            auto error = glz::read<glz::opts_size{}>(model, json);
            return !error;
        }
        case 9: {
            code_size_test::TransactionRecord model;
            auto error = glz::read<glz::opts_size{}>(model, json);
            return !error;
        }
        case 10: {
            code_size_test::TelemetryPacket model;
            auto error = glz::read<glz::opts_size{}>(model, json);
            return !error;
        }
        case 11: {
            code_size_test::RobotCommand model;
            auto error = glz::read<glz::opts_size{}>(model, json);
            return !error;
        }
        case 12: {
            code_size_test::WeatherData model;
            auto error = glz::read<glz::opts_size{}>(model, json);
            return !error;
        }
        case 13: {
            code_size_test::DatabaseQuery model;
            auto error = glz::read<glz::opts_size{}>(model, json);
            return !error;
        }
        case 14: {
            code_size_test::VideoStream model;
            auto error = glz::read<glz::opts_size{}>(model, json);
            return !error;
        }
        case 15: {
            code_size_test::EncryptionContext model;
            auto error = glz::read<glz::opts_size{}>(model, json);
            return !error;
        }
        case 16: {
            code_size_test::MeshNode model;
            auto error = glz::read<glz::opts_size{}>(model, json);
            return !error;
        }
        case 17: {
            code_size_test::GameState model;
            auto error = glz::read<glz::opts_size{}>(model, json);
            return !error;
        }
        case 18: {
            code_size_test::LogEntry model;
            auto error = glz::read<glz::opts_size{}>(model, json);
            return !error;
        }
        case 19: {
            code_size_test::CalendarEvent model;
            auto error = glz::read<glz::opts_size{}>(model, json);
            return !error;
        }
        case 20: {
            code_size_test::HardwareProfile model;
            auto error = glz::read<glz::opts_size{}>(model, json);
            return !error;
        }
        default:
            return false;
    }
}
#endif

// Entry point - ensures parse_config is not eliminated by linker
// In a real embedded system, this would be your main loop
int main() {
    // Call parse_config to ensure it's included in binary
    volatile bool result = parse_config("", 0);
    volatile bool rpc_result = parse_rpc_command("", 0);
    (void)result;
    (void)rpc_result;
#ifdef JSON_FUSION_BENCHMARK_ADDITIONAL_MODELS
    // Call additional model parser for all 20 models to ensure they're included in binary
    for (int i = 1; i <= 20; i++) {
        volatile bool r = parse_additional_model(i, "", 0);
        (void)r;
    }
#endif
    // Infinite loop (typical for embedded without OS)
    while(1) {}
    return 0;
}

