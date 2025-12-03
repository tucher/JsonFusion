#pragma once


#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <rapidjson/reader.h>
#include <rapidjson/memorystream.h>
#include <rapidjson/error/en.h>

#include "benchmarks_models.hpp"
#include <format>
namespace json_fusion_benchmarks {


struct RapidJSON {
    static constexpr std::string_view library_name = "RapidJSON";


    rapidjson::Document doc;

    // SAX-style handler for EmbeddedConfigStatic - no dynamic allocations!
    struct EmbeddedConfigStaticHandler : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>, EmbeddedConfigStaticHandler> {
        EmbeddedConfigSmall::EmbeddedConfigStatic& out;
        std::string& error_msg;
        bool error_occurred = false;

        // State machine for tracking parsing context
        enum State {
            ROOT, NETWORK, FALLBACK_NETWORK, CONTROLLER, MOTORS_ARRAY, MOTOR_OBJECT,
            POSITION_ARRAY, VEL_LIMITS_ARRAY, SENSORS_ARRAY, SENSOR_OBJECT, LOGGING
        };

        std::array<State, 16> state_stack;
        size_t stack_depth = 0;

        size_t current_motor_idx = 0;
        size_t current_position_idx = 0;
        size_t current_vel_limits_idx = 0;
        size_t current_sensor_idx = 0;
        size_t motors_count = 0;
        size_t sensors_count = 0;

        std::array<char, 64> current_key;
        size_t current_key_len = 0;

        bool in_fallback_network = false;
        typename decltype(out.fallback_network_conf)::value_type fallback_temp;

        EmbeddedConfigStaticHandler(EmbeddedConfigSmall::EmbeddedConfigStatic& output, std::string& err)
            : out(output), error_msg(err) {
            state_stack[0] = ROOT;
            stack_depth = 1;
        }

        void push_state(State s) {
            if (stack_depth < state_stack.size()) {
                state_stack[stack_depth++] = s;
            }
        }

        void pop_state() {
            if (stack_depth > 0) stack_depth--;
        }

        State current_state() const {
            return stack_depth > 0 ? state_stack[stack_depth - 1] : ROOT;
        }

        void copy_to_array(auto& dest, const char* src, size_t len) {
            size_t copy_len = std::min(len, dest.size() - 1);
            std::memcpy(dest.data(), src, copy_len);
            dest[copy_len] = '\0';
        }

        bool Key(const char* str, rapidjson::SizeType length, bool) {
            current_key_len = std::min<size_t>(length, current_key.size() - 1);
            std::memcpy(current_key.data(), str, current_key_len);
            current_key[current_key_len] = '\0';
            return true;
        }

        bool StartObject() {
            std::string_view key(current_key.data(), current_key_len);

            if (current_state() == ROOT) {
                if (key == "network") {
                    push_state(NETWORK);
                } else if (key == "fallback_network_conf") {
                    in_fallback_network = true;
                    push_state(FALLBACK_NETWORK);
                } else if (key == "controller") {
                    push_state(CONTROLLER);
                } else if (key == "logging") {
                    push_state(LOGGING);
                }
            } else if (current_state() == MOTORS_ARRAY) {
                if (current_motor_idx < out.controller.motors.size()) {
                    push_state(MOTOR_OBJECT);
                }
            } else if (current_state() == SENSORS_ARRAY) {
                if (current_sensor_idx < out.controller.sensors.size()) {
                    push_state(SENSOR_OBJECT);
                }
            }
            return true;
        }

        bool EndObject(rapidjson::SizeType) {
            if (current_state() == FALLBACK_NETWORK) {
                out.fallback_network_conf = fallback_temp;
                in_fallback_network = false;
            } else if (current_state() == MOTOR_OBJECT) {
                current_motor_idx++;
            } else if (current_state() == SENSOR_OBJECT) {
                current_sensor_idx++;
            }
            pop_state();
            return true;
        }

        bool StartArray() {
            std::string_view key(current_key.data(), current_key_len);

            if (current_state() == CONTROLLER && key == "motors") {
                push_state(MOTORS_ARRAY);
                current_motor_idx = 0;
                motors_count = 0;
            } else if (current_state() == CONTROLLER && key == "sensors") {
                push_state(SENSORS_ARRAY);
                current_sensor_idx = 0;
                sensors_count = 0;
            } else if (current_state() == MOTOR_OBJECT && key == "position") {
                push_state(POSITION_ARRAY);
                current_position_idx = 0;
            } else if (current_state() == MOTOR_OBJECT && key == "vel_limits") {
                push_state(VEL_LIMITS_ARRAY);
                current_vel_limits_idx = 0;
            }
            return true;
        }

