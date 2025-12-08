
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <vector>
#include <format>
#include <filesystem>

#include <JsonFusion/parser.hpp>
#include <JsonFusion/error_formatting.hpp>
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/yyjson_reader.hpp>
#include <JsonFusion/generic_streamer.hpp>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include "benchmark.hpp"

#include "twitter_model_generic.hpp"
#include "rapidjson_populate.hpp"
#include "reflectcpp_parsing.hpp"
#include "glaze_parsing.hpp"
#include "yyjson_parsing.hpp"

void print_diff_region(const std::string& a,
                       const std::string& b,
                       std::size_t context = 40)
{
    const std::size_t min_size = std::min(a.size(), b.size());

    std::size_t pos = 0;
    for (; pos < min_size; ++pos) {
        if (a[pos] != b[pos]) break;
    }

    if (pos == min_size) {
        // No differing byte in common prefix – length mismatch
        if (a.size() == b.size()) {
            std::cerr << "Strings are identical.\n";
        } else {
            std::cerr << std::format(
                "Strings differ in length only: native size = {}, yyjson size = {}, "
                "common prefix length = {}\n",
                a.size(), b.size(), min_size);
        }
        return;
    }

    // We found a real difference at index `pos`
    const std::size_t start = (pos > context) ? pos - context : 0;
    const std::size_t end_a = std::min(a.size(), pos + context + 1);
    const std::size_t end_b = std::min(b.size(), pos + context + 1);

    const std::string_view slice_a(a.data() + start, end_a - start);
    const std::string_view slice_b(b.data() + start, end_b - start);

    const std::size_t caret_pos = pos - start;

    std::string caret_line(caret_pos, ' ');
    caret_line.push_back('^');

    std::cerr << std::format("First difference at index {}:\n", pos);
    std::cerr << std::format("native_out[{}] = {} ('{}')\n",
                             pos,
                             static_cast<int>(static_cast<unsigned char>(a[pos])),
                             a[pos]);
    std::cerr << std::format("yyjson_out[{}] = {} ('{}')\n",
                             pos,
                             static_cast<int>(static_cast<unsigned char>(b[pos])),
                             b[pos]);
    std::cerr << "\nRegion around mismatch:\n";
    std::cerr << std::format("native: \"{}\"\n", slice_a);
    std::cerr << std::format("yyjson: \"{}\"\n", slice_b);
    std::cerr << "         " << caret_line << "\n";
}



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



int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path-to-twitter.json>\n";
        return 1;
    }
    
    fs::path json_path = argv[1];
    
    try {
        std::cout << "Reading file: " << json_path << "\n";
        std::string json_data = read_file(json_path);
        std::cout << std::format("File size: {:.2f} MB ({} bytes)\n\n", 
                                json_data.size() / (1024.0 * 1024.0), 
                                json_data.size());
        
        const int iterations = 10000;
        
        std::cout << "=== twitter.json Parsing Benchmark ===\n\n";
        

        rj_parse_only(iterations, json_data);
        rj_parse_populate(iterations, json_data);
        reflectcpp_parse_populate(iterations, json_data);
        glaze_parse_populate(iterations, json_data);
        yyjson_parse(iterations, json_data);
        {
            using namespace JsonFusion;
            using namespace JsonFusion::options;
            using TwitterData = TwitterData_T<A<std::optional<bool>, options::key<"protected">>>;

            TwitterData model;
            benchmark("JsonFusion parsing + populating", iterations, [&]() {
                std::string copy = json_data;


                auto res = Parse(model, copy.data(), copy.data() + copy.size());
                if (!res) {
                    std::cerr << ParseResultToString<TwitterData>(res, copy.data(), copy.data() + copy.size()) << std::endl;
                    throw std::runtime_error(std::format("JsonFusion parse error"));
                }

            });
            std::string native_out;
            Serialize(model, native_out);

            benchmark("JsonFusion parsing + populating (yyjson backend)", iterations, [&]() {
                std::string copy = json_data;

                yyjson_read_err err;
                yyjson_doc* doc = yyjson_read_opts(const_cast<char*>(copy.data()),
                                                   copy.size(), 0, NULL, &err);
                if (!doc) {
                    throw std::string(err.msg);
                }
                yyjson_val* root = yyjson_doc_get_root(doc);
                YyjsonReader reader(root);

                auto res = JsonFusion::ParseWithReader(model, reader);
                yyjson_doc_free(doc);
                if (!res) {
                    std::cerr << ParseResultToString<TwitterData>(res, copy.data(), copy.data() + copy.size()) << std::endl;
                    throw std::runtime_error(std::format("JsonFusion parse error"));
                }

            });
            std::string yyjson_out;
            Serialize(model, yyjson_out);

            if(native_out != yyjson_out) {
                print_diff_region(native_out, yyjson_out, 60);
                throw std::runtime_error(std::format("yyjson backed parsing output does not match native parsing one"));
            }

            using TwitterDataStream = TwitterData_T<
                A<std::optional<bool>, options::key<"protected">>,
                streamers::CountingStreamer
            >;
            TwitterDataStream streamModel;

            benchmark("JsonFusion streaming (yyjson backend)", iterations, [&]() {
                std::string copy = json_data;

                yyjson_read_err err;
                yyjson_doc* doc = yyjson_read_opts(const_cast<char*>(copy.data()),
                                                   copy.size(), 0, NULL, &err);
                if (!doc) {
                    throw std::string(err.msg);
                }
                yyjson_val* root = yyjson_doc_get_root(doc);
                YyjsonReader reader(root);

                auto res = JsonFusion::ParseWithReader(streamModel, reader);
                yyjson_doc_free(doc);
                if (!res) {
                    std::cerr << ParseResultToString<TwitterDataStream>(res, copy.data(), copy.data() + copy.size()) << std::endl;
                    throw std::runtime_error(std::format("JsonFusion parse error"));
                }

            });
            std::cout << "Statuses count: " << streamModel.statuses->counter << std::endl;

        }



        std::cout << "\nBenchmark complete.\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}

