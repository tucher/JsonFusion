// Canada.json parsing benchmark
// Tests raw parsing speed on a large real-world JSON file
// Download canada.json from: https://github.com/miloyip/nativejson-benchmark/blob/master/data/canada.json

/*
RapidJSON DOM Parse + Populate            5595.24 µs/iter  (1000 iterations)
RapidJSON DOM Parse ONLY                  5231.22 µs/iter  (1000 iterations)
JsonFusion Parse + Populate               5336.94 µs/iter  (1000 iterations)
RapidJSON SAX + count objects             3761.07 µs/iter  (1000 iterations)
RapidJSON SAX + count objects + insitu    4372.01 µs/iter  (1000 iterations)
JsonFusion Stream + count objects         5134.85 µs/iter  (1000 iterations)

*/
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <vector>
#include <format>
#include <filesystem>

#include <JsonFusion/parser.hpp>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

namespace fs = std::filesystem;


using namespace JsonFusion;
using namespace JsonFusion::options;
using namespace JsonFusion::validators;



struct Canada {
    std::string type;

    struct Feature {
        std::map<std::string, std::string> properties;
        std::string type;
        struct Geometry {
            std::string type;
            struct Point {
                float x;
                float y;
            };
            using PointAsArray = Annotated<Point, as_array>;
            std::vector<std::vector<
                PointAsArray
            >> coordinates;
        };
        Geometry geometry;
    };
    std::vector<Feature> features;
};


struct Stats {
    std::size_t totalPoints = 0;
    std::size_t totalRings = 0;
    std::size_t totalFeatures = 0;
};

struct Point {
    float x;
    float y;
};

struct PointSkippedXY {
    A<float, skip_json> x;
    A<float, skip_json> y;
};

struct PointUnmaterializedXY {
    A<float, skip_materializing> x;
    A<float, skip_materializing> y;
};

template<typename PT>
using PointAsArray = Annotated<PT, as_array>;


template<typename PT>
struct RingConsumer {
    using value_type = PointAsArray<PT>;

    bool finalize(bool success)  { return true; }

    Stats * stats = nullptr;
    void reset()  { }
    bool consume(const PT & r)  {
        stats->totalPoints ++;
        return true;
    }
    void set_json_fusion_context(Stats * ctx) {
        stats = ctx;
    }
};

template<typename PT>
struct RingsConsumer {
    using value_type = RingConsumer<PT>;

    bool finalize(bool success)  { return true; }

    Stats * stats = nullptr;
    void reset()  { }

    bool consume(const RingConsumer<PT> & ringConsumer)  {
        stats->totalRings ++;
        return true;
    }
    void set_json_fusion_context(Stats * ctx) {
        stats = ctx;
    }
};

template<typename PT>
struct Feature {
    A<std::string, key<"type">, string_constant<"Feature"> > _;
    std::map<std::string, std::string> properties;

    struct PolygonGeometry {
        A<std::string, key<"type">, string_constant<"Polygon"> > _;
        A<RingsConsumer<PT>, key<"coordinates">> rings;
    };
    PolygonGeometry geometry;
};

template<typename PT>
struct FeatureConsumer {
    using value_type = Feature<PT>;
    bool finalize(bool success)  { return true; }

    Stats * stats = nullptr;
    void reset()  {  }
    bool consume(const Feature<PT> & f)  {
        stats->totalFeatures ++;
        return true;
    }
    void set_json_fusion_context(Stats * ctx) {
        stats = ctx;
    }
};

template<typename PT>
struct CanadaStatsCounter {
    A<std::string, key<"type">, string_constant<"FeatureCollection"> > _;
    FeatureConsumer<PT> features;
};

std::string read_file(const fs::path& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error(std::format("Failed to open file: {}", filepath.string()));
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::string buffer(size, '\0');
    if (!file.read(buffer.data(), size)) {
        throw std::runtime_error(std::format("Failed to read file: {}", filepath.string()));
    }
    
    return buffer;
}