        bool EndArray(rapidjson::SizeType elementCount) {
            if (current_state() == MOTORS_ARRAY) {
                // Validate min_items<1>
                if (elementCount < 1) {
                    error_msg = "motors array must have at least 1 item";
                    error_occurred = true;
                    return false;
                }
            } else if (current_state() == SENSORS_ARRAY) {
                // Validate min_items<1>
                if (elementCount < 1) {
                    error_msg = "sensors array must have at least 1 item";
                    error_occurred = true;
                    return false;
                }
            } else if (current_state() == POSITION_ARRAY) {
                // Validate min_items<3>
                if (elementCount < 3) {
                    error_msg = "position array must have at least 3 items";
                    error_occurred = true;
                    return false;
                }
            } else if (current_state() == VEL_LIMITS_ARRAY) {
                // Validate min_items<3>
                if (elementCount < 3) {
                    error_msg = "vel_limits array must have at least 3 items";
                    error_occurred = true;
                    return false;
                }
            }
            pop_state();
            return true;
        }

        bool String(const char* str, rapidjson::SizeType length, bool) {
            std::string_view key(current_key.data(), current_key_len);
            State state = current_state();

            if (state == ROOT && key == "app_name") {
                copy_to_array(out.app_name, str, length);
            } else if (state == NETWORK && key == "name") {
                copy_to_array(out.network.name, str, length);
            } else if (state == NETWORK && key == "address") {
                copy_to_array(out.network.address, str, length);
            } else if (state == FALLBACK_NETWORK && key == "name") {
                copy_to_array(fallback_temp.name, str, length);
            } else if (state == FALLBACK_NETWORK && key == "address") {
                copy_to_array(fallback_temp.address, str, length);
            } else if (state == CONTROLLER && key == "name") {
                copy_to_array(out.controller.name, str, length);
            } else if (state == MOTOR_OBJECT && key == "name" && current_motor_idx < out.controller.motors.size()) {
                copy_to_array(out.controller.motors[current_motor_idx].name, str, length);
            } else if (state == SENSOR_OBJECT && key == "type" && current_sensor_idx < out.controller.sensors.size()) {
                copy_to_array(out.controller.sensors[current_sensor_idx].type, str, length);
            } else if (state == SENSOR_OBJECT && key == "model" && current_sensor_idx < out.controller.sensors.size()) {
                copy_to_array(out.controller.sensors[current_sensor_idx].model, str, length);
            } else if (state == LOGGING && key == "path") {
                copy_to_array(out.logging.path, str, length);
            }
            return true;
        }

        bool Int(int i) {
            std::string_view key(current_key.data(), current_key_len);
            State state = current_state();

            if (state == ROOT && key == "version_minor") {
                out.version_minor = i;
            } else if (state == CONTROLLER && key == "loop_hz") {
                // Validate range<10, 10000>
                if (i < 10 || i > 10000) {
                    error_msg = std::format("loop_hz value {} out of range [10, 10000]", i);
                    error_occurred = true;
                    return false;
                }
                out.controller.loop_hz = i;
            }
            return true;
        }

        bool Uint(unsigned u) {
            std::string_view key(current_key.data(), current_key_len);
            State state = current_state();

            if (state == ROOT && key == "version_major") {
                out.version_major = static_cast<uint16_t>(u);
            } else if (state == NETWORK && key == "port") {
                out.network.port = static_cast<uint16_t>(u);
            } else if (state == FALLBACK_NETWORK && key == "port") {
                fallback_temp.port = static_cast<uint16_t>(u);
            } else if (state == LOGGING && key == "max_files") {
                out.logging.max_files = u;
            }
            return true;
        }

        bool Int64(int64_t i) {
            std::string_view key(current_key.data(), current_key_len);

            if (current_state() == MOTOR_OBJECT && key == "id" && current_motor_idx < out.controller.motors.size()) {
                out.controller.motors[current_motor_idx].id = i;
            }
            return true;
        }

