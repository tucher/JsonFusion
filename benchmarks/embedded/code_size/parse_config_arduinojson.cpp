// ArduinoJson benchmark - manual parsing + validation
// Implements the SAME validation logic as JsonFusion for fair comparison

#include <ArduinoJson.h>
#include <string_view>
#include "embedded_config.hpp"

using embedded_benchmark::EmbeddedConfig;

// Global config instance
EmbeddedConfig g_config;

// Helper: Copy JSON string to fixed-size array
template<size_t N>
static bool copy_string(JsonVariant src, std::array<char, N>& dst) {
    const char* str = src.as<const char*>();
    if (!str) return false;
    
    size_t len = strlen(str);
    if (len >= N) len = N - 1;
    
    std::copy(str, str + len, dst.begin());
    dst[len] = '\0';
    return true;
}

// Helper: Validate range (for integers)
template<typename T, int64_t Min, int64_t Max>
static bool validate_range_int(T value) {
    return value >= static_cast<T>(Min) && value <= static_cast<T>(Max);
}

// Helper: Validate floating-point range
static bool validate_double_range(double value, double min, double max) {
    return value >= min && value <= max;
}

static bool validate_float_range(float value, float min, float max) {
    return value >= min && value <= max;
}

// Parse and validate Motor
static bool parse_motor(JsonObject motor_obj, EmbeddedConfig::Controller::Motor& motor) {
    // Required fields
    if (!motor_obj["id"].is<int64_t>()) return false;
    motor.id = motor_obj["id"];
    
    if (!copy_string(motor_obj["name"], motor.name)) return false;
    
    // Position: must be array of 3 doubles, each in range [-1000, 1000]
    JsonArray pos_arr = motor_obj["position"];
    if (!pos_arr || pos_arr.size() < 3) return false;  // min_items<3>
    
    for (size_t i = 0; i < 3; ++i) {
        if (!pos_arr[i].is<double>()) return false;
        double val = pos_arr[i];
        if (!validate_double_range(val, -1000.0, 1000.0)) return false;  // range validation!
        motor.position[i] = val;
    }
    
    // Velocity limits: array of 3 floats, each in range [-1000, 1000]
    JsonArray vel_arr = motor_obj["vel_limits"];
    if (!vel_arr || vel_arr.size() < 3) return false;  // min_items<3>
    
    for (size_t i = 0; i < 3; ++i) {
        if (!vel_arr[i].is<float>()) return false;
        float val = vel_arr[i];
        if (!validate_float_range(val, -1000.0f, 1000.0f)) return false;  // range validation!
        motor.vel_limits[i] = val;
    }
    
    if (!motor_obj["inverted"].is<bool>()) return false;
    motor.inverted = motor_obj["inverted"];
    
    return true;
}

// Parse and validate Sensor
static bool parse_sensor(JsonObject sensor_obj, EmbeddedConfig::Controller::Sensor& sensor) {
    if (!copy_string(sensor_obj["type"], sensor.type)) return false;
    if (!copy_string(sensor_obj["model"], sensor.model)) return false;
    
    // range_min: float in range [-100, 100000]
    if (!sensor_obj["range_min"].is<float>()) return false;
    float range_min = sensor_obj["range_min"];
    if (!validate_float_range(range_min, -100.0f, 100000.0f)) return false;  // range validation!
    sensor.range_min = range_min;
    
    // range_max: double in range [-1000, 100000]
    if (!sensor_obj["range_max"].is<double>()) return false;
    double range_max = sensor_obj["range_max"];
    if (!validate_double_range(range_max, -1000.0, 100000.0)) return false;  // range validation!
    sensor.range_max = range_max;
    
    if (!sensor_obj["active"].is<bool>()) return false;
    sensor.active = sensor_obj["active"];
    
    return true;
}