template<typename Func>
double benchmark(const std::string& label, int iterations, Func&& func) {
    using clock = std::chrono::steady_clock;
    
    // Warm-up
    for (int i = 0; i < 3; ++i) {
        func();
    }
    
    auto start = clock::now();
    for (int i = 0; i < iterations; ++i) {
        func();
    }
    auto end = clock::now();
    
    auto total_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    double avg_us = static_cast<double>(total_us) / iterations;
    
    std::cout << std::format("{:<70} {:>8.2f} µs/iter  ({} iterations)\n",
                            label, avg_us, iterations);
    
    return avg_us;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path-to-canada.json>\n";
        std::cerr << "Download from: https://github.com/miloyip/nativejson-benchmark/blob/master/data/canada.json\n";
        return 1;
    }
    
    fs::path json_path = argv[1];
    
    try {
        std::cout << "Reading file: " << json_path << "\n";
        std::string json_data = read_file(json_path);
        std::cout << std::format("File size: {:.2f} MB ({} bytes)\n\n", 
                                json_data.size() / (1024.0 * 1024.0), 
                                json_data.size());
        
        const int iterations = 1000;
        
        std::cout << "=== Canada.json Parsing Benchmark ===\n\n";
        
        // RapidJSON DOM parsing + population
        {
            rapidjson::Document doc;
            Canada canada;
            
            benchmark("RapidJSON DOM Parse + Populate", iterations, [&]() {
                std::string copy = json_data;
                doc.ParseInsitu(copy.data());
                if (doc.HasParseError()) {
                    throw std::runtime_error(std::format("RapidJSON parse error: {}", 
                        rapidjson::GetParseError_En(doc.GetParseError())));
                }
                
                // Populate Canada structure from DOM
                canada.features.clear();
                
                if (!doc.IsObject()) return;
                
                // Parse type
                if (auto it = doc.FindMember("type"); it != doc.MemberEnd() && it->value.IsString()) {
                    canada.type.assign(it->value.GetString(), it->value.GetStringLength());
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
                            feature.type.assign(type_it->value.GetString(), type_it->value.GetStringLength());
                        }
                        
                        // Parse geometry
                        if (auto geom_it = feature_obj.FindMember("geometry"); geom_it != feature_obj.MemberEnd() && geom_it->value.IsObject()) {
                            const auto& geom_obj = geom_it->value;
                            
                            // Parse geometry type
                            if (auto gtype_it = geom_obj.FindMember("type"); gtype_it != geom_obj.MemberEnd() && gtype_it->value.IsString()) {
                                feature.geometry.type.assign(gtype_it->value.GetString(), gtype_it->value.GetStringLength());
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
            });
        }
        
        // RapidJSON DOM parsing only (no population)
        {
            rapidjson::Document doc;
            
            benchmark("RapidJSON DOM Parse ONLY", iterations, [&]() {
                std::string copy = json_data;
                doc.ParseInsitu(copy.data());
                if (doc.HasParseError()) {
                    throw std::runtime_error(std::format("RapidJSON parse error: {}", 
                        rapidjson::GetParseError_En(doc.GetParseError())));
                }
            });
        }
        
        // JsonFusion typed parsing
        {
            benchmark("JsonFusion Parse + Populate", iterations, [&]() {
                std::string copy = json_data;
                Canada canada;
                auto res = Parse(canada, copy);
                if (!res) {
                    throw std::runtime_error(std::format("JsonFusion parse error"));
                }
            });
        }
        
        // RapidJSON SAX parsing + count objects (no materialization)
        {
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
            
            benchmark("RapidJSON SAX + count objects", iterations, [&]() {
                std::string copy = json_data;
                CanadaSAXHandler handler;
                rapidjson::Reader reader;
                rapidjson::StringStream ss(copy.data());
                
                auto result = reader.Parse(ss, handler);
                
                if (!result || handler.error_occurred) {
                    throw std::runtime_error(std::format("RapidJSON SAX parse error: {}", 
                        handler.error_occurred ? handler.error_msg : rapidjson::GetParseError_En(result.Code())));
                }
            });
            
            benchmark("RapidJSON SAX + count objects + insitu", iterations, [&]() {
                std::string copy = json_data;
                CanadaSAXHandler handler;
                rapidjson::Reader reader;
                rapidjson::InsituStringStream ss(copy.data());
                
                auto result = reader.Parse(ss, handler);
                
                if (!result || handler.error_occurred) {
                    throw std::runtime_error(std::format("RapidJSON SAX parse error: {}", 
                        handler.error_occurred ? handler.error_msg : rapidjson::GetParseError_En(result.Code())));
                }
            });
        }
       

        benchmark("JsonFusion Stream + count objects", iterations, [&]() {
            std::string copy = json_data;

            Stats stats;
            CanadaStatsCounter<Point> canada;
            canada.features.set_json_fusion_context(&stats);


            auto res = ParseWithContext(canada, copy, &stats);
            if (!res) {
                throw std::runtime_error(std::format("JsonFusion parse error"));
            }
        });


        benchmark("JsonFusion Stream + count objects + skip unneeded parsing", iterations, [&]() {
            std::string copy = json_data;

            Stats stats;
            CanadaStatsCounter<PointSkippedXY> canada;
            canada.features.set_json_fusion_context(&stats);


            auto res = ParseWithContext(canada, copy, &stats);
            if (!res) {
                throw std::runtime_error(std::format("JsonFusion parse error"));
            }
        });


        benchmark("JsonFusion Stream + count objects + numbers-tokenizing-only", iterations, [&]() {
            std::string copy = json_data;

            Stats stats;
            CanadaStatsCounter<PointUnmaterializedXY> canada;
            canada.features.set_json_fusion_context(&stats);


            auto res = ParseWithContext(canada, copy, &stats);
            if (!res) {
                throw std::runtime_error(std::format("JsonFusion parse error"));
            }
        });
        
        std::cout << "\nBenchmark complete.\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}

