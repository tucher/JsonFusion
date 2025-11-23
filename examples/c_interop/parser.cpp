// C++ wrapper that provides JSON parsing for C structs
#include "structures.h"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>

extern "C" {

int ParseDeviceConfig(DeviceConfig* config, 
                     const char* json_data, 
                     size_t json_size,
                     size_t* error_position) {
    if (!config || !json_data) {
        return -1;
    }
    
    auto result = JsonFusion::Parse(*config, json_data, json_data + json_size);
    
    if (!result) {
        if (error_position) {
            *error_position = result.pos() - json_data;
        }
        return static_cast<int>(result.error());
    }
    
    return 0;  // Success
}

int SerializeDeviceConfig(const DeviceConfig* config,
                         char* output_buffer,
                         size_t buffer_size,
                         size_t* bytes_written) {
    if (!config || !output_buffer) {
        return -1;
    }
    
    auto result = JsonFusion::Serialize(*config, output_buffer, output_buffer + buffer_size);
    
    if (!result) {
        return static_cast<int>(result.error());
    }
    
    if (bytes_written) {
        *bytes_written = result.pos() - output_buffer;
    }
    
    return 0;  // Success
}

} // extern "C"

