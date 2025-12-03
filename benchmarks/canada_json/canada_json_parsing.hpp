
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <chrono>

#include <JsonFusion/annotated.hpp>
#include <JsonFusion/options.hpp>
#include <JsonFusion/validators.hpp>




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
    A<float, skip_json<>> x;
    A<float, skip_json<>> y;
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



template<typename Func>
double benchmark(const std::string& label, int iterations, Func&& func) {
    using clock = std::chrono::steady_clock;

    // Warm-up
    for (int i = 0; i < 3; ++i) {
        func();
    }

    auto start = clock::now();
    for (int i = 0; i < iterations; ++i) {
        if(!func()) break;;
    }
    auto end = clock::now();

    auto total_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    double avg_us = static_cast<double>(total_us) / iterations;

    std::cout << std::format("{:<70} {:>8.2f} µs/iter  ({} iterations)\n",
                             label, avg_us, iterations);

    return avg_us;
}

void rj_parse_populate(int iterations, std::string json_data);
void rj_parse_only(int iterations, std::string json_data);
void rj_sax_counting(int iterations, std::string json_data);
void rj_sax_counting_insitu(int iterations, std::string json_data);
