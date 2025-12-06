#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Motor structure with C arrays
typedef struct {
    int64_t position[3];  // 3D position
    int active;            // Using int as bool for C compatibility
    char name[20];         // Motor name (null-terminated C string)
} Motor;

// MotorSystem with nested Motor and C array of Motors
typedef struct {
    Motor primary_motor;   // Primary motor
    Motor motors[5];       // Array of up to 5 motors
    int motor_count;       // Number of active motors (0-5)
    char system_name[32];  // System name (null-terminated C string)
    double transform[3][3];  // 3x3 transformation matrix (nested C array)
} MotorSystem;

#ifdef __cplusplus
}
#endif

#endif // STRUCTURES_H
