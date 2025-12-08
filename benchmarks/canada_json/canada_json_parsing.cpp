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
#include <JsonFusion/yyjson_reader.hpp>

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
            Canada canada;
            benchmark("JsonFusion Parse + Populate", iterations, [&]() {
                std::string copy = json_data;

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


        CanadaStatsCounter<Point> canada;
        benchmark("JsonFusion Stream + count objects", iterations, [&]() {
            std::string copy = json_data;

            Stats stats;

            canada.features.set_json_fusion_context(&stats);


            auto res = Parse(canada, copy.data(), copy.data() + copy.size(), &stats);
            if (!res) {
                std::cerr << ParseResultToString<CanadaStatsCounter<Point>>(res, copy.data(), copy.data() + copy.size()) << std::endl;
                return false;
            } else {
                return true;
            }
        });

        CanadaStatsCounter<PointSkippedXY> canadaSkip;

        benchmark("JsonFusion Stream + count objects + skip unneeded parsing", iterations, [&]() {
            std::string copy = json_data;

            Stats stats;

            canadaSkip.features.set_json_fusion_context(&stats);


            auto res = Parse(canadaSkip, copy.data(), copy.data() + copy.size(), &stats);
            if (!res) {
                std::cerr << ParseResultToString<CanadaStatsCounter<Point>>(res, copy.data(), copy.data() + copy.size()) << std::endl;
                return false;
            } else {
                return true;
            }
        });

        {
            Canada canada;
            benchmark("JsonFusion Parse + Populate (yyjson backend)", iterations, [&]() {
                std::string copy = json_data;

                yyjson_read_err err;
                yyjson_doc* doc = yyjson_read_opts(const_cast<char*>(copy.data()),
                                                   copy.size(), 0, NULL, &err);
                if (!doc) {
                    return false;
                }
                yyjson_val* root = yyjson_doc_get_root(doc);
                YyjsonReader reader(root);

                auto res = ParseWithReader(canada, reader);
                yyjson_doc_free(doc);
                if (!res) {
                    std::cerr << ParseResultToString<Canada>(res, copy.data(), copy.data() + copy.size()) << std::endl;
                    return false;
                } else {
                    return true;
                }
            });
        }

        {
            CanadaStatsCounter<Point> canada;
            benchmark("JsonFusion Stream + count objects (yyjson backend)", iterations, [&]() {
                std::string copy = json_data;

                Stats stats;

                canada.features.set_json_fusion_context(&stats);

                yyjson_read_err err;
                yyjson_doc* doc = yyjson_read_opts(const_cast<char*>(copy.data()),
                                                   copy.size(), 0, NULL, &err);
                if (!doc) {
                    return false;
                }
                yyjson_val* root = yyjson_doc_get_root(doc);
                YyjsonReader reader(root);
                auto res = ParseWithReader(canada, reader, &stats);
                yyjson_doc_free(doc);
                if (!res) {
                    std::cerr << ParseResultToString<CanadaStatsCounter<Point>>(res, copy.data(), copy.data() + copy.size()) << std::endl;
                    return false;
                } else {
                    return true;
                }
            });
        }


        std::cout << "\nBenchmark complete.\n";



    return 0;
}

