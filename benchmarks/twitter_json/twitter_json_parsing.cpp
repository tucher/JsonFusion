
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

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include "benchmark.hpp"

#include "twitter_model_generic.hpp"
#include "rapidjson_populate.hpp"
#include "reflectcpp_parsing.hpp"





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
        {
            using namespace JsonFusion;
            using namespace JsonFusion::options;
            using TwitterData = TwitterData_T<JsonFusion::A<std::optional<bool>, JsonFusion::options::key<"protected">>>;

            TwitterData model;
            benchmark("JsonFusion parsing + populating", iterations, [&]() {
                std::string copy = json_data;


                auto res = Parse(model, copy.data(), copy.data() + copy.size());
                if (!res) {
                    std::cerr << ParseResultToString<TwitterData>(res, copy.data(), copy.data() + copy.size()) << std::endl;
                    throw std::runtime_error(std::format("JsonFusion parse error"));
                }

            });
        }



        std::cout << "\nBenchmark complete.\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}

