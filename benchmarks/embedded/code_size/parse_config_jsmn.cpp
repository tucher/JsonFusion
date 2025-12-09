// jsmn benchmark implementation
// jsmn is a minimalist JSON tokenizer - you manually traverse tokens and parse values

#define JSMN_STATIC
#define JSMN_STRICT
#include "jsmn.h"
#include "embedded_config.hpp"
#include <cstring>
#include <cstdlib>

// Global config instance
embedded_benchmark::EmbeddedConfig g_config_jsmn;

// Maximum tokens for our JSON (adjust as needed)
constexpr size_t MAX_TOKENS = 2048;

// Token array (stack allocated)
static jsmntok_t tokens[MAX_TOKENS];

// Helper: compare token with string key
inline bool tok_eq(const char* json, const jsmntok_t* tok, const char* key) {
    size_t len = tok->end - tok->start;
    return strlen(key) == len && memcmp(json + tok->start, key, len) == 0;
}

// Helper: copy string from token to fixed buffer with null termination
inline bool tok_copy_str(const char* json, const jsmntok_t* tok, char* dest, size_t dest_size) {
    size_t len = tok->end - tok->start;
    if (len >= dest_size) return false; // Too long
    memcpy(dest, json + tok->start, len);
    dest[len] = '\0';
    return true;
}

// Helper: parse integer from token
inline bool tok_parse_int(const char* json, const jsmntok_t* tok, int& out) {
    char buf[32];
    size_t len = tok->end - tok->start;
    if (len >= sizeof(buf)) return false;
    memcpy(buf, json + tok->start, len);
    buf[len] = '\0';
    out = atoi(buf);
    return true;
}

// Helper: parse int64_t from token
inline bool tok_parse_int64(const char* json, const jsmntok_t* tok, int64_t& out) {
    char buf[32];
    size_t len = tok->end - tok->start;
    if (len >= sizeof(buf)) return false;
    memcpy(buf, json + tok->start, len);
    buf[len] = '\0';
    out = atoll(buf);
    return true;
}

// Helper: parse uint16_t from token
inline bool tok_parse_uint16(const char* json, const jsmntok_t* tok, uint16_t& out) {
    int val;
    if (!tok_parse_int(json, tok, val)) return false;
    if (val < 0 || val > 65535) return false;
    out = static_cast<uint16_t>(val);
    return true;
}

// Helper: parse uint32_t from token
inline bool tok_parse_uint32(const char* json, const jsmntok_t* tok, uint32_t& out) {
    char buf[32];
    size_t len = tok->end - tok->start;
    if (len >= sizeof(buf)) return false;
    memcpy(buf, json + tok->start, len);
    buf[len] = '\0';
    out = static_cast<uint32_t>(strtoul(buf, nullptr, 10));
    return true;
}

// Helper: parse float from token
inline bool tok_parse_float(const char* json, const jsmntok_t* tok, float& out) {
    char buf[32];
    size_t len = tok->end - tok->start;
    if (len >= sizeof(buf)) return false;
    memcpy(buf, json + tok->start, len);
    buf[len] = '\0';
    out = strtof(buf, nullptr);
    return true;
}

// Helper: parse double from token
inline bool tok_parse_double(const char* json, const jsmntok_t* tok, double& out) {
    char buf[32];
    size_t len = tok->end - tok->start;
    if (len >= sizeof(buf)) return false;
    memcpy(buf, json + tok->start, len);
    buf[len] = '\0';
    out = strtod(buf, nullptr);
    return true;
}

// Helper: parse bool from token
inline bool tok_parse_bool(const char* json, const jsmntok_t* tok, bool& out) {
    if (tok_eq(json, tok, "true")) {
        out = true;
        return true;
    } else if (tok_eq(json, tok, "false")) {
        out = false;
        return true;
    }
    return false;
}

// Forward declarations for parsing nested structures
bool parse_network(const char* json, const jsmntok_t* tokens, int& idx, embedded_benchmark::EmbeddedConfig::Network& net);
bool parse_motor(const char* json, const jsmntok_t* tokens, int& idx, embedded_benchmark::EmbeddedConfig::Controller::Motor& motor);
bool parse_sensor(const char* json, const jsmntok_t* tokens, int& idx, embedded_benchmark::EmbeddedConfig::Controller::Sensor& sensor);
bool parse_controller(const char* json, const jsmntok_t* tokens, int& idx, embedded_benchmark::EmbeddedConfig::Controller& ctrl);
bool parse_logging(const char* json, const jsmntok_t* tokens, int& idx, embedded_benchmark::EmbeddedConfig::Logging& log);

