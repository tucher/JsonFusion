// Canada.json parsing benchmark
// Tests raw parsing speed on a large real-world JSON file
// Download canada.json from: https://github.com/miloyip/nativejson-benchmark/blob/master/data/canada.json


#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <vector>
#include <format>
#include <filesystem>
#include <map>

#include <JsonFusion/parser.hpp>
#include <JsonFusion/error_formatting.hpp>

#include "canada_json_parsing.hpp"

namespace fs = std::filesystem;


using namespace JsonFusion;
using namespace JsonFusion::options;
using namespace JsonFusion::validators;




std::string read_file(const fs::path& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) {
        abort();
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::string buffer(size, '\0');
    if (!file.read(buffer.data(), size)) {
        abort();
    }
    
    return buffer;
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path-to-canada.json>\n";
        std::cerr << "Download from: https://github.com/miloyip/nativejson-benchmark/blob/master/data/canada.json\n";
        return 1;
    }
    
    fs::path json_path = argv[1];
    
        std::cout << "Reading file: " << json_path << "\n";
        std::string json_data = read_file(json_path);
        std::cout << std::format("File size: {:.2f} MB ({} bytes)\n\n", 
                                json_data.size() / (1024.0 * 1024.0), 
                                json_data.size());
        
        const int iterations = 1000;
        
        std::cout << "=== Canada.json Parsing Benchmark ===\n\n";
        
        // RapidJSON DOM parsing + population

        rj_parse_only(iterations, json_data);
        rj_parse_populate(iterations, json_data);
        // rj_sax_counting_insitu(iterations, json_data);

        // JsonFusion typed parsing
        {
            benchmark("JsonFusion Parse + Populate", iterations, [&]() {
                std::string copy = json_data;
                Canada canada;
                auto res = Parse(canada, copy.data(), copy.data() + copy.size());
                if (!res) {
                    std::cerr << ParseResultToString<Canada>(res, copy.data(), copy.data() + copy.size()) << std::endl;
                    return false;
                } else {
                    return true;
                }
            });
        }
        rj_sax_counting(iterations, json_data);


        benchmark("JsonFusion Stream + count objects", iterations, [&]() {
            std::string copy = json_data;

            Stats stats;
            CanadaStatsCounter<Point> canada;
            canada.features.set_json_fusion_context(&stats);


            auto res = Parse(canada, copy.data(), copy.data() + copy.size(), &stats);
            if (!res) {
                std::cerr << ParseResultToString<CanadaStatsCounter<Point>>(res, copy.data(), copy.data() + copy.size()) << std::endl;
                return false;
            } else {
                return true;
            }
        });


        benchmark("JsonFusion Stream + count objects + skip unneeded parsing", iterations, [&]() {
            std::string copy = json_data;

            Stats stats;
            CanadaStatsCounter<PointSkippedXY> canada;
            canada.features.set_json_fusion_context(&stats);


            auto res = Parse(canada, copy.data(), copy.data() + copy.size(), &stats);
            if (!res) {
                std::cerr << ParseResultToString<CanadaStatsCounter<Point>>(res, copy.data(), copy.data() + copy.size()) << std::endl;
                return false;
            } else {
                return true;
            }
        });

/*
        benchmark("JsonFusion Stream + count objects + numbers-tokenizing-only", iterations, [&]() {
            std::string copy = json_data;

            Stats stats;
            CanadaStatsCounter<PointUnmaterializedXY> canada;
            canada.features.set_json_fusion_context(&stats);


            auto res = Parse(canada, copy, &stats);
            if (!res) {
                throw std::runtime_error(std::format("JsonFusion parse error"));
            }
        });
*/
        
        std::cout << "\nBenchmark complete.\n";
        

    
    return 0;
}