// Parse and validate Network
static bool parse_network(JsonObject net_obj, EmbeddedConfig::Network& network) {
    if (!copy_string(net_obj["name"], network.name)) return false;
    if (!copy_string(net_obj["address"], network.address)) return false;
    if (!net_obj["port"].is<uint16_t>()) return false;
    network.port = net_obj["port"];
    if (!net_obj["enabled"].is<bool>()) return false;
    network.enabled = net_obj["enabled"];
    return true;
}

// Main parse function with full validation
extern "C" __attribute__((used)) bool parse_config_arduinojson(const char* data, size_t size) {
    // Allocate JSON document (static to avoid stack overflow)
    static JsonDocument doc;
    
    // Parse JSON
    DeserializationError error = deserializeJson(doc, data, size);
    if (error) return false;
    
    JsonObject root = doc.as<JsonObject>();
    
    // Parse app_name, version_major, version_minor
    if (!copy_string(root["app_name"], g_config.app_name)) return false;
    if (!root["version_major"].is<uint16_t>()) return false;
    g_config.version_major = root["version_major"];
    if (!root["version_minor"].is<int>()) return false;
    g_config.version_minor = root["version_minor"];
    
    // Parse network (required)
    JsonObject net_obj = root["network"];
    if (!net_obj) return false;
    if (!parse_network(net_obj, g_config.network)) return false;
    
    // Parse fallback_network_conf (optional)
    if (root["fallback_network_conf"].is<JsonObject>()) {
        EmbeddedConfig::Network fallback;
        if (parse_network(root["fallback_network_conf"], fallback)) {
            g_config.fallback_network_conf = fallback;
        }
    }
    
    // Parse controller
    JsonObject ctrl_obj = root["controller"];
    if (!ctrl_obj) return false;
    
    if (!copy_string(ctrl_obj["name"], g_config.controller.name)) return false;
    
    // loop_hz: validate range [10, 10000]
    if (!ctrl_obj["loop_hz"].is<int>()) return false;
    int loop_hz = ctrl_obj["loop_hz"];
    if (!validate_range_int<int, 10, 10000>(loop_hz)) return false;  // range validation!
    g_config.controller.loop_hz = loop_hz;
    
    // Parse motors array (min_items<1>)
    JsonArray motors_arr = ctrl_obj["motors"];
    if (!motors_arr || motors_arr.size() == 0) return false;  // min_items validation!
    if (motors_arr.size() > EmbeddedConfig::kMaxMotors) return false;
    
    g_config.controller.motors_count = 0;
    for (JsonObject motor_obj : motors_arr) {
        if (g_config.controller.motors_count >= EmbeddedConfig::kMaxMotors) break;
        if (!parse_motor(motor_obj, g_config.controller.motors[g_config.controller.motors_count])) {
            return false;
        }
        g_config.controller.motors_count++;
    }
    
    // Parse sensors array (min_items<1>)
    JsonArray sensors_arr = ctrl_obj["sensors"];
    if (!sensors_arr || sensors_arr.size() == 0) return false;  // min_items validation!
    if (sensors_arr.size() > EmbeddedConfig::kMaxSensors) return false;
    
    g_config.controller.sensors_count = 0;
    for (JsonObject sensor_obj : sensors_arr) {
        if (g_config.controller.sensors_count >= EmbeddedConfig::kMaxSensors) break;
        if (!parse_sensor(sensor_obj, g_config.controller.sensors[g_config.controller.sensors_count])) {
            return false;
        }
        g_config.controller.sensors_count++;
    }
    
    // Parse logging
    JsonObject log_obj = root["logging"];
    if (!log_obj) return false;
    
    if (!log_obj["enabled"].is<bool>()) return false;
    g_config.logging.enabled = log_obj["enabled"];
    if (!copy_string(log_obj["path"], g_config.logging.path)) return false;
    if (!log_obj["max_files"].is<uint32_t>()) return false;
    g_config.logging.max_files = log_obj["max_files"];
    
    return true;
}

// Entry point
int main() {
    volatile bool result = parse_config_arduinojson("", 0);
    (void)result;
    while(1) {}
    return 0;
}