// Parse Network object
bool parse_network(const char* json, const jsmntok_t* tokens, int& idx, embedded_benchmark::EmbeddedConfig::Network& net) {
    const jsmntok_t* obj = &tokens[idx++];
    if (obj->type != JSMN_OBJECT) return false;
    
    int fields = obj->size; // number of key-value pairs
    for (int i = 0; i < fields; ++i) {
        const jsmntok_t* key = &tokens[idx++];
        const jsmntok_t* val = &tokens[idx++];
        
        if (tok_eq(json, key, "name")) {
            if (!tok_copy_str(json, val, net.name.data(), net.name.size())) return false;
        } else if (tok_eq(json, key, "address")) {
            if (!tok_copy_str(json, val, net.address.data(), net.address.size())) return false;
        } else if (tok_eq(json, key, "port")) {
            if (!tok_parse_uint16(json, val, net.port)) return false;
        } else if (tok_eq(json, key, "enabled")) {
            if (!tok_parse_bool(json, val, net.enabled)) return false;
        } else {
            // Skip unknown field
            if (val->type == JSMN_OBJECT || val->type == JSMN_ARRAY) {
                // Would need recursive skip logic
                return false;
            }
        }
    }
    return true;
}

// Parse Motor object
bool parse_motor(const char* json, const jsmntok_t* tokens, int& idx, embedded_benchmark::EmbeddedConfig::Controller::Motor& motor) {
    const jsmntok_t* obj = &tokens[idx++];
    if (obj->type != JSMN_OBJECT) return false;
    
    int fields = obj->size;
    for (int i = 0; i < fields; ++i) {
        const jsmntok_t* key = &tokens[idx++];
        
        if (tok_eq(json, key, "id")) {
            const jsmntok_t* val = &tokens[idx++];
            if (!tok_parse_int64(json, val, motor.id)) return false;
        } else if (tok_eq(json, key, "name")) {
            const jsmntok_t* val = &tokens[idx++];
            if (!tok_copy_str(json, val, motor.name.data(), motor.name.size())) return false;
        } else if (tok_eq(json, key, "position")) {
            const jsmntok_t* arr = &tokens[idx++];
            if (arr->type != JSMN_ARRAY) return false;
            if (arr->size < 3) return false; // min_items<3> validation
            if (arr->size > 3) return false; // Fixed size array
            for (int j = 0; j < 3; ++j) {
                const jsmntok_t* val = &tokens[idx++];
                if (!tok_parse_double(json, val, motor.position[j])) return false;
                // range<-1000, 1000> validation
                if (motor.position[j] < -1000.0 || motor.position[j] > 1000.0) return false;
            }
        } else if (tok_eq(json, key, "vel_limits")) {
            const jsmntok_t* arr = &tokens[idx++];
            if (arr->type != JSMN_ARRAY) return false;
            if (arr->size < 3) return false; // min_items<3> validation
            if (arr->size > 3) return false; // Fixed size array
            for (int j = 0; j < 3; ++j) {
                const jsmntok_t* val = &tokens[idx++];
                if (!tok_parse_float(json, val, motor.vel_limits[j])) return false;
                // range<-1000, 1000> validation
                if (motor.vel_limits[j] < -1000.0f || motor.vel_limits[j] > 1000.0f) return false;
            }
        } else if (tok_eq(json, key, "inverted")) {
            const jsmntok_t* val = &tokens[idx++];
            if (!tok_parse_bool(json, val, motor.inverted)) return false;
        } else {
            idx++; // Skip unknown field
        }
    }
    return true;
}

// Parse Sensor object
bool parse_sensor(const char* json, const jsmntok_t* tokens, int& idx, embedded_benchmark::EmbeddedConfig::Controller::Sensor& sensor) {
    const jsmntok_t* obj = &tokens[idx++];
    if (obj->type != JSMN_OBJECT) return false;
    
    int fields = obj->size;
    for (int i = 0; i < fields; ++i) {
        const jsmntok_t* key = &tokens[idx++];
        const jsmntok_t* val = &tokens[idx++];
        
        if (tok_eq(json, key, "type")) {
            if (!tok_copy_str(json, val, sensor.type.data(), sensor.type.size())) return false;
        } else if (tok_eq(json, key, "model")) {
            if (!tok_copy_str(json, val, sensor.model.data(), sensor.model.size())) return false;
        } else if (tok_eq(json, key, "range_min")) {
            if (!tok_parse_float(json, val, sensor.range_min)) return false;
            // range<-100, 100000> validation
            if (sensor.range_min < -100.0f || sensor.range_min > 100000.0f) return false;
        } else if (tok_eq(json, key, "range_max")) {
            if (!tok_parse_double(json, val, sensor.range_max)) return false;
            // range<-1000, 100000> validation
            if (sensor.range_max < -1000.0 || sensor.range_max > 100000.0) return false;
        } else if (tok_eq(json, key, "active")) {
            if (!tok_parse_bool(json, val, sensor.active)) return false;
        }
    }
    return true;
}

