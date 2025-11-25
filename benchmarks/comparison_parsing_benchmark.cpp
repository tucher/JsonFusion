#include <iostream>
#include <format>
#include <cassert>
#include <chrono>
#include <JsonFusion/parser.hpp>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <rapidjson/reader.h>
#include <rapidjson/memorystream.h>
#include <rapidjson/error/en.h>


#include "../tests/test_model.hpp"

namespace rj = rapidjson;




int main() {
    constexpr int iterations = 100000;

    std::ios::sync_with_stdio(false);



    using clock = std::chrono::steady_clock;

    auto printErr = [](auto res) {
        int pos = res.pos() - json_fusion_test_models::kJsonStatic.data();
        int wnd = 20;
        std::string before(json_fusion_test_models::kJsonStatic.substr(pos+1 >= wnd ? pos+1-wnd:0, pos+1 >= wnd ? wnd:0));
        std::string after(json_fusion_test_models::kJsonStatic.substr(pos+1, wnd));
        std::cerr << std::format("JsonFusion parse failed: error {} at {}: '...{}😖{}...'", int(res.error()), pos, before, after)<< std::endl;
    };

    std::cout << std::format("iterations: {}",  iterations) << std::endl;
    {
        json_fusion_test_models::static_model::ComplexConfig cfg{};
        auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < iterations; ++i) {
            auto res = JsonFusion::Parse(cfg, json_fusion_test_models::kJsonStatic);
            if(!res) {
                printErr(res);
                return 1;
            }
        }
        assert(cfg.controller.motors[1].inverted == true);
        
        auto end   = std::chrono::steady_clock::now();
        auto total = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        std::cout << "JsonFusion (static containers): total "
                  << total << " us, avg " << double(total) / iterations << " us/parse\n";
    }

    {
        json_fusion_test_models::dynamic_model::ComplexConfig cfg{};
        auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < iterations; ++i) {
            auto res = JsonFusion::Parse(cfg, json_fusion_test_models::kJsonStatic);
            if (!res) {
                printErr(res);
                return 1;
            }
        }
        assert(cfg.controller.motors->front().id == 1);
        auto end   = std::chrono::steady_clock::now();
        auto total = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        std::cout << "JsonFusion (dynamic containers): total "
                  << total << " us, avg " << double(total) / iterations << " us/parse\n";
    }

    {
        rj::Document doc;
        std::string buf = std::string(json_fusion_test_models::kJsonStatic);
        auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < iterations; ++i) {
            std::string tmp = buf;   
            // doc.Parse(json_fusion_test_models::kJsonStatic.data(), json_fusion_test_models::kJsonStatic.size());
            doc.ParseInsitu(tmp.data()); 
            if (doc.HasParseError()) {
                std::cerr << "RapidJSON parse error: "
                          << rj::GetParseError_En(doc.GetParseError())
                          << " at offset " << doc.GetErrorOffset() << "\n";
                return 1;
            }
        }
        auto end   = std::chrono::steady_clock::now();
        auto total = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        std::cout << "RapidJSON parsing only, without mapping: total "
                  << total << " us, avg " << double(total) / iterations << " us/parse\n";
    }


    return 0;
}
