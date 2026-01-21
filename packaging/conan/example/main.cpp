#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/error_formatting.hpp>
#include <iostream>
#include <vector>

using JsonFusion::A;
using namespace JsonFusion::validators;

// Example: Configuration for a sensor network
struct SensorConfig {
    struct Sensor {
        int id;
        std::string name;
        A<double, range<-50.0, 150.0>> temperature_threshold;
        bool active;
    };
    
    std::vector<Sensor> sensors;
    std::string network_name;
    A<int, range<1000, 65535>> port;
};

int main() {
    std::cout << "=== JsonFusion Conan Consumer Example ===\n\n";
    
    // Sample JSON input
    std::string_view json_input = R"({
        "network_name": "Building_A_Sensors",
        "port": 8080,
        "sensors": [
            {
                "id": 1,
                "name": "Room_101",
                "temperature_threshold": 25.5,
                "active": true
            },
            {
                "id": 2,
                "name": "Room_102",
                "temperature_threshold": 22.0,
                "active": false
            }
        ]
    })";
    
    // Parse JSON into struct
    SensorConfig config;
    auto parse_result = JsonFusion::Parse(config, json_input);
    
    if (!parse_result) {
        std::cerr << "Parse error: " << JsonFusion::ParseResultToString<SensorConfig>(parse_result, json_input.begin(), json_input.end()) << "\n";
        return 1;
    }
    
    std::cout << "Successfully parsed configuration!\n";
    std::cout << "Network: " << config.network_name << "\n";
    std::cout << "Port: " << config.port << "\n";
    std::cout << "Number of sensors: " << config.sensors.size() << "\n\n";
    
    // Display sensor information
    for (const auto& sensor : config.sensors) {
        std::cout << "Sensor ID: " << sensor.id << "\n";
        std::cout << "  Name: " << sensor.name << "\n";
        std::cout << "  Threshold: " << sensor.temperature_threshold << "°C\n";
        std::cout << "  Active: " << (sensor.active ? "Yes" : "No") << "\n\n";
    }
    
    // Modify configuration
    config.sensors[0].temperature_threshold = 27.0;
    config.port = 9090;
    
    // Serialize back to JSON
    std::string json_output;
    auto serialize_result = JsonFusion::Serialize(config, json_output);
    
    if (!serialize_result) {
        std::cerr << "Serialization error\n";
        return 1;
    }
    
    std::cout << "Modified configuration:\n" << json_output << "\n\n";
    
    // Example with validation error
    std::string_view invalid_json = R"({
        "network_name": "Test",
        "port": 99999,
        "sensors": []
    })";
    
    SensorConfig invalid_config;
    auto invalid_result = JsonFusion::Parse(invalid_config, invalid_json);
    
    if (!invalid_result) {
        std::cout << "Expected validation error caught:\n";
        std::cout << "  " << JsonFusion::ParseResultToString<SensorConfig>(invalid_result, invalid_json.begin(), invalid_json.end()) << "\n";
    }
    
    std::cout << "\n=== Example completed successfully! ===\n";
    return 0;
}
