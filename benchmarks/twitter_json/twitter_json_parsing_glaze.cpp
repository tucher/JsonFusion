#include <string>
#include <format>
#include <stdexcept>

#include <glaze/glaze.hpp>

#include "twitter_model_generic.hpp"
#include "benchmark.hpp"

// Glaze instantiation - Glaze works with plain structs, no wrapper needed
using TwitterDataGlaze = TwitterData_T<std::optional<bool>>;

// Glaze metadata for User to handle "protected" field renaming
template<>
struct glz::meta<User_T<std::optional<bool>>> {
    static constexpr auto modify = glz::object(
        "protected",  &User_T<std::optional<bool>>::protected_
     );
};

void glaze_parse_populate(int iterations, const std::string& json_data) {
    TwitterDataGlaze model;
    
    benchmark("Glaze parsing + populating", iterations, [&]() {
        // Glaze requires a mutable string for in-place parsing
        std::string copy = json_data;
        
        auto error = glz::read_json(model, copy);
        
        if (error) {
            throw std::runtime_error(
                std::format("Glaze parse error: {}", glz::format_error(error, copy))
            );
        }
    });
}

