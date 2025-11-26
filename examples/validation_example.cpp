// JsonFusion validation example with Annotated fields
// Demonstrates:
//  - Field validation (range, length constraints)
//  - Map validation (property count, key length)
//  - Using key<"..."> to decouple C++ names from JSON field names
// Compile: g++ -std=c++23 -I../include validation_example.cpp -o validation_example

#include <JsonFusion/parser.hpp>
#include <JsonFusion/annotated.hpp>
#include <JsonFusion/options.hpp>
#include <JsonFusion/validators.hpp>
#include <iostream>
#include <array>
#include <map>
#include <string>

using namespace JsonFusion;
using namespace JsonFusion::options;
using namespace JsonFusion::validators;

struct Motor {
    // C++ name is "motor_id", but JSON field is "id"
    Annotated<int, 
              key<"id">, 
              range<1, 8>>                          motor_id;
    
    // C++ name is "motor_name", but JSON field is "name"
    Annotated<std::string,
              key<"name">,
              min_length<1>, max_length<32>>        motor_name;
    
    // C++ name is "pos", but JSON field is "position"
    Annotated<std::array<float, 3>,
              key<"position">>                      pos;
};

struct Config {
    std::array<Motor, 2> motors;
};

int main() {
    // Valid JSON
    const char* valid_json = R"({
        "motors": [
            {
                "id": 1,
                "name": "Motor1",
                "position": [1.0, 2.0, 3.0]
            },
            {
                "id": 2,
                "name": "Motor2",
                "position": [4.0, 5.0, 6.0]
            }
        ]
    })";
    
    Config config;
    auto result = Parse(config, std::string_view(valid_json));
    
    if (result) {
        std::cout << "✓ Valid JSON parsed successfully!" << std::endl;
        std::cout << "Motor 1: motor_id=" << config.motors[0].motor_id 
                  << ", motor_name=" << config.motors[0].motor_name.data() << std::endl;
        std::cout << "  (C++ names differ from JSON field names!)" << std::endl;
    }
    
    // Invalid JSON - ID out of range
    const char* invalid_json = R"({
        "motors": [
            {
                "id": 99,
                "name": "MotorX",
                "position": [1.0, 2.0, 3.0]
            },
            {
                "id": 2,
                "name": "Motor2",
                "position": [4.0, 5.0, 6.0]
            }
        ]
    })";
    
    Config config2;
    auto result2 = Parse(config2, std::string_view(invalid_json));
    
    if (!result2) {
        std::cout << "✗ Invalid JSON caught: error=" << static_cast<int>(result2.error())
                  << " at position " << result2.pos() << std::endl;
        std::cout << "  (Motor ID 99 is outside valid range 1-8)" << std::endl;
    }
    
    // Map validation example
    std::cout << "\n--- Map Validation ---" << std::endl;
    
    using ValidatedMap = Annotated<
        std::map<std::string, int>,
        min_properties<2>,      // At least 2 entries
        max_properties<5>,      // At most 5 entries
        min_key_length<3>,      // Keys >= 3 chars
        max_key_length<10>      // Keys <= 10 chars
    >;
    
    ValidatedMap map;
    const char* map_json = R"({"name": 1, "age": 2, "city": 3})";
    auto map_result = Parse(map, map_json);
    
    if (map_result) {
        std::cout << "✓ Valid map: 3 entries, keys 3-10 chars" << std::endl;
    }
    
    // Invalid: key too short
    const char* bad_map = R"({"ab": 1, "name": 2})";
    auto bad_result = Parse(map, bad_map);
    if (!bad_result) {
        std::cout << "✗ Invalid map caught: key 'ab' too short (< 3 chars)" << std::endl;
    }
    
    return 0;
}

