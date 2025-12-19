
#include "canada_json_parsing.hpp"
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <iostream>

// RapidJSON DOM parsing + population
void rj_parse_populate(int iterations, std::string &json_data)
{
    rapidjson::Document doc;
    Canada canada;

    benchmark("RapidJSON DOM Parse + Populate", iterations, [&]() {
        std::string copy = json_data;

        doc.ParseInsitu(copy.data());
        if (doc.HasParseError()) {
            std::cerr << std::format("RapidJSON parse error: {}",
                                     rapidjson::GetParseError_En(doc.GetParseError())) <<std::endl;
        }

        // Populate Canada structure from DOM
        canada.features.clear();

        if (!doc.IsObject()) return false;

        // Parse type
        if (auto it = doc.FindMember("type"); it != doc.MemberEnd() && it->value.IsString()) {
            canada.type->assign(it->value.GetString(), it->value.GetStringLength());
        }

        // Parse features array
        if (auto features_it = doc.FindMember("features"); features_it != doc.MemberEnd() && features_it->value.IsArray()) {
            const auto& features_arr = features_it->value.GetArray();
            canada.features.reserve(features_arr.Size());

            for (rapidjson::SizeType i = 0; i < features_arr.Size(); ++i) {
                const auto& feature_obj = features_arr[i];
                if (!feature_obj.IsObject()) continue;

                Canada::Feature & feature = canada.features.emplace_back();

                // Parse properties
                if (auto props_it = feature_obj.FindMember("properties"); props_it != feature_obj.MemberEnd() && props_it->value.IsObject()) {
                    const auto& props_obj = props_it->value.GetObject();
                    feature.properties.clear();
                    for (auto prop_it = props_obj.MemberBegin(); prop_it != props_obj.MemberEnd(); ++prop_it) {
                        if (prop_it->value.IsString()) {
                            feature.properties.emplace(
                                std::string(prop_it->name.GetString(), prop_it->name.GetStringLength()),
                                std::string(prop_it->value.GetString(), prop_it->value.GetStringLength())
                                );
                        }
                    }
                }

                // Parse type
                if (auto type_it = feature_obj.FindMember("type"); type_it != feature_obj.MemberEnd() && type_it->value.IsString()) {
                    feature.type->assign(type_it->value.GetString(), type_it->value.GetStringLength());
                }

                // Parse geometry
                if (auto geom_it = feature_obj.FindMember("geometry"); geom_it != feature_obj.MemberEnd() && geom_it->value.IsObject()) {
                    const auto& geom_obj = geom_it->value;

                    // Parse geometry type
                    if (auto gtype_it = geom_obj.FindMember("type"); gtype_it != geom_obj.MemberEnd() && gtype_it->value.IsString()) {
                        feature.geometry.type->assign(gtype_it->value.GetString(), gtype_it->value.GetStringLength());
                    }

                    // Parse coordinates
                    if (auto coords_it = geom_obj.FindMember("coordinates"); coords_it != geom_obj.MemberEnd() && coords_it->value.IsArray()) {
                        const auto& coords_arr = coords_it->value.GetArray();
                        feature.geometry.coordinates.clear();
                        feature.geometry.coordinates.reserve(coords_arr.Size());

                        for (rapidjson::SizeType j = 0; j < coords_arr.Size(); ++j) {
                            auto & polygon = feature.geometry.coordinates.emplace_back();
                            if (!coords_arr[j].IsArray()) continue;
                            const auto& poly_arr = coords_arr[j].GetArray();

                            polygon.reserve(poly_arr.Size());

                            for (rapidjson::SizeType k = 0; k < poly_arr.Size(); ++k) {
                                if (!poly_arr[k].IsArray() || poly_arr[k].Size() < 2) continue;
                                const auto& point_arr = poly_arr[k].GetArray();

                                Canada::Feature::Geometry::Point p;
                                p.x = point_arr[0].IsDouble() ? static_cast<float>(point_arr[0].GetDouble()) : 0.0f;
                                p.y = point_arr[1].IsDouble() ? static_cast<float>(point_arr[1].GetDouble()) : 0.0f;

                                polygon.push_back(p);
                            }
                        }
                    }
                }
            }
        }
        return true;
    });
}

// RapidJSON DOM parsing only (no population)
void rj_parse_only(int iterations, std::string & json_data)
{
    rapidjson::Document doc;

    benchmark("RapidJSON DOM Parse ONLY", iterations, [&]() {
        std::string copy = json_data;
        doc.ParseInsitu(copy.data());
        if (doc.HasParseError()) {
            std::cerr << std::format("RapidJSON parse error: {}",
                                     rapidjson::GetParseError_En(doc.GetParseError())) <<std::endl;
            return false;
        }
        return true;
    });
}