// Parse Controller object
bool parse_controller(const char* json, const jsmntok_t* tokens, int& idx, embedded_benchmark::EmbeddedConfig::Controller& ctrl) {
    const jsmntok_t* obj = &tokens[idx++];
    if (obj->type != JSMN_OBJECT) return false;
    
    int fields = obj->size;
    for (int i = 0; i < fields; ++i) {
        const jsmntok_t* key = &tokens[idx++];
        
        if (tok_eq(json, key, "name")) {
            const jsmntok_t* val = &tokens[idx++];
            if (!tok_copy_str(json, val, ctrl.name.data(), ctrl.name.size())) return false;
        } else if (tok_eq(json, key, "loop_hz")) {
            const jsmntok_t* val = &tokens[idx++];
            if (!tok_parse_int(json, val, ctrl.loop_hz)) return false;
            // range<10, 10000> validation
            if (ctrl.loop_hz < 10 || ctrl.loop_hz > 10000) return false;
        } else if (tok_eq(json, key, "motors")) {
            const jsmntok_t* arr = &tokens[idx++];
            if (arr->type != JSMN_ARRAY) return false;
            if (arr->size < 1) return false; // min_items<1> validation
            if (arr->size > embedded_benchmark::EmbeddedConfig::kMaxMotors) return false; // max_items validation
            
            for (int j = 0; j < arr->size; ++j) {
                if (!parse_motor(json, tokens, idx, ctrl.motors[j])) return false;
            }
        } else if (tok_eq(json, key, "sensors")) {
            const jsmntok_t* arr = &tokens[idx++];
            if (arr->type != JSMN_ARRAY) return false;
            if (arr->size < 1) return false; // min_items<1> validation
            if (arr->size > embedded_benchmark::EmbeddedConfig::kMaxSensors) return false; // max_items validation
            
            for (int j = 0; j < arr->size; ++j) {
                if (!parse_sensor(json, tokens, idx, ctrl.sensors[j])) return false;
            }
        } else {
            idx++; // Skip unknown field
        }
    }
    return true;
}

// Parse Logging object
bool parse_logging(const char* json, const jsmntok_t* tokens, int& idx, embedded_benchmark::EmbeddedConfig::Logging& log) {
    const jsmntok_t* obj = &tokens[idx++];
    if (obj->type != JSMN_OBJECT) return false;
    
    int fields = obj->size;
    for (int i = 0; i < fields; ++i) {
        const jsmntok_t* key = &tokens[idx++];
        const jsmntok_t* val = &tokens[idx++];
        
        if (tok_eq(json, key, "enabled")) {
            if (!tok_parse_bool(json, val, log.enabled)) return false;
        } else if (tok_eq(json, key, "path")) {
            if (!tok_copy_str(json, val, log.path.data(), log.path.size())) return false;
        } else if (tok_eq(json, key, "max_files")) {
            if (!tok_parse_uint32(json, val, log.max_files)) return false;
        }
    }
    return true;
}

// Main parsing function
extern "C" __attribute__((used)) bool parse_config_jsmn(const char* data, size_t size) {
    // Initialize parser
    jsmn_parser parser;
    jsmn_init(&parser);
    
    // Parse JSON into tokens
    int num_tokens = jsmn_parse(&parser, data, size, tokens, MAX_TOKENS);
    if (num_tokens < 0) {
        return false; // Parse error
    }
    
    // Root should be an object
    if (num_tokens < 1 || tokens[0].type != JSMN_OBJECT) {
        return false;
    }
    
    // Traverse tokens and populate config
    int idx = 1; // Start after root object
    int fields = tokens[0].size; // Number of top-level key-value pairs
    
    for (int i = 0; i < fields; ++i) {
        const jsmntok_t* key = &tokens[idx++];
        
        if (tok_eq(data, key, "app_name")) {
            const jsmntok_t* val = &tokens[idx++];
            if (!tok_copy_str(data, val, g_config_jsmn.app_name.data(), g_config_jsmn.app_name.size())) return false;
        } else if (tok_eq(data, key, "version_major")) {
            const jsmntok_t* val = &tokens[idx++];
            if (!tok_parse_uint16(data, val, g_config_jsmn.version_major)) return false;
        } else if (tok_eq(data, key, "version_minor")) {
            const jsmntok_t* val = &tokens[idx++];
            if (!tok_parse_int(data, val, g_config_jsmn.version_minor)) return false;
        } else if (tok_eq(data, key, "network")) {
            if (!parse_network(data, tokens, idx, g_config_jsmn.network)) return false;
        } else if (tok_eq(data, key, "fallback_network_conf")) {
            const jsmntok_t* val = &tokens[idx];
            if (val->type == JSMN_OBJECT) {
                g_config_jsmn.fallback_network_conf.emplace();
                if (!parse_network(data, tokens, idx, *g_config_jsmn.fallback_network_conf)) return false;
            } else {
                // null or missing - skip
                idx++;
                g_config_jsmn.fallback_network_conf.reset();
            }
        } else if (tok_eq(data, key, "controller")) {
            if (!parse_controller(data, tokens, idx, g_config_jsmn.controller)) return false;
        } else if (tok_eq(data, key, "logging")) {
            if (!parse_logging(data, tokens, idx, g_config_jsmn.logging)) return false;
        } else {
            // Skip unknown field
            idx++;
        }
    }
    
    return true;
}

// Entry point
extern "C" __attribute__((used)) int main() {
    volatile bool result = parse_config_jsmn("", 0);
    (void)result;
    while(1) {}
    return 0;
}

