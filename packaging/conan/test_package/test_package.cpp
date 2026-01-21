#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/error_formatting.hpp>
#include <iostream>
#include <string>

using JsonFusion::A;

struct SimpleConfig {
    std::string name;
    int value;
    bool enabled;
};

int main() {
    std::cout << "Testing JsonFusion package...\n";
    
    SimpleConfig config;
    std::string input = R"({"name":"test","value":42,"enabled":true})";
    
    auto parseResult = JsonFusion::Parse(config, input);
    
    if (!parseResult) {
        std::cerr << "Parse failed: " << JsonFusion::ParseResultToString<SimpleConfig>(parseResult, input.begin(), input.end()) << "\n";
        return 1;
    }
    
    std::cout << "Parsed: name=" << config.name 
              << ", value=" << config.value 
              << ", enabled=" << config.enabled << "\n";
    
    std::string output;
    auto serResult = JsonFusion::Serialize(config, output);
    
    if (!serResult) {
        std::cerr << "Serialization failed\n";
        return 1;
    }
    
    std::cout << "Serialized: " << output << "\n";
    std::cout << "JsonFusion package test PASSED!\n";
    
    return 0;
}
