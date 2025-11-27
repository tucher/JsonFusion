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
        int pos = res.offset();
        int wnd = 20;
        std::string before(json_fusion_test_models::kJsonStatic.substr(pos+1 >= wnd ? pos+1-wnd:0, pos+1 >= wnd ? wnd:0));
        std::string after(json_fusion_test_models::kJsonStatic.substr(pos+1, wnd));
        std::cerr << std::format("JsonFusion parse failed: error {} at {}: '...{}😖{}...'", int(res.error()), pos, before, after)<< std::endl;
    };

    std::cout << std::format("iterations: {}",  iterations) << std::endl;
    auto runner = [&](std::string_view funcName, auto&& func) {
        auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < iterations; ++i) {
            if(!func()) {
                return false;
            }
        }
        auto end = std::chrono::steady_clock::now();
        auto total = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        std::cout << std::format("{}: total us, avg {:.2f} us/parse", funcName, double(total) / iterations) << std::endl;   
        return true;
    };

    {
        /*
            JsonFusion benchmark:

            - The data model uses only fixed-size containers (std::array and fixed-size char buffers),
            so parsing does not perform any dynamic allocations.

            - JsonFusion parses, type-checks and validates the JSON in a single pass:
                * C++ types (int, double, std::array<>, etc.) enforce structural and type constraints.
                * Annotations (range<>, min_items<>, etc.) enforce additional semantic constraints.

            - The parser only performs three operations on the input iterator: ++, *, and !=.
            There are no rewinds or extra passes over the input.

            - On success we end up with a fully filled and validated ComplexConfig 'cfg'.
            On error, the result object carries the first error position so we can report it.
        */
        
        json_fusion_test_models::static_model::ComplexConfig cfg{};

        if(!runner("JsonFusion (static containers)", [&]() {
            std::string buf = std::string(json_fusion_test_models::kJsonStatic);
            auto res = JsonFusion::Parse(cfg, buf);
            if(!res) {
                printErr(res);
                return false;
            } return true;
        })) {
            return 1;
        };
        assert(cfg.controller.motors[1].inverted == true);
    }

    {
        json_fusion_test_models::dynamic_model::ComplexConfig cfg{};

        if(!runner("JsonFusion (dynamic containers)", [&]() {
            std::string buf = std::string(json_fusion_test_models::kJsonStatic);
            auto res = JsonFusion::Parse(cfg, buf);
            if(!res) {
                printErr(res);
                return false;
            } return true;
        })) {
            return 1;
        };
        assert(cfg.controller.motors->front().id == 1);
    }

    {
        /*
            RapidJSON benchmark:

            - Measures "parsing only" into a DOM (rapidjson::Document), without any mapping
            into a C++ config struct, and without additional semantic validation.

            - To use ParseInsitu(), we need a mutable buffer, so each iteration:
                * Copies the JSON into a temporary std::string.
                * Parses in-place from that mutable buffer.

            This means the RapidJSON timing includes the cost of copying the JSON string
            on every iteration, in addition to DOM construction.
        */
        rj::Document doc;
        if(!runner("RapidJSON 'insitu' parsing only, without mapping", [&]() {
            std::string buf = std::string(json_fusion_test_models::kJsonStatic);
            doc.ParseInsitu(buf.data()); 
            if (doc.HasParseError()) {
                return false;
            }
            return true;
        })) {
            return 1;
        }
    }
    {
        /*
            RapidJSON benchmark:

            - Measures "parsing only" into a DOM (rapidjson::Document), without any mapping
            into a C++ config struct, and without additional semantic validation.


            This means the RapidJSON timing includes the cost of copying the JSON string
            on every iteration, in addition to DOM construction.
        */
        rj::Document doc;
        std::string buf = std::string(json_fusion_test_models::kJsonStatic);
        if(!runner("RapidJSON non-insitu parsing only, without mapping", [&]() {
            doc.Parse(buf.data(), buf.size());
            if (doc.HasParseError()) {
                return false;
            }
            return true;
        })) {
            return 1;
        }
    }


    return 0;
}
