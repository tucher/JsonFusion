#include <string>
#include <format>
#include <stdexcept>

#include <rfl.hpp>
#include <rfl/json.hpp>

#include "twitter_model_generic.hpp"
#include "benchmark.hpp"

// reflect-cpp instantiation using rfl::Rename (allows renaming single field)
using TwitterDataReflectCpp = TwitterData_T<rfl::Rename<"protected", std::optional<bool>>>;

void reflectcpp_parse_populate(int iterations, const std::string& json_data) {
    TwitterDataReflectCpp model;
    
    benchmark("reflect-cpp parsing + populating", iterations, [&]() {
        // reflect-cpp requires a string copy since it parses from string_view
        std::string copy = json_data;
        
        auto result = rfl::json::read<TwitterDataReflectCpp, rfl::DefaultIfMissing>(copy);
        
        // Check if parsing failed
        if (!result) {
            const auto& err = result.error();
            throw std::runtime_error(
                std::format("reflect-cpp parse error: {}", err.what())
            );
        }
        
        model = std::move(result.value());
    });
}