struct CanadaSAXHandler : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>, CanadaSAXHandler> {
    enum State {
        ROOT, FEATURES_ARRAY, FEATURE_OBJECT, PROPERTIES_OBJECT,
        GEOMETRY_OBJECT, COORDINATES_ARRAY, RING_ARRAY, POINT_ARRAY
    };

    std::array<State, 16> state_stack;
    size_t stack_depth = 0;

    std::array<char, 32> current_key;
    size_t current_key_len = 0;

    size_t totalFeatures = 0;
    size_t totalRings = 0;
    size_t totalPoints = 0;

    std::string error_msg;
    bool error_occurred = false;

    CanadaSAXHandler() {
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

    bool Key(const char* str, rapidjson::SizeType length, bool) {
        current_key_len = std::min<size_t>(length, current_key.size() - 1);
        std::memcpy(current_key.data(), str, current_key_len);
        current_key[current_key_len] = '\0';
        return true;
    }

    bool String(const char* str, rapidjson::SizeType length, bool) {
        std::string_view key(current_key.data(), current_key_len);
        std::string_view value(str, length);

        // Validate type constants
        if (key == "type") {
            if (current_state() == ROOT) {
                if (value != "FeatureCollection") {
                    error_msg = std::format("Expected root type 'FeatureCollection', got '{}'", value);
                    error_occurred = true;
                    return false;
                }
            } else if (current_state() == FEATURE_OBJECT) {
                if (value != "Feature") {
                    error_msg = std::format("Expected feature type 'Feature', got '{}'", value);
                    error_occurred = true;
                    return false;
                }
            } else if (current_state() == GEOMETRY_OBJECT) {
                if (value != "Polygon") {
                    error_msg = std::format("Expected geometry type 'Polygon', got '{}'", value);
                    error_occurred = true;
                    return false;
                }
            }
        }
        return true;
    }

    bool StartObject() {
        std::string_view key(current_key.data(), current_key_len);
        State state = current_state();

        if (state == ROOT) {
            // Root object stays in ROOT state
        } else if (state == FEATURES_ARRAY) {
            push_state(FEATURE_OBJECT);
        } else if (state == FEATURE_OBJECT && key == "properties") {
            push_state(PROPERTIES_OBJECT);
        } else if (state == FEATURE_OBJECT && key == "geometry") {
            push_state(GEOMETRY_OBJECT);
        }
        return true;
    }

    bool EndObject(rapidjson::SizeType) {
        State state = current_state();

        if (state == FEATURE_OBJECT) {
            totalFeatures++;
        }

        if (state != ROOT) {
            pop_state();
        }
        return true;
    }

    bool StartArray() {
        std::string_view key(current_key.data(), current_key_len);
        State state = current_state();

        if (state == ROOT && key == "features") {
            push_state(FEATURES_ARRAY);
        } else if (state == GEOMETRY_OBJECT && key == "coordinates") {
            push_state(COORDINATES_ARRAY);
        } else if (state == COORDINATES_ARRAY) {
            push_state(RING_ARRAY);
        } else if (state == RING_ARRAY) {
            push_state(POINT_ARRAY);
        }
        return true;
    }

    bool EndArray(rapidjson::SizeType) {
        State state = current_state();

        if (state == RING_ARRAY) {
            totalRings++;
        } else if (state == POINT_ARRAY) {
            totalPoints++;
        }

        pop_state();
        return true;
    }

    // Numeric values in POINT_ARRAY are just discarded
    bool Double(double) { return true; }
    bool Int(int) { return true; }
    bool Uint(unsigned) { return true; }
    bool Int64(int64_t) { return true; }
    bool Uint64(uint64_t) { return true; }
    bool Bool(bool) { return true; }
    bool Null() { return true; }
};

void rj_sax_counting(int iterations, std::string &json_data)
// RapidJSON SAX parsing + count objects (no materialization)
{


    benchmark("RapidJSON SAX + count objects", iterations, [&]() {
        std::string copy = json_data;
        CanadaSAXHandler handler;
        rapidjson::Reader reader;
        rapidjson::StringStream ss(copy.data());

        auto result = reader.Parse(ss, handler);

        if (!result || handler.error_occurred) {
            std::cerr << std::format("RapidJSON SAX parse error: {}",
                                                 handler.error_occurred ? handler.error_msg : rapidjson::GetParseError_En(result.Code()));
            return false;
        } else {
            return true;
        }
    });
}
void rj_sax_counting_insitu(int iterations, std::string &json_data) {

    benchmark("RapidJSON SAX + count objects + insitu", iterations, [&]() {
        std::string copy = json_data;
        CanadaSAXHandler handler;
        rapidjson::Reader reader;
        rapidjson::InsituStringStream ss(copy.data());

        auto result = reader.Parse(ss, handler);

        if (!result || handler.error_occurred) {
            std::cerr <<  std::format("RapidJSON SAX parse error: {}",
                                                 handler.error_occurred ? handler.error_msg : rapidjson::GetParseError_En(result.Code()));
            return false;
        } else {
            return true;
        }
    });
}
