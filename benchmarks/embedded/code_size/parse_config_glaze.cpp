#define GLZ_DISABLE_ALWAYS_INLINE
#include <string_view>
// #include <glaze/json.hpp>
#include <cstdlib>
#include <glaze/json/read.hpp>

#include "embedded_config.hpp"



// Global config instance (will go in .bss section)
embedded_benchmark::EmbeddedConfig g_config;

// Parse function - instantiates JsonFusion parser for this model
// This is what gets measured for code size
extern "C" __attribute__((used)) bool parse_config(const char* data, size_t size) {
    auto error = glz::read<glz::opts_size{}>(g_config, std::string_view(data, size));
    return !error;
}

extern "C" __attribute__((used)) bool parse_rpc_command(const char* data, size_t size) {
    
    embedded_benchmark::RpcCommand cmd;

    auto error = glz::read<glz::opts_size{}>(cmd, std::string_view(data, size));
    return !error;
}
// Entry point - ensures parse_config is not eliminated by linker
// In a real embedded system, this would be your main loop
int main() {
    // Call parse_config to ensure it's included in binary
    volatile bool result = parse_config("", 0);
    volatile bool rpc_result = parse_rpc_command("", 0);
    (void)result;
    (void)rpc_result;
    
    // Infinite loop (typical for embedded without OS)
    while(1) {}
    return 0;
}

