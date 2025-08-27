#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdint.h>

/* Sensor type IDs */
typedef enum {
    SENSOR_TYPE_HUM_TEMP = 0,
    SENSOR_TYPE_PRESSURE = 1,
    SENSOR_TYPE_IMU      = 2,
} sensor_type_t;

/* Data structs for each sensor */
struct hum_temp_data {
    float temp;
    float hum;
};

struct pressure_data {
    float pressure;
};

struct imu_data {
    float ax, ay, az;
    float gx, gy, gz;
};

/* Unified sensor data struct for logging */
struct sensor_data {
    int sensor_id;       // sensor type
    int64_t timestamp;   // milliseconds since boot
    union {
        struct hum_temp_data hum_temp;
        struct pressure_data pressure;
        struct imu_data imu;
        float value;  // fallback single value
    };
};

/* Generic sensor message to pass through the queue */
struct sensor_message {
    struct sensor_data data;
};

#endif /* STRUCTS_H */
