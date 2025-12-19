
#include "canada_json_parsing.hpp"
#include <glaze/glaze.hpp>

#include <iostream>
#include <format>

struct GlzCanada {
    std::string type;

    struct Feature {
        std::string type;
        std::map<std::string, std::string> properties;
        struct Geometry {
            std::string type;
            struct Point {
                float x;
                float y;
            };
            std::vector<std::vector<
                std::array<double, 3>
            >> coordinates;
        };
        Geometry geometry;
    };
    std::vector<Feature> features;
};
// RapidJSON DOM parsing + population
void glaze_parse_populate(int iterations, std::string & json_data)
{
    GlzCanada canada;

    benchmark("Glaze Parse + Populate", iterations, [&]() {
        std::string copy = json_data;

        auto error = glz::read_json(canada, copy);
        
        if (error) {
            std::cerr << std::format("Glaze parse error: {}", glz::format_error(error, copy));
            return false;
        }
        return true;
    });
}

