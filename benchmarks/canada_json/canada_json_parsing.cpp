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
#include <JsonFusion/yyjson.hpp>
#include <JsonFusion/cbor.hpp>
#include <JsonFusion/serializer.hpp>

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

    const int iterations = 100;

    std::cout << std::format("=== Canada.json Benchmark ===({} iterations, µs/iter)\n\n", iterations);

    // RapidJSON DOM parsing + population
    glaze_parse_populate(iterations, json_data);
    rj_parse_only(iterations, json_data);
    rj_parse_populate(iterations, json_data);
    rj_sax_counting(iterations, json_data);

    // rj_sax_counting_insitu(iterations, json_data);

    // JsonFusion typed parsing
    Canada canadaPopulated;
    {
        Canada & canada = canadaPopulated;
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


    std::string cbor_out;
    using JsonFusion::io_details::limitless_sentinel;
    cbor_out.clear();
    auto it  = std::back_inserter(cbor_out);
    limitless_sentinel end{};
    if(auto res = JsonFusion::SerializeWithWriter(canadaPopulated, JsonFusion::CborWriter(it, end)); !res) {
        std::cerr << std::format("JsonFusion CBOR serialize error") << std::endl;
        return 1;
    }
    // std::cout << "CBOR size: " << cbor_out.size() << std::endl;

    CanadaStatsCounter<Point> canada;
    Stats refStats;
    benchmark("JsonFusion Stream + count objects", iterations, [&]() {
        std::string copy = json_data;


        canada.features.set_jsonfusion_context(&refStats);


        auto res = Parse(canada, copy.data(), copy.data() + copy.size(), &refStats);
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

        canadaSkip.features.set_jsonfusion_context(&stats);


        auto res = Parse(canadaSkip, copy.data(), copy.data() + copy.size(), &stats);
        if (!res) {
            std::cerr << ParseResultToString<CanadaStatsCounter<Point>>(res, copy.data(), copy.data() + copy.size()) << std::endl;
            return false;
        } else {
            return true;
        }
    });


    {
        Canada  canada;
        benchmark("JsonFusion Parse + Populate (yyjson backend)", iterations, [&]() {
            std::string copy = json_data;

            auto res = ParseWithReader(canada, YyjsonReader(copy.data(), copy.size()));
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

            canada.features.set_jsonfusion_context(&stats);

            auto res = ParseWithReader(canada, YyjsonReader(copy.data(), copy.size()), &stats);

            if (!res) {
                std::cerr << ParseResultToString<CanadaStatsCounter<Point>>(res, copy.data(), copy.data() + copy.size()) << std::endl;
                return false;
            } else {
                return true;
            }
        });
    }


    {
        Canada modelFromCBOR;
        benchmark("JsonFusion CBOR parsing", iterations, [&]() {
            std::string copy = cbor_out;

            std::uint8_t * b = reinterpret_cast<std::uint8_t *>(copy.data());
            auto res = JsonFusion::ParseWithReader(modelFromCBOR, JsonFusion::CborReader(b, b + copy.size()));
            if (!res) {
                std::cerr << ParseResultToString<Canada>(res, copy.data(), copy.data() + copy.size()) << std::endl;
                return false;
            } else {
                return true;
            }

        });

        if(canadaPopulated.features.back().geometry.coordinates.size() != modelFromCBOR.features.back().geometry.coordinates.size()) {
            std::cout << std::format("Data mismatch") << std::endl;
        }
    }
    {
        CanadaStatsCounter<Point> canada;
        Stats stats;
        benchmark("JsonFusion CBOR Stream", iterations, [&]() {
            std::string copy = cbor_out;



            canada.features.set_jsonfusion_context(&stats);

            std::uint8_t * b = reinterpret_cast<std::uint8_t *>(copy.data());
            auto res = ParseWithReader(canada, JsonFusion::CborReader(b, b + copy.size()), &stats);

            if (!res) {
                std::cerr << ParseResultToString<CanadaStatsCounter<Point>>(res, copy.data(), copy.data() + copy.size()) << std::endl;
                return false;
            } else {
                return true;
            }
        });
        if(stats.totalPoints != refStats.totalPoints) {
            std::cout << std::format("error: stats.totalPoints {} refStats.totalPoints {}",stats.totalPoints, refStats.totalPoints) << std::endl;
        }
    }

    std::cout << "\n--Serialization--\n";

    std::string serialize_buffer;
    serialize_buffer.resize(10000000);
    {
        std::size_t final_size = 0;
        benchmark("JsonFusion serializing(yyjson backend)", iterations, [&]() {
            auto res = JsonFusion::SerializeWithWriter(canadaPopulated, YyjsonWriter(serialize_buffer));
            if( !res) {
                std::cerr << std::format("JsonFusion serialize error") << std::endl;
                return false;
            } else {
                return true;
            }
            return false;
        });
        // std::cout << "JsonFusion+yyjson serialized size: " << final_size << std::endl;
    }

    {
        std::size_t final_size = 0;

        benchmark("JsonFusion serializing", iterations, [&]() {
            char *d = serialize_buffer.data();
            if(auto res = JsonFusion::Serialize(canadaPopulated, d, d + serialize_buffer.size()); !res) {
                std::cerr << std::format("JsonFusion serialize error") << std::endl;
                return false;
            } else {
                final_size = d - serialize_buffer.data();
                return true;
            }

        });
        // std::cout << "JsonFusion serialized size: " << final_size << std::endl;

    }
    {
        std::size_t final_sz;
        benchmark("JsonFusion CBOR serializing", iterations, [&]() {
            std::uint8_t * b = reinterpret_cast<std::uint8_t *>(serialize_buffer.data());
            if(auto res = JsonFusion::SerializeWithWriter(canadaPopulated, JsonFusion::CborWriter(b, b + serialize_buffer.size())); !res) {
                std::cerr << std::format("JsonFusion CBOR serialize error") << std::endl;
                return false;
            } else {
                final_sz = res.bytesWritten();
                return true;
            }

        });
        if(final_sz != cbor_out.size()) {
            std::cerr << "Something is wrong" << std::endl;
        }
    }


    std::cout << "\nBenchmark complete.\n\n\n";



    return 0;
}

