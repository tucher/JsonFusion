// Pure C code that uses JSON parsing through C++ wrapper
#include <stdio.h>
#include <string.h>
#include "structures.h"

// Functions implemented in parser.cpp
extern int ParseDeviceConfig(DeviceConfig* config, 
                            const char* json_data, 
                            size_t json_size,
                            size_t* error_position);

extern int SerializeDeviceConfig(const DeviceConfig* config,
                                char* output_buffer,
                                size_t buffer_size,
                                size_t* bytes_written);

int main() {
    printf("=== JsonFusion C Interop Test ===\n\n");
    
    // Test parsing
    const char* json = 
        "{"
        "  \"device_id\": 42,"
        "  \"temperature\": 23.5,"
        "  \"sensor\": {"
        "    \"sensor_id\": 1,"
        "    \"threshold\": 25.5,"
        "    \"active\": 1"
        "  }"
        "}";
    
    DeviceConfig config;
    memset(&config, 0, sizeof(config));
    size_t error_pos = 0;
    
    printf("Parsing JSON...\n");
    int result = ParseDeviceConfig(&config, json, strlen(json), &error_pos);
    
    if (result == 0) {
        printf("✓ Parsed successfully!\n\n");
        printf("Device ID: %d\n", config.device_id);
        printf("Temperature: %.1f\n", config.temperature);
        printf("Sensor ID: %d\n", config.sensor.sensor_id);
        printf("Sensor Threshold: %.1f\n", config.sensor.threshold);
        printf("Sensor Active: %d\n", config.sensor.active);
    } else{
        printf("✗ Parse error %d at position %zu\n", result, error_pos);
        return 1;
    }
    
    // Test serialization (round-trip)
    printf("\n=== Round-trip test ===\n");
    char output[512];
    size_t bytes_written = 0;
    
    result = SerializeDeviceConfig(&config, output, sizeof(output), &bytes_written);
    
    if (result == 0) {
        printf("✓ Serialized successfully!\n");
        printf("Output (%zu bytes):\n%s\n", bytes_written, output);
        
        // Parse it back
        DeviceConfig config2;
        memset(&config2, 0, sizeof(config2));
        result = ParseDeviceConfig(&config2, output, bytes_written, &error_pos);
        
        if (result == 0) {
            printf("\n✓ Round-trip successful!\n");
            printf("Device ID matches: %d == %d ? %s\n", 
                   config.device_id, config2.device_id,
                   config.device_id == config2.device_id ? "YES" : "NO");
        } else {
            printf("✗ Round-trip parse failed\n");
            return 1;
        }
    } else {
        printf("✗ Serialization failed with error %d\n", result);
        return 1;
    }
    
    return 0;
}

