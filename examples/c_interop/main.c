// Pure C code that uses JSON parsing through C++ wrapper
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "structures.h"

// Functions implemented in parser.cpp
extern int ParseMotorSystem(MotorSystem* system, 
                            const char* json_data, 
                            size_t json_size,
                            size_t* error_position);

extern int SerializeMotorSystem(const MotorSystem* system,
                                char* output_buffer,
                                size_t buffer_size,
                                size_t* bytes_written);

void print_motor(const char* label, const Motor* motor) {
    printf("%s:\n", label);
    printf("  Position: [%lld, %lld, %lld]\n", 
           (long long)motor->position[0],
           (long long)motor->position[1],
           (long long)motor->position[2]);
    printf("  Active: %s\n", motor->active ? "Yes" : "No");
    printf("  Name: \"%s\"\n", motor->name);
}

void print_matrix(const double matrix[3][3], const char* label) {
    printf("%s:\n", label);
    for (int i = 0; i < 3; ++i) {
        printf("  [");
        for (int j = 0; j < 3; ++j) {
            printf("%.2f", matrix[i][j]);
            if (j < 2) printf(", ");
        }
        printf("]\n");
    }
}

void print_system(const MotorSystem* system) {
    printf("System Name: \"%s\"\n", system->system_name);
    printf("Motor Count: %d\n", system->motor_count);
    printf("\n");
    print_motor("Primary Motor", &system->primary_motor);
    printf("\n");
    
    printf("Motors Array:\n");
    for (int i = 0; i < system->motor_count && i < 5; ++i) {
        char label[32];
        snprintf(label, sizeof(label), "  Motor[%d]", i);
        print_motor(label, &system->motors[i]);
    }
    
    printf("\n");
    print_matrix(system->transform, "Transformation Matrix (3x3)");
}

int main() {
    printf("=== JsonFusion C Interop Test (MotorSystem) ===\n\n");
    
    // Test parsing
    const char* json = 
        "{"
        "  \"primary_motor\": {"
        "    \"position\": [100, 200, 300],"
        "    \"active\": 1,"
        "    \"name\": \"PrimaryMotor\""
        "  },"
        "  \"motors\": ["
        "    {"
        "      \"position\": [10, 20, 30],"
        "      \"active\": 1,"
        "      \"name\": \"Motor1\""
        "    },"
        "    {"
        "      \"position\": [11, 21, 31],"
        "      \"active\": 0,"
        "      \"name\": \"Motor2\""
        "    }"
        "  ],"
        "  \"motor_count\": 2,"
        "  \"system_name\": \"TestSystem\","
        "  \"transform\": ["
        "    [1.0, 0.0, 0.0],"
        "    [0.0, 1.0, 0.0],"
        "    [0.0, 0.0, 1.0]"
        "  ]"
        "}";
    
    MotorSystem system;
    memset(&system, 0, sizeof(system));
    size_t error_pos = 0;
    
    printf("Parsing JSON...\n");
    int result = ParseMotorSystem(&system, json, strlen(json), &error_pos);
    
    if (result == 0) {
        printf("✓ Parsed successfully!\n\n");
        print_system(&system);
    } else {
        printf("✗ Parse error %d at position %zu\n", result, error_pos);
        printf("JSON around error position:\n");
        if (error_pos < strlen(json)) {
            size_t start = (error_pos > 20) ? error_pos - 20 : 0;
            size_t len = (error_pos + 20 < strlen(json)) ? 40 : strlen(json) - start;
            printf("  ...%.*s...\n", (int)len, json + start);
            printf("  %*s^\n", (int)(error_pos - start), "");
        }
        return 1;
    }
    
    // Test serialization (round-trip)
    printf("\n=== Round-trip test ===\n");
    printf("IMPORTANT: C arrays serialize ALL elements.\n");
    printf("All motors in the array must have valid names (min_length<1>).\n");
    printf("Initializing unused motors with default values...\n\n");
    
    // Initialize unused motors with valid default values
    // This is required because C arrays serialize all elements,
    // and validation (min_length<1>) applies to all elements
    for (int i = system.motor_count; i < 5; ++i) {
        system.motors[i].position[0] = 0;
        system.motors[i].position[1] = 0;
        system.motors[i].position[2] = 0;
        system.motors[i].active = 0;
        strncpy(system.motors[i].name, "Unused", sizeof(system.motors[i].name) - 1);
        system.motors[i].name[sizeof(system.motors[i].name) - 1] = '\0';
    }
    
    char output[2048];
    size_t bytes_written = 0;
    
    result = SerializeMotorSystem(&system, output, sizeof(output), &bytes_written);
    
    if (result == 0) {
        printf("✓ Serialized successfully!\n");
        printf("Output (%zu bytes):\n%s\n\n", bytes_written, output);
        
        // Parse it back
        MotorSystem system2;
        memset(&system2, 0, sizeof(system2));
        result = ParseMotorSystem(&system2, output, bytes_written, &error_pos);
        
        if (result == 0) {
            printf("✓ Round-trip successful!\n\n");
            printf("Parsed back system:\n");
            print_system(&system2);
            
            // Verify round-trip
            printf("\n=== Verification ===\n");
            int match = 1;
            
            if (strcmp(system.system_name, system2.system_name) != 0) {
                printf("✗ System name mismatch\n");
                match = 0;
            }
            if (system.motor_count != system2.motor_count) {
                printf("✗ Motor count mismatch: %d != %d\n", 
                       system.motor_count, system2.motor_count);
                match = 0;
            }
            if (system.primary_motor.position[0] != system2.primary_motor.position[0] ||
                system.primary_motor.position[1] != system2.primary_motor.position[1] ||
                system.primary_motor.position[2] != system2.primary_motor.position[2]) {
                printf("✗ Primary motor position mismatch\n");
                match = 0;
            }
            if (strcmp(system.primary_motor.name, system2.primary_motor.name) != 0) {
                printf("✗ Primary motor name mismatch\n");
                match = 0;
            }
            
            // Verify transform matrix
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    if (system.transform[i][j] != system2.transform[i][j]) {
                        printf("✗ Transform matrix mismatch at [%d][%d]: %.2f != %.2f\n",
                               i, j, system.transform[i][j], system2.transform[i][j]);
                        match = 0;
                    }
                }
            }
            
            if (match) {
                printf("✓ All fields match!\n");
            }
        } else {
            printf("✗ Round-trip parse failed with error %d\n", result);
            return 1;
        }
    } else {
        printf("✗ Serialization failed with error %d\n", result);
        return 1;
    }
    
    return 0;
}
