// Basic JsonFusion usage example
// Compile: g++ -std=c++23 -I../include basic_usage.cpp -o basic_usage

#include <JsonFusion/parser.hpp>
#include <iostream>
#include <string>

using namespace JsonFusion;

struct Config {
    std::string app_name;
    int version;
    bool debug_mode;
    
    struct Server {
        std::string host;
        int port;
    };
    Server server;
};

int main() {
    const char* json = R"({
        "app_name": "MyApp",
        "version": 1,
        "debug_mode": true,
        "server": {
            "host": "localhost",
            "port": 8080
        }
    })";
    
    Config config;
    auto result = Parse(config, std::string_view(json));
    
    if (!result) {
        std::cout << "Parse error: " << static_cast<int>(result.error()) 
                  << " at position " << result.pos() << std::endl;
        return 1;
    }
    
    std::cout << "Successfully parsed!" << std::endl;
    std::cout << "App: " << config.app_name << std::endl;
    std::cout << "Version: " << config.version << std::endl;
    std::cout << "Debug: " << (config.debug_mode ? "ON" : "OFF") << std::endl;
    std::cout << "Server: " << config.server.host << ":" << config.server.port << std::endl;
    
    return 0;
}

