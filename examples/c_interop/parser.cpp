// C++ wrapper that provides JSON parsing for C structs
#include "structures.h"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/validators.hpp>
#include <JsonFusion/options.hpp>
#include <JsonFusion/struct_introspection.hpp>

using namespace JsonFusion;
using namespace JsonFusion::validators;
using namespace JsonFusion::options;

// External annotation for Motor using StructMeta (required for C arrays)
template<>
struct JsonFusion::StructMeta<Motor> {
    using Fields = StructFields<
        Field<&Motor::position, "position", min_items<3>>,  // int64_t position[3];
        Field<&Motor::active, "active">,                     // bool active;
        Field<&Motor::name, "name", min_length<1>>           // char name[20];
    >;
};

// External annotation for MotorSystem
template<>
struct JsonFusion::StructMeta<MotorSystem> {
    using Fields = StructFields<
        Field<&MotorSystem::primary_motor, "primary_motor">,      // Nested Motor
        Field<&MotorSystem::motors, "motors", max_items<5>>,       // C array of Motors
        Field<&MotorSystem::motor_count, "motor_count">,          // int
        Field<&MotorSystem::system_name, "system_name", min_length<1>, max_length<31>>,  // char[32]
        Field<&MotorSystem::transform, "transform">  // double[3][3] - nested C array handled automatically
    >;
};

extern "C" {

int ParseMotorSystem(MotorSystem* system, 
                     const char* json_data, 
                     size_t json_size,
                     size_t* error_position) {
    if (!system || !json_data) {
        return -1;
    }
    
    auto result = JsonFusion::Parse(*system, json_data, json_data + json_size);
    
    if (!result) {
        if (error_position) {
            *error_position = result.pos() - json_data;
        }
        return static_cast<int>(result.error());
    }
    
    return 0;  // Success
}

int SerializeMotorSystem(const MotorSystem* system,
                         char* output_buffer,
                         size_t buffer_size,
                         size_t* bytes_written) {
    if (!system || !output_buffer) {
        return -1;
    }
    char * start = output_buffer;
    auto result = JsonFusion::Serialize(*system, output_buffer, output_buffer + buffer_size);
    
    if (!result) {
        return static_cast<int>(result.error());
    }
    output_buffer[output_buffer - start] = '\0';
    if (bytes_written) {
        *bytes_written = output_buffer - start;
    }
    
    return 0;  // Success
}

} // extern "C"
