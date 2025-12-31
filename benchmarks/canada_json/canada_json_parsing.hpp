
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <chrono>

#include <JsonFusion/annotated.hpp>
#include <JsonFusion/options.hpp>
#include <JsonFusion/validators.hpp>
#include <JsonFusion/generic_streamer.hpp>



using namespace JsonFusion;
using namespace JsonFusion::options;
using namespace JsonFusion::validators;



struct Canada {
    A<std::string,  string_constant<"FeatureCollection"> > type;

    struct Feature {
        A<std::string, string_constant<"Feature"> > type;
        std::map<std::string, std::string> properties;
        struct Geometry {
            A<std::string, string_constant<"Polygon"> > type;
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
    A<float, skip<>> x;
    A<float, skip<>> y;
};


template<typename PT>
using PointAsArray = Annotated<PT, as_array>;


template<typename PT>
using RingConsumer = streamers::LambdaStreamer<[](Stats * stats, const PointAsArray<PT>&) {
    stats->totalPoints ++;
    return true;
}>;

template<typename PT>
using RingsConsumer = streamers::LambdaStreamer<[](Stats * stats, const RingConsumer<PT>&) {
    stats->totalRings ++;
    return true;
}>;

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
using FeatureConsumer = streamers::LambdaStreamer<[](Stats * stats, const Feature<PT>&) {
    stats->totalFeatures ++;
    return true;
}>;


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

    std::cout << std::format("{:<70} {:>8.0f}\n",
                             label, avg_us, iterations);

    return avg_us;
}

void rj_parse_populate(int iterations, std::string & json_data);
void rj_parse_only(int iterations, std::string & json_data);
void rj_sax_counting(int iterations, std::string & json_data);
void rj_sax_counting_insitu(int iterations, std::string & json_data);
void glaze_parse_populate(int iterations, std::string &json_data);
