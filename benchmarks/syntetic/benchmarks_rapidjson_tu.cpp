#include "benchmarks_rapidjson_tu.hpp"

namespace json_fusion_benchmarks {


bool RapidJSON::parse_validate_and_populate(EmbeddedConfigSmall::EmbeddedConfigStatic& out, std::string& data, bool insitu, std::string& remark) {
    EmbeddedConfigStaticHandler handler(out, remark);
    rapidjson::Reader reader;

    if (insitu) {
        rapidjson::InsituStringStream ss(data.data());
        auto result = reader.Parse<rapidjson::kParseInsituFlag>(ss, handler);

        if (!result || handler.error_occurred) {
            if (remark.empty()) {
                remark = std::string("Parse error: ") + rapidjson::GetParseError_En(result.Code());
            }
            return false;
        }
    } else {
        rapidjson::StringStream ss(data.c_str());
        auto result = reader.Parse(ss, handler);

        if (!result || handler.error_occurred) {
            if (remark.empty()) {
                remark = std::string("Parse error: ") + rapidjson::GetParseError_En(result.Code());
            }
            return false;
        }
    }
    return true;
}
bool RapidJSON::parse_validate_and_populate(EmbeddedConfigSmall::EmbeddedConfigDynamic& out, std::string& data, bool insitu /*not needed*/, std::string& remark) {
    if(!insitu) {
        doc.Parse(data.data(), data.size());
    } else {
        doc.ParseInsitu(data.data());
    }

    if (doc.HasParseError()) {
        remark = std::string("Parse error: ") +
                 rapidjson::GetParseError_En(doc.GetParseError());
        return false;
    }

    if (!doc.IsObject()) {
        remark = "Expected root to be an object";
        return false;
    }

    // Parse app_name (REQUIRED)
    if (auto it = doc.FindMember("app_name"); it != doc.MemberEnd() && it->value.IsString()) {
        out.app_name.assign(it->value.GetString(), it->value.GetStringLength());
    } else {
        remark = "Missing or invalid required field: app_name";
        return false;
    }

    // Parse version_major (REQUIRED)
    if (auto it = doc.FindMember("version_major"); it != doc.MemberEnd() && it->value.IsUint()) {
        out.version_major = static_cast<uint16_t>(it->value.GetUint());
    } else {
        remark = "Missing or invalid required field: version_major";
        return false;
    }

    // Parse version_minor (REQUIRED)
    if (auto it = doc.FindMember("version_minor"); it != doc.MemberEnd() && it->value.IsInt()) {
        out.version_minor = it->value.GetInt();
    } else {
        remark = "Missing or invalid required field: version_minor";
        return false;
    }

    // Parse network (REQUIRED)
    if (auto it = doc.FindMember("network"); it != doc.MemberEnd() && it->value.IsObject()) {
        const auto& net = it->value;

        // network.name (REQUIRED)
        if (auto name_it = net.FindMember("name"); name_it != net.MemberEnd() && name_it->value.IsString()) {
            out.network.name.assign(name_it->value.GetString(), name_it->value.GetStringLength());
        } else {
            remark = "Missing or invalid required field: network.name";
            return false;
        }

        // network.address (REQUIRED)
        if (auto addr_it = net.FindMember("address"); addr_it != net.MemberEnd() && addr_it->value.IsString()) {
            out.network.address.assign(addr_it->value.GetString(), addr_it->value.GetStringLength());
        } else {
            remark = "Missing or invalid required field: network.address";
            return false;
        }

        // network.port (REQUIRED)
        if (auto port_it = net.FindMember("port"); port_it != net.MemberEnd() && port_it->value.IsUint()) {
            out.network.port = static_cast<uint16_t>(port_it->value.GetUint());
        } else {
            remark = "Missing or invalid required field: network.port";
            return false;
        }

        // network.enabled (REQUIRED)
        if (auto enabled_it = net.FindMember("enabled"); enabled_it != net.MemberEnd() && enabled_it->value.IsBool()) {
            out.network.enabled = enabled_it->value.GetBool();
        } else {
            remark = "Missing or invalid required field: network.enabled";
            return false;
        }
    } else {
        remark = "Missing or invalid required field: network";
        return false;
    }

    // Parse fallback_network_conf
    if (auto it = doc.FindMember("fallback_network_conf"); it != doc.MemberEnd()) {
        if (it->value.IsNull()) {
            out.fallback_network_conf = std::nullopt;
        } else if (it->value.IsObject()) {
            typename decltype(out.fallback_network_conf)::value_type fallback;
            const auto& net = it->value;
            if (auto name_it = net.FindMember("name"); name_it != net.MemberEnd() && name_it->value.IsString()) {
                fallback.name.assign(name_it->value.GetString(), name_it->value.GetStringLength());
            }
            if (auto addr_it = net.FindMember("address"); addr_it != net.MemberEnd() && addr_it->value.IsString()) {
                fallback.address.assign(addr_it->value.GetString(), addr_it->value.GetStringLength());
            }
            if (auto port_it = net.FindMember("port"); port_it != net.MemberEnd() && port_it->value.IsUint()) {
                fallback.port = static_cast<uint16_t>(port_it->value.GetUint());
            }
            if (auto enabled_it = net.FindMember("enabled"); enabled_it != net.MemberEnd() && enabled_it->value.IsBool()) {
                fallback.enabled = enabled_it->value.GetBool();
            }
            out.fallback_network_conf = std::move(fallback);
        }
    }

    // Parse controller (REQUIRED)
    if (auto it = doc.FindMember("controller"); it != doc.MemberEnd() && it->value.IsObject()) {
        const auto& ctrl = it->value;

        // controller.name (REQUIRED)
        if (auto name_it = ctrl.FindMember("name"); name_it != ctrl.MemberEnd() && name_it->value.IsString()) {
            out.controller.name.assign(name_it->value.GetString(), name_it->value.GetStringLength());
        } else {
            remark = "Missing or invalid required field: controller.name";
            return false;
        }

        // controller.loop_hz (REQUIRED)
        if (auto hz_it = ctrl.FindMember("loop_hz"); hz_it != ctrl.MemberEnd() && hz_it->value.IsInt()) {
            int hz_value = hz_it->value.GetInt();
            // Validate range<10, 10000>
            if (hz_value < 10 || hz_value > 10000) {
                remark = std::format("loop_hz value {} out of range [10, 10000]", hz_value);
                return false;
            }
            out.controller.loop_hz = hz_value;
        } else {
            remark = "Missing or invalid required field: controller.loop_hz";
            return false;
        }

        // Parse motors array (REQUIRED)
        if (auto motors_it = ctrl.FindMember("motors"); motors_it != ctrl.MemberEnd() && motors_it->value.IsArray()) {
            const auto& motors_arr = motors_it->value.GetArray();

            // Validate min_items<1>
            if (motors_arr.Size() < 1) {
                remark = "motors array must have at least 1 item";
                return false;
            }

            out.controller.motors.clear();
            out.controller.motors.value.reserve(motors_arr.Size());

            for (rapidjson::SizeType i = 0; i < motors_arr.Size(); ++i) {
                const auto& motor_obj = motors_arr[i];
                if (!motor_obj.IsObject()) continue;

                EmbeddedConfigSmall::EmbeddedConfigDynamic::Controller::Motor motor;

                if (auto id_it = motor_obj.FindMember("id"); id_it != motor_obj.MemberEnd() && id_it->value.IsInt64()) {
                    motor.id = id_it->value.GetInt64();
                }
                if (auto name_it = motor_obj.FindMember("name"); name_it != motor_obj.MemberEnd() && name_it->value.IsString()) {
                    motor.name.assign(name_it->value.GetString(), name_it->value.GetStringLength());
                }

                // Parse position array [x, y, z]
                if (auto pos_it = motor_obj.FindMember("position"); pos_it != motor_obj.MemberEnd() && pos_it->value.IsArray()) {
                    const auto& pos_arr = pos_it->value.GetArray();

                    // Validate min_items<3>
                    if (pos_arr.Size() < 3) {
                        remark = "position array must have at least 3 items";
                        return false;
                    }

                    motor.position.clear();
                    motor.position.value.reserve(pos_arr.Size());
                    for (rapidjson::SizeType j = 0; j < pos_arr.Size(); ++j) {
                        if (pos_arr[j].IsDouble()) {
                            double val = pos_arr[j].GetDouble();
                            // Validate range<-1000, 1000>
                            if (val < -1000.0 || val > 1000.0) {
                                remark = std::format("position[{}] value {} out of range [-1000, 1000]", j, val);
                                return false;
                            }
                            motor.position.value.push_back({val});
                        }
                    }
                }

                // Parse vel_limits array [vx, vy, vz]
                if (auto vel_it = motor_obj.FindMember("vel_limits"); vel_it != motor_obj.MemberEnd() && vel_it->value.IsArray()) {
                    const auto& vel_arr = vel_it->value.GetArray();

                    // Validate min_items<3>
                    if (vel_arr.Size() < 3) {
                        remark = "vel_limits array must have at least 3 items";
                        return false;
                    }

                    motor.vel_limits.clear();
                    motor.vel_limits.value.reserve(vel_arr.Size());
                    for (rapidjson::SizeType j = 0; j < vel_arr.Size(); ++j) {
                        if (vel_arr[j].IsDouble()) {
                            float val = static_cast<float>(vel_arr[j].GetDouble());
                            // Validate range<-1000, 1000>
                            if (val < -1000.0f || val > 1000.0f) {
                                remark = std::format("vel_limits[{}] value {} out of range [-1000, 1000]", j, val);
                                return false;
                            }
                            motor.vel_limits.value.push_back({val});
                        }
                    }
                }

                if (auto inv_it = motor_obj.FindMember("inverted"); inv_it != motor_obj.MemberEnd() && inv_it->value.IsBool()) {
                    motor.inverted = inv_it->value.GetBool();
                }

                out.controller.motors.push_back(std::move(motor));
            }
        } else {
            remark = "Missing or invalid required field: controller.motors";
            return false;
        }

        // Parse sensors array (REQUIRED)
        if (auto sensors_it = ctrl.FindMember("sensors"); sensors_it != ctrl.MemberEnd() && sensors_it->value.IsArray()) {
            const auto& sensors_arr = sensors_it->value.GetArray();

            // Validate min_items<1>
            if (sensors_arr.Size() < 1) {
                remark = "sensors array must have at least 1 item";
                return false;
            }

            out.controller.sensors.clear();
            out.controller.sensors.value.reserve(sensors_arr.Size());

            for (rapidjson::SizeType i = 0; i < sensors_arr.Size(); ++i) {
                const auto& sensor_obj = sensors_arr[i];
                if (!sensor_obj.IsObject()) continue;

                EmbeddedConfigSmall::EmbeddedConfigDynamic::Controller::Sensor sensor;

                if (auto type_it = sensor_obj.FindMember("type"); type_it != sensor_obj.MemberEnd() && type_it->value.IsString()) {
                    sensor.type.assign(type_it->value.GetString(), type_it->value.GetStringLength());
                }
                if (auto model_it = sensor_obj.FindMember("model"); model_it != sensor_obj.MemberEnd() && model_it->value.IsString()) {
                    sensor.model.assign(model_it->value.GetString(), model_it->value.GetStringLength());
                }
                if (auto min_it = sensor_obj.FindMember("range_min"); min_it != sensor_obj.MemberEnd() && min_it->value.IsDouble()) {
                    float min_val = static_cast<float>(min_it->value.GetDouble());
                    // Validate range<-100, 100000>
                    if (min_val < -100.0f || min_val > 100000.0f) {
                        remark = std::format("range_min value {} out of range [-100, 100000]", min_val);
                        return false;
                    }
                    sensor.range_min = min_val;
                }
                if (auto max_it = sensor_obj.FindMember("range_max"); max_it != sensor_obj.MemberEnd() && max_it->value.IsDouble()) {
                    double max_val = max_it->value.GetDouble();
                    // Validate range<-1000, 100000>
                    if (max_val < -1000.0 || max_val > 100000.0) {
                        remark = std::format("range_max value {} out of range [-1000, 100000]", max_val);
                        return false;
                    }
                    sensor.range_max = max_val;
                }
                if (auto active_it = sensor_obj.FindMember("active"); active_it != sensor_obj.MemberEnd() && active_it->value.IsBool()) {
                    sensor.active = active_it->value.GetBool();
                }

                out.controller.sensors.push_back(std::move(sensor));
            }
        } else {
            remark = "Missing or invalid required field: controller.sensors";
            return false;
        }
    } else {
        remark = "Missing or invalid required field: controller";
        return false;
    }

    // Parse logging (REQUIRED)
    if (auto it = doc.FindMember("logging"); it != doc.MemberEnd() && it->value.IsObject()) {
        const auto& log = it->value;

        // logging.enabled (REQUIRED)
        if (auto enabled_it = log.FindMember("enabled"); enabled_it != log.MemberEnd() && enabled_it->value.IsBool()) {
            out.logging.enabled = enabled_it->value.GetBool();
        } else {
            remark = "Missing or invalid required field: logging.enabled";
            return false;
        }

        // logging.path (REQUIRED)
        if (auto path_it = log.FindMember("path"); path_it != log.MemberEnd() && path_it->value.IsString()) {
            out.logging.path.assign(path_it->value.GetString(), path_it->value.GetStringLength());
        } else {
            remark = "Missing or invalid required field: logging.path";
            return false;
        }

        // logging.max_files (REQUIRED)
        if (auto max_files_it = log.FindMember("max_files"); max_files_it != log.MemberEnd() && max_files_it->value.IsUint()) {
            out.logging.max_files = max_files_it->value.GetUint();
        } else {
            remark = "Missing or invalid required field: logging.max_files";
            return false;
        }
    } else {
        remark = "Missing or invalid required field: logging";
        return false;
    }

    return true;
}
bool RapidJSON::parse_validate_and_populate(TelemetrySample::SamplesDynamic& out, std::string& data, bool insitu /*not needed*/, std::string& remark) {
    if(!insitu) {
        doc.Parse(data.data(), data.size());
    } else {
        doc.ParseInsitu(data.data());
    }

    if (doc.HasParseError()) {
        remark = std::string("Parse error: ") +
                 rapidjson::GetParseError_En(doc.GetParseError());
        return false;
    }

    if (!doc.IsObject()) {
        remark = "Expected root to be an object";
        return false;
    }

    // Parse samples array (REQUIRED)
    if (auto samples_it = doc.FindMember("samples"); samples_it != doc.MemberEnd() && samples_it->value.IsArray()) {
        const auto& samples_arr = samples_it->value.GetArray();
        out.samples.clear();

        for (rapidjson::SizeType i = 0; i < samples_arr.Size(); ++i) {
            const auto& sample_obj = samples_arr[i];
            if (!sample_obj.IsObject()) continue;

            TelemetrySample::TelemetrySampleDynamic sample;

            // Parse device_id (REQUIRED, with min_length<1> validation)
            if (auto id_it = sample_obj.FindMember("device_id"); id_it != sample_obj.MemberEnd() && id_it->value.IsString()) {
                std::string_view dev_id(id_it->value.GetString(), id_it->value.GetStringLength());
                if (dev_id.length() < 1) {
                    remark = "device_id must have at least 1 character";
                    return false;
                }
                sample.device_id.value.assign(dev_id.data(), dev_id.size());
            } else {
                remark = std::format("Missing or invalid required field: samples[{}].device_id", i);
                return false;
            }

            // Parse timestamp (REQUIRED)
            if (auto ts_it = sample_obj.FindMember("timestamp"); ts_it != sample_obj.MemberEnd() && ts_it->value.IsInt64()) {
                sample.timestamp = ts_it->value.GetInt64();
            } else {
                remark = std::format("Missing or invalid required field: samples[{}].timestamp", i);
                return false;
            }

            // Parse battery (REQUIRED, with range<0, 5> validation)
            if (auto batt_it = sample_obj.FindMember("battery"); batt_it != sample_obj.MemberEnd() && batt_it->value.IsDouble()) {
                float batt_val = static_cast<float>(batt_it->value.GetDouble());
                if (batt_val < 0.0f || batt_val > 5.0f) {
                    remark = std::format("battery value {} out of range [0, 5]", batt_val);
                    return false;
                }
                sample.battery = batt_val;
            } else {
                remark = std::format("Missing or invalid required field: samples[{}].battery", i);
                return false;
            }

            // Parse temp_c (REQUIRED, with range<-100, 150> validation)
            if (auto temp_it = sample_obj.FindMember("temp_c"); temp_it != sample_obj.MemberEnd() && temp_it->value.IsDouble()) {
                float temp_val = static_cast<float>(temp_it->value.GetDouble());
                if (temp_val < -100.0f || temp_val > 150.0f) {
                    remark = std::format("temp_c value {} out of range [-100, 150]", temp_val);
                    return false;
                }
                sample.temp_c = temp_val;
            } else {
                remark = std::format("Missing or invalid required field: samples[{}].temp_c", i);
                return false;
            }

            // Parse errors array (REQUIRED)
            if (auto err_it = sample_obj.FindMember("errors"); err_it != sample_obj.MemberEnd() && err_it->value.IsArray()) {
                const auto& errors_arr = err_it->value.GetArray();
                sample.errors.clear();
                sample.errors.reserve(errors_arr.Size());
                for (rapidjson::SizeType j = 0; j < errors_arr.Size(); ++j) {
                    if (errors_arr[j].IsString()) {
                        sample.errors.emplace_back(errors_arr[j].GetString(), errors_arr[j].GetStringLength());
                    }
                }
            } else {
                remark = std::format("Missing or invalid required field: samples[{}].errors", i);
                return false;
            }

            // Parse tags map (REQUIRED, with max_properties<16> and max_key_length<32> validation)
            if (auto tags_it = sample_obj.FindMember("tags"); tags_it != sample_obj.MemberEnd() && tags_it->value.IsObject()) {
                const auto& tags_obj = tags_it->value.GetObject();

                // Validate max_properties<16>
                if (tags_obj.MemberCount() > 16) {
                    remark = "tags map exceeds max_properties<16>";
                    return false;
                }

                sample.tags.clear();
                sample.tags.value.reserve(tags_obj.MemberCount());

                for (auto tag_it = tags_obj.MemberBegin(); tag_it != tags_obj.MemberEnd(); ++tag_it) {
                    std::string_view key(tag_it->name.GetString(), tag_it->name.GetStringLength());

                    // Validate max_key_length<32>
                    if (key.length() > 32) {
                        remark = std::format("tags key '{}' exceeds max_key_length<32>", key);
                        return false;
                    }

                    if (tag_it->value.IsString()) {
                        sample.tags.value.emplace(
                            std::string(key),
                            std::string(tag_it->value.GetString(), tag_it->value.GetStringLength())
                            );
                    }
                }
            } else {
                remark = std::format("Missing or invalid required field: samples[{}].tags", i);
                return false;
            }

            // Parse accel (OPTIONAL Vec3 as array)
            if (auto accel_it = sample_obj.FindMember("accel"); accel_it != sample_obj.MemberEnd()) {
                if (accel_it->value.IsArray()) {
                    const auto& accel_arr = accel_it->value.GetArray();
                    if (accel_arr.Size() >= 3) {
                        TelemetrySample::TelemetrySampleDynamic::Vec3 vec3;
                        vec3.x = accel_arr[0].IsDouble() ? static_cast<float>(accel_arr[0].GetDouble()) : 0.0f;
                        vec3.y = accel_arr[1].IsDouble() ? static_cast<float>(accel_arr[1].GetDouble()) : 0.0f;
                        vec3.z = accel_arr[2].IsDouble() ? static_cast<float>(accel_arr[2].GetDouble()) : 0.0f;
                        sample.accel = vec3;
                    }
                } else if (accel_it->value.IsNull()) {
                    sample.accel = std::nullopt;
                }
            }

            out.samples.push_back(std::move(sample));
        }
    } else {
        remark = "Missing or invalid required field: samples";
        return false;
    }

    return true;
}
bool RapidJSON::parse_validate_and_populate(RPCCommand::TopLevel& out, std::string& data, bool insitu /*not needed*/, std::string& remark) {
    if(!insitu) {
        doc.Parse(data.data(), data.size());
    } else {
        doc.ParseInsitu(data.data());
    }

    if (doc.HasParseError()) {
        remark = std::string("Parse error: ") +
                 rapidjson::GetParseError_En(doc.GetParseError());
        return false;
    }

    if (!doc.IsObject()) {
        remark = "Expected root to be an object";
        return false;
    }

    // Parse commands array (REQUIRED)
    if (auto cmds_it = doc.FindMember("commands"); cmds_it != doc.MemberEnd() && cmds_it->value.IsArray()) {
        const auto& cmds_arr = cmds_it->value.GetArray();
        out.commands.clear();
        out.commands.reserve(cmds_arr.Size());

        for (rapidjson::SizeType i = 0; i < cmds_arr.Size(); ++i) {
            const auto& cmd_obj = cmds_arr[i];
            if (!cmd_obj.IsObject()) continue;

            RPCCommand::Cmd cmd;

            // Parse cmd field (REQUIRED, with enum_values validation)
            if (auto cmd_it = cmd_obj.FindMember("cmd"); cmd_it != cmd_obj.MemberEnd() && cmd_it->value.IsString()) {
                std::string_view cmd_val(cmd_it->value.GetString(), cmd_it->value.GetStringLength());
                // Validate enum_values<"set_param", "start_job", "stop_job">
                if (cmd_val != "set_param" && cmd_val != "start_job" && cmd_val != "stop_job") {
                    remark = std::format("cmd value '{}' not in allowed enum values", cmd_val);
                    return false;
                }
                cmd.cmd.value.assign(cmd_val.data(), cmd_val.size());
            } else {
                remark = std::format("Missing or invalid required field: commands[{}].cmd", i);
                return false;
            }

            // Parse correlation_id (JSON key is "id", not "correlation_id")
            if (auto id_it = cmd_obj.FindMember("id"); id_it != cmd_obj.MemberEnd() && id_it->value.IsString()) {
                cmd.correlation_id.value.assign(id_it->value.GetString(), id_it->value.GetStringLength());
            }

            // Parse set_param payload (optional)
            if (auto sp_it = cmd_obj.FindMember("set_param"); sp_it != cmd_obj.MemberEnd() && sp_it->value.IsObject()) {
                const auto& sp_obj = sp_it->value;
                RPCCommand::SetParamPayload payload;

                if (auto hz_it = sp_obj.FindMember("loop_hz"); hz_it != sp_obj.MemberEnd()) {
                    if (hz_it->value.IsNull()) {
                        payload.loop_hz = std::nullopt;
                    } else if (hz_it->value.IsInt()) {
                        payload.loop_hz = hz_it->value.GetInt();
                    }
                }

                if (auto log_it = sp_obj.FindMember("logging_enabled"); log_it != sp_obj.MemberEnd()) {
                    if (log_it->value.IsNull()) {
                        payload.logging_enabled = std::nullopt;
                    } else if (log_it->value.IsBool()) {
                        payload.logging_enabled = log_it->value.GetBool();
                    }
                }

                if (auto lvl_it = sp_obj.FindMember("log_level"); lvl_it != sp_obj.MemberEnd()) {
                    if (lvl_it->value.IsNull()) {
                        payload.log_level = std::nullopt;
                    } else if (lvl_it->value.IsString()) {
                        payload.log_level.emplace(lvl_it->value.GetString(), lvl_it->value.GetStringLength());
                    }
                }

                cmd.set_param = std::move(payload);
            }

            // Parse start_job payload (optional)
            if (auto sj_it = cmd_obj.FindMember("start_job"); sj_it != cmd_obj.MemberEnd() && sj_it->value.IsObject()) {
                const auto& sj_obj = sj_it->value;
                RPCCommand::StartJobPayload payload;

                if (auto job_it = sj_obj.FindMember("job_id"); job_it != sj_obj.MemberEnd() && job_it->value.IsString()) {
                    std::string_view job_id(job_it->value.GetString(), job_it->value.GetStringLength());
                    // Validate min_length<1>
                    if (job_id.length() < 1) {
                        remark = "job_id must have at least 1 character";
                        return false;
                    }
                    payload.job_id.value.assign(job_id.data(), job_id.size());
                }

                if (auto mode_it = sj_obj.FindMember("mode"); mode_it != sj_obj.MemberEnd()) {
                    if (mode_it->value.IsNull()) {
                        payload.mode = std::nullopt;
                    } else if (mode_it->value.IsString()) {
                        payload.mode.emplace(mode_it->value.GetString(), mode_it->value.GetStringLength());
                    }
                }

                cmd.start_job = std::move(payload);
            }

            // Wrap Cmd in Command (which is Annotated<Cmd, ...>)
            RPCCommand::Command command;
            command.value = std::move(cmd);
            out.commands.push_back(std::move(command));
        }
    } else {
        remark = "Missing or invalid required field: commands";
        return false;
    }

    return true;
}

bool RapidJSON::parse_validate_and_populate(vector<LogEvent::Entry> & out, std::string& data, bool insitu /*not needed*/, std::string& remark) {
    if(!insitu) {
        doc.Parse(data.data(), data.size());
    } else {
        doc.ParseInsitu(data.data());
    }

    if (doc.HasParseError()) {
        remark = std::string("Parse error: ") +
                 rapidjson::GetParseError_En(doc.GetParseError());
        return false;
    }

    if (!doc.IsArray()) {
        remark = "Expected root to be an array";
        return false;
    }

    const auto& entries_arr = doc.GetArray();
    out.clear();
    out.reserve(entries_arr.Size());

    for (rapidjson::SizeType i = 0; i < entries_arr.Size(); ++i) {
        const auto& entry_obj = entries_arr[i];
        if (!entry_obj.IsObject()) continue;

        LogEvent::Entry entry;

        // Parse timestamp (REQUIRED, JSON key is "ts")
        if (auto ts_it = entry_obj.FindMember("ts"); ts_it != entry_obj.MemberEnd() && ts_it->value.IsString()) {
            entry.timestamp.value.assign(ts_it->value.GetString(), ts_it->value.GetStringLength());
        } else {
            remark = std::format("Missing or invalid required field: [{}].ts", i);
            return false;
        }

        // Parse level (REQUIRED, with enum_values validation)
        if (auto lvl_it = entry_obj.FindMember("level"); lvl_it != entry_obj.MemberEnd() && lvl_it->value.IsString()) {
            std::string_view level_val(lvl_it->value.GetString(), lvl_it->value.GetStringLength());
            // Validate enum_values<"TRACE", "DEBUG", "INFO", "WARN", "ERROR">
            if (level_val != "TRACE" && level_val != "DEBUG" && level_val != "INFO" &&
                level_val != "WARN" && level_val != "ERROR") {
                remark = std::format("level value '{}' not in allowed enum values", level_val);
                return false;
            }
            entry.level.value.assign(level_val.data(), level_val.size());
        } else {
            remark = std::format("Missing or invalid required field: [{}].level", i);
            return false;
        }

        // Parse logger (REQUIRED)
        if (auto log_it = entry_obj.FindMember("logger"); log_it != entry_obj.MemberEnd() && log_it->value.IsString()) {
            entry.logger.assign(log_it->value.GetString(), log_it->value.GetStringLength());
        } else {
            remark = std::format("Missing or invalid required field: [{}].logger", i);
            return false;
        }

        // Parse message (REQUIRED)
        if (auto msg_it = entry_obj.FindMember("message"); msg_it != entry_obj.MemberEnd() && msg_it->value.IsString()) {
            entry.message.assign(msg_it->value.GetString(), msg_it->value.GetStringLength());
        } else {
            remark = std::format("Missing or invalid required field: [{}].message", i);
            return false;
        }

        // Parse optional order_id
        if (auto oid_it = entry_obj.FindMember("order_id"); oid_it != entry_obj.MemberEnd()) {
            if (oid_it->value.IsNull()) {
                entry.order_id = std::nullopt;
            } else if (oid_it->value.IsString()) {
                entry.order_id.emplace(oid_it->value.GetString(), oid_it->value.GetStringLength());
            }
        }

        // Parse optional user_id
        if (auto uid_it = entry_obj.FindMember("user_id"); uid_it != entry_obj.MemberEnd()) {
            if (uid_it->value.IsNull()) {
                entry.user_id = std::nullopt;
            } else if (uid_it->value.IsString()) {
                entry.user_id.emplace(uid_it->value.GetString(), uid_it->value.GetStringLength());
            }
        }

        // Parse context map (REQUIRED, with max_properties<32> and max_key_length<32> validation)
        if (auto ctx_it = entry_obj.FindMember("context"); ctx_it != entry_obj.MemberEnd() && ctx_it->value.IsObject()) {
            const auto& ctx_obj = ctx_it->value.GetObject();

            // Validate max_properties<32>
            if (ctx_obj.MemberCount() > 32) {
                remark = "context map exceeds max_properties<32>";
                return false;
            }

            entry.context.clear();
            entry.context.value.reserve(ctx_obj.MemberCount());

            for (auto it = ctx_obj.MemberBegin(); it != ctx_obj.MemberEnd(); ++it) {
                std::string_view key(it->name.GetString(), it->name.GetStringLength());

                // Validate max_key_length<32>
                if (key.length() > 32) {
                    remark = std::format("context key '{}' exceeds max_key_length<32>", key);
                    return false;
                }

                if (it->value.IsString()) {
                    entry.context.value.emplace(
                        std::string(key),
                        std::string(it->value.GetString(), it->value.GetStringLength())
                        );
                }
            }
        } else {
            remark = std::format("Missing or invalid required field: [{}].context", i);
            return false;
        }

        // Parse tags map (REQUIRED, with max_properties<32> and max_key_length<32> validation)
        if (auto tags_it = entry_obj.FindMember("tags"); tags_it != entry_obj.MemberEnd() && tags_it->value.IsObject()) {
            const auto& tags_obj = tags_it->value.GetObject();

            // Validate max_properties<32>
            if (tags_obj.MemberCount() > 32) {
                remark = "tags map exceeds max_properties<32>";
                return false;
            }

            entry.tags.clear();
            entry.tags.value.reserve(tags_obj.MemberCount());

            for (auto it = tags_obj.MemberBegin(); it != tags_obj.MemberEnd(); ++it) {
                std::string_view key(it->name.GetString(), it->name.GetStringLength());

                // Validate max_key_length<32>
                if (key.length() > 32) {
                    remark = std::format("tags key '{}' exceeds max_key_length<32>", key);
                    return false;
                }

                if (it->value.IsString()) {
                    entry.tags.value.emplace(
                        std::string(key),
                        std::string(it->value.GetString(), it->value.GetStringLength())
                        );
                }
            }
        } else {
            remark = std::format("Missing or invalid required field: [{}].tags", i);
            return false;
        }

        out.push_back(std::move(entry));
    }

    return true;
}

bool RapidJSON::parse_validate_and_populate(vector<BusEvents_MessagePayloads::Event> & out, std::string& data, bool insitu /*not needed*/, std::string& remark) {
    if(!insitu) {
        doc.Parse(data.data(), data.size());
    } else {
        doc.ParseInsitu(data.data());
    }

    if (doc.HasParseError()) {
        remark = std::string("Parse error: ") +
                 rapidjson::GetParseError_En(doc.GetParseError());
        return false;
    }

    if (!doc.IsArray()) {
        remark = "Expected root to be an array";
        return false;
    }

    const auto& events_arr = doc.GetArray();
    out.clear();
    out.reserve(events_arr.Size());

    for (rapidjson::SizeType i = 0; i < events_arr.Size(); ++i) {
        const auto& event_obj = events_arr[i];
        if (!event_obj.IsObject()) continue;

        BusEvents_MessagePayloads::Event event;

        // Parse event_type (with enum_values validation)
        if (auto type_it = event_obj.FindMember("event_type"); type_it != event_obj.MemberEnd() && type_it->value.IsString()) {
            std::string_view type_val(type_it->value.GetString(), type_it->value.GetStringLength());
            // Validate enum_values<"OrderCreated", "OrderPaid", "OrderCancelled">
            if (type_val != "OrderCreated" && type_val != "OrderPaid" && type_val != "OrderCancelled") {
                remark = std::format("event_type value '{}' not in allowed enum values", type_val);
                return false;
            }
            event.event_type.value.assign(type_val.data(), type_val.size());
        }

        // Parse event_version
        if (auto ver_it = event_obj.FindMember("event_version"); ver_it != event_obj.MemberEnd() && ver_it->value.IsInt()) {
            event.event_version = ver_it->value.GetInt();
        }

        // Parse event_id
        if (auto id_it = event_obj.FindMember("event_id"); id_it != event_obj.MemberEnd() && id_it->value.IsString()) {
            event.event_id.assign(id_it->value.GetString(), id_it->value.GetStringLength());
        }

        // Parse timestamp
        if (auto ts_it = event_obj.FindMember("timestamp"); ts_it != event_obj.MemberEnd() && ts_it->value.IsInt64()) {
            event.timestamp = ts_it->value.GetInt64();
        }

        // Parse payload
        if (auto payload_it = event_obj.FindMember("payload"); payload_it != event_obj.MemberEnd() && payload_it->value.IsObject()) {
            const auto& payload_obj = payload_it->value;

            // Parse order_id (with min_length<1> validation)
            if (auto oid_it = payload_obj.FindMember("order_id"); oid_it != payload_obj.MemberEnd() && oid_it->value.IsString()) {
                std::string_view order_id(oid_it->value.GetString(), oid_it->value.GetStringLength());
                if (order_id.length() < 1) {
                    remark = "order_id must have at least 1 character";
                    return false;
                }
                event.payload.order_id.value.assign(order_id.data(), order_id.size());
            }

            // Parse customer_id (with min_length<1> validation)
            if (auto cid_it = payload_obj.FindMember("customer_id"); cid_it != payload_obj.MemberEnd() && cid_it->value.IsString()) {
                std::string_view customer_id(cid_it->value.GetString(), cid_it->value.GetStringLength());
                if (customer_id.length() < 1) {
                    remark = "customer_id must have at least 1 character";
                    return false;
                }
                event.payload.customer_id.value.assign(customer_id.data(), customer_id.size());
            }

            // Parse currency (with enum_values validation)
            if (auto cur_it = payload_obj.FindMember("currency"); cur_it != payload_obj.MemberEnd() && cur_it->value.IsString()) {
                std::string_view currency(cur_it->value.GetString(), cur_it->value.GetStringLength());
                // Validate enum_values<"USD", "EUR", "GBP">
                if (currency != "USD" && currency != "EUR" && currency != "GBP") {
                    remark = std::format("currency value '{}' not in allowed enum values", currency);
                    return false;
                }
                event.payload.currency.value.assign(currency.data(), currency.size());
            }

            // Parse lines array (with min_items<1> validation)
            if (auto lines_it = payload_obj.FindMember("lines"); lines_it != payload_obj.MemberEnd() && lines_it->value.IsArray()) {
                const auto& lines_arr = lines_it->value.GetArray();

                // Validate min_items<1>
                if (lines_arr.Size() < 1) {
                    remark = "lines array must have at least 1 item";
                    return false;
                }

                event.payload.lines.clear();
                event.payload.lines.value.reserve(lines_arr.Size());

                for (rapidjson::SizeType j = 0; j < lines_arr.Size(); ++j) {
                    const auto& line_obj = lines_arr[j];
                    if (!line_obj.IsObject()) continue;

                    BusEvents_MessagePayloads::Event::OrderPayload::OrderLine line;

                    // Parse sku (with min_length<1> validation)
                    if (auto sku_it = line_obj.FindMember("sku"); sku_it != line_obj.MemberEnd() && sku_it->value.IsString()) {
                        std::string_view sku(sku_it->value.GetString(), sku_it->value.GetStringLength());
                        if (sku.length() < 1) {
                            remark = "sku must have at least 1 character";
                            return false;
                        }
                        line.sku.value.assign(sku.data(), sku.size());
                    }

                    // Parse qty (with range<1, 1000000> validation)
                    if (auto qty_it = line_obj.FindMember("qty"); qty_it != line_obj.MemberEnd() && qty_it->value.IsInt()) {
                        int qty_val = qty_it->value.GetInt();
                        if (qty_val < 1 || qty_val > 1000000) {
                            remark = std::format("qty value {} out of range [1, 1000000]", qty_val);
                            return false;
                        }
                        line.qty = qty_val;
                    }

                    // Parse price (with range<0, 1000000> validation)
                    if (auto price_it = line_obj.FindMember("price"); price_it != line_obj.MemberEnd() && price_it->value.IsDouble()) {
                        double price_val = price_it->value.GetDouble();
                        if (price_val < 0.0 || price_val > 1000000.0) {
                            remark = std::format("price value {} out of range [0, 1000000]", price_val);
                            return false;
                        }
                        line.price = price_val;
                    }

                    event.payload.lines.push_back(std::move(line));
                }
            }

            // Parse total (with range<0, 1000000> validation)
            if (auto total_it = payload_obj.FindMember("total"); total_it != payload_obj.MemberEnd() && total_it->value.IsDouble()) {
                double total_val = total_it->value.GetDouble();
                if (total_val < 0.0 || total_val > 1000000.0) {
                    remark = std::format("total value {} out of range [0, 1000000]", total_val);
                    return false;
                }
                event.payload.total = total_val;
            }
        }

        // Parse meta map (with max_properties<32> and max_key_length<64> validation)
        if (auto meta_it = event_obj.FindMember("meta"); meta_it != event_obj.MemberEnd() && meta_it->value.IsObject()) {
            const auto& meta_obj = meta_it->value.GetObject();

            // Validate max_properties<32>
            if (meta_obj.MemberCount() > 32) {
                remark = "meta map exceeds max_properties<32>";
                return false;
            }

            event.meta.clear();
            event.meta.value.reserve(meta_obj.MemberCount());

            for (auto it = meta_obj.MemberBegin(); it != meta_obj.MemberEnd(); ++it) {
                std::string_view key(it->name.GetString(), it->name.GetStringLength());

                // Validate max_key_length<64>
                if (key.length() > 64) {
                    remark = std::format("meta key '{}' exceeds max_key_length<64>", key);
                    return false;
                }

                if (it->value.IsString()) {
                    event.meta.value.emplace(
                        std::string(key),
                        std::string(it->value.GetString(), it->value.GetStringLength())
                        );
                }
            }
        }

        out.push_back(std::move(event));
    }

    return true;
}

bool RapidJSON::parse_validate_and_populate(vector<MetricsTimeSeries::MetricSample> & out, std::string& data, bool insitu /*not needed*/, std::string& remark) {
    if(!insitu) {
        doc.Parse(data.data(), data.size());
    } else {
        doc.ParseInsitu(data.data());
    }

    if (doc.HasParseError()) {
        remark = std::string("Parse error: ") +
                 rapidjson::GetParseError_En(doc.GetParseError());
        return false;
    }

    if (!doc.IsArray()) {
        remark = "Expected root to be an array";
        return false;
    }

    const auto& samples_arr = doc.GetArray();
    out.clear();
    out.reserve(samples_arr.Size());

    for (rapidjson::SizeType i = 0; i < samples_arr.Size(); ++i) {
        const auto& sample_obj = samples_arr[i];
        if (!sample_obj.IsObject()) continue;

        MetricsTimeSeries::MetricSample sample;

        // Parse metric (with min_length<1> validation)
        if (auto metric_it = sample_obj.FindMember("metric"); metric_it != sample_obj.MemberEnd() && metric_it->value.IsString()) {
            std::string_view metric(metric_it->value.GetString(), metric_it->value.GetStringLength());
            if (metric.length() < 1) {
                remark = "metric must have at least 1 character";
                return false;
            }
            sample.metric.value.assign(metric.data(), metric.size());
        }

        // Parse value
        if (auto val_it = sample_obj.FindMember("value"); val_it != sample_obj.MemberEnd() && val_it->value.IsDouble()) {
            sample.value = val_it->value.GetDouble();
        }

        // Parse ts
        if (auto ts_it = sample_obj.FindMember("ts"); ts_it != sample_obj.MemberEnd() && ts_it->value.IsInt64()) {
            sample.ts = ts_it->value.GetInt64();
        }

        // Parse labels map (with max_properties<16> and max_key_length<32> validation)
        if (auto labels_it = sample_obj.FindMember("labels"); labels_it != sample_obj.MemberEnd() && labels_it->value.IsObject()) {
            const auto& labels_obj = labels_it->value.GetObject();

            // Validate max_properties<16>
            if (labels_obj.MemberCount() > 16) {
                remark = "labels map exceeds max_properties<16>";
                return false;
            }

            sample.labels.clear();
            sample.labels.value.reserve(labels_obj.MemberCount());

            for (auto it = labels_obj.MemberBegin(); it != labels_obj.MemberEnd(); ++it) {
                std::string_view key(it->name.GetString(), it->name.GetStringLength());

                // Validate max_key_length<32>
                if (key.length() > 32) {
                    remark = std::format("labels key '{}' exceeds max_key_length<32>", key);
                    return false;
                }

                if (it->value.IsString()) {
                    sample.labels.value.emplace(
                        std::string(key),
                        std::string(it->value.GetString(), it->value.GetStringLength())
                        );
                }
            }
        }

        out.push_back(std::move(sample));
    }

    return true;
}
}
