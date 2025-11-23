#ifndef STRUCTURES_H
#define STRUCTURES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int sensor_id;
    float threshold;
    int active;  // Using int as bool for C compatibility
} SensorConfig;

typedef struct {
    int device_id;
    float temperature;
    SensorConfig sensor;
} DeviceConfig;

#ifdef __cplusplus
}
#endif

#endif // STRUCTURES_H

