
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

#include "twitter_model.hpp"
#include "rapidjson_populate.hpp"

namespace fs = std::filesystem;


using namespace JsonFusion;
using namespace JsonFusion::options;
using namespace JsonFusion::validators;




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
        

        auto printErr = [&json_data](auto res) {
            int pos = res.offset();
            int wnd = 40;
            std::string before(json_data.substr(pos+1 >= wnd ? pos+1-wnd:0, pos+1 >= wnd ? wnd:0));
            std::string after(json_data.substr(pos+1, wnd));
            std::cerr << std::format("JsonFusion parse failed: error {} at {}: '...{}😖{}...'", int(res.error()), pos, before, after)<< std::endl;
        };

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
        
        {
            TwitterData model;
            rapidjson::Document doc;
            
            benchmark("RapidJSON parsing + populating (manual)", iterations, [&]() {
                std::string copy = json_data;
                doc.ParseInsitu(copy.data());
                if (doc.HasParseError()) {
                    throw std::runtime_error(std::format("RapidJSON parse error: {}",
                        rapidjson::GetParseError_En(doc.GetParseError())));
                }
                
                RapidJSONPopulate::populate_twitter_data(model, doc);
            });
        }
        
        {
            TwitterData model;
            benchmark("JsonFusion parsing + populating", iterations, [&]() {
                std::string copy = json_data;


                auto res = Parse(model, copy);
                if (!res) {
                    printErr(res);
                    throw std::runtime_error(std::format("JsonFusion parse error"));
                }
            });
        }

        // Verify what gets parsed
        {
            std::cout << "=== Verification ===\n";

            // RapidJSON
            {
                TwitterData model;
                rapidjson::Document doc;
                std::string copy = json_data;
                doc.ParseInsitu(copy.data());
                RapidJSONPopulate::populate_twitter_data(model, doc);
                
                std::cout << "RapidJSON populated:\n";
                if (model.statuses.has_value()) {
                    std::cout << "  - Statuses count: " << model.statuses->size() << "\n";
                    if (!model.statuses->empty()) {
                        auto& first = model.statuses->front();
                        std::cout << "  - First status text length: " << first.text.size() << "\n";
                        std::cout << "  - User name: " << first.user.name.value_or("(none)") << "\n";

                    }
                }
            }

            // JsonFusion
            {
                TwitterData model;
                std::string copy = json_data;
                auto res = Parse(model, copy);
                if (!res) {
                    printErr(res);
                    throw std::runtime_error("JsonFusion parse error");
                }
                
                std::cout << "\nJsonFusion populated:\n";
                if (model.statuses.has_value()) {
                    std::cout << "  - Statuses count: " << model.statuses->size() << "\n";
                    if (!model.statuses->empty()) {
                        auto& first = model.statuses->front();
                        std::cout << "  - First status text length: " << first.text.size() << "\n";                       
                        std::cout << "  - User name: " << first.user.name.value_or("(none)") << "\n";
                        
                    }
                }
            }
            std::cout << "\n";
        }
        

        std::cout << "\nBenchmark complete.\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}