        bool Double(double d) {
            std::string_view key(current_key.data(), current_key_len);
            State state = current_state();

            if (state == POSITION_ARRAY && current_motor_idx < out.controller.motors.size()) {
                if (current_position_idx < out.controller.motors[current_motor_idx].position.size()) {
                    // Validate range<-1000, 1000>
                    if (d < -1000.0 || d > 1000.0) {
                        error_msg = std::format("position[{}] value {} out of range [-1000, 1000]", current_position_idx, d);
                        error_occurred = true;
                        return false;
                    }
                    out.controller.motors[current_motor_idx].position[current_position_idx++] = d;
                }
            } else if (state == VEL_LIMITS_ARRAY && current_motor_idx < out.controller.motors.size()) {
                if (current_vel_limits_idx < out.controller.motors[current_motor_idx].vel_limits.size()) {
                    float val = static_cast<float>(d);
                    // Validate range<-1000, 1000>
                    if (val < -1000.0f || val > 1000.0f) {
                        error_msg = std::format("vel_limits[{}] value {} out of range [-1000, 1000]", current_vel_limits_idx, val);
                        error_occurred = true;
                        return false;
                    }
                    out.controller.motors[current_motor_idx].vel_limits[current_vel_limits_idx++] = val;
                }
            } else if (state == SENSOR_OBJECT && current_sensor_idx < out.controller.sensors.size()) {
                if (key == "range_min") {
                    float val = static_cast<float>(d);
                    // Validate range<-100, 100000>
                    if (val < -100.0f || val > 100000.0f) {
                        error_msg = std::format("range_min value {} out of range [-100, 100000]", val);
                        error_occurred = true;
                        return false;
                    }
                    out.controller.sensors[current_sensor_idx].range_min = val;
                } else if (key == "range_max") {
                    // Validate range<-1000, 100000>
                    if (d < -1000.0 || d > 100000.0) {
                        error_msg = std::format("range_max value {} out of range [-1000, 100000]", d);
                        error_occurred = true;
                        return false;
                    }
                    out.controller.sensors[current_sensor_idx].range_max = d;
                }
            }
            return true;
        }

        bool Bool(bool b) {
            std::string_view key(current_key.data(), current_key_len);
            State state = current_state();

            if (state == NETWORK && key == "enabled") {
                out.network.enabled = b;
            } else if (state == FALLBACK_NETWORK && key == "enabled") {
                fallback_temp.enabled = b;
            } else if (state == MOTOR_OBJECT && key == "inverted" && current_motor_idx < out.controller.motors.size()) {
                out.controller.motors[current_motor_idx].inverted = b;
            } else if (state == SENSOR_OBJECT && key == "active" && current_sensor_idx < out.controller.sensors.size()) {
                out.controller.sensors[current_sensor_idx].active = b;
            } else if (state == LOGGING && key == "enabled") {
                out.logging.enabled = b;
            }
            return true;
        }

        bool Null() {
            std::string_view key(current_key.data(), current_key_len);
            if (current_state() == ROOT && key == "fallback_network_conf") {
                out.fallback_network_conf = std::nullopt;
            }
            return true;
        }
    };

    bool parse_validate_and_populate(EmbeddedConfigSmall::EmbeddedConfigStatic& out, std::string& data, bool insitu, std::string& remark);
    bool parse_validate_and_populate(EmbeddedConfigSmall::EmbeddedConfigDynamic& out, std::string& data, bool insitu /*not needed*/, std::string& remark);
    bool parse_validate_and_populate(TelemetrySample::SamplesDynamic& out, std::string& data, bool insitu /*not needed*/, std::string& remark);
    bool parse_validate_and_populate(RPCCommand::TopLevel& out, std::string& data, bool insitu /*not needed*/, std::string& remark);
    bool parse_validate_and_populate(vector<LogEvent::Entry> & out, std::string& data, bool insitu /*not needed*/, std::string& remark);
    bool parse_validate_and_populate(vector<BusEvents_MessagePayloads::Event> & out, std::string& data, bool insitu /*not needed*/, std::string& remark);
    bool parse_validate_and_populate(vector<MetricsTimeSeries::MetricSample> & out, std::string& data, bool insitu /*not needed*/, std::string& remark);

};

}
