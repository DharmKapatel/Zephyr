/**
 * @file structs.h
 * @brief Definitions of sensor data structures and IDs
 *
 * Provides unified data structures for all sensors and
 * message passing in the logging system.
 */

#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdint.h>

/**
 * @brief Sensor type identifiers
 */
typedef enum {
    SENSOR_TYPE_HUM_TEMP = 0, /**< Humidity & Temperature sensor */
    SENSOR_TYPE_PRESSURE = 1, /**< Pressure sensor */
    SENSOR_TYPE_IMU      = 2, /**< IMU sensor (accelerometer + gyro) */
} sensor_type_t;

/**
 * @brief Humidity and temperature sensor data
 */
struct hum_temp_data {
    float temp; /**< Temperature in °C */
    float hum;  /**< Relative humidity in % */
};

/**
 * @brief Pressure sensor data
 */
struct pressure_data {
    float pressure; /**< Pressure in hPa */
};

/**
 * @brief IMU sensor data
 */
struct imu_data {
    float ax; /**< Acceleration X-axis in m/s² */
    float ay; /**< Acceleration Y-axis in m/s² */
    float az; /**< Acceleration Z-axis in m/s² */
    float gx; /**< Gyroscope X-axis in °/s */
    float gy; /**< Gyroscope Y-axis in °/s */
    float gz; /**< Gyroscope Z-axis in °/s */
};

/**
 * @brief Unified sensor data structure for logging
 *
 * @details
 * - Simplified to a single float value for general logging purposes.
 * - Includes a timestamp in milliseconds since system boot.
 */
struct sensor_data {
    int sensor_id;      /**< Sensor type (see sensor_type_t) */
    float value;        /**< Sensor value (single float for simplicity) */
    int64_t timestamp;  /**< Time in milliseconds since boot */
};

/**
 * @brief Generic sensor message for message queue
 *
 * @details
 * - Wraps sensor_data for passing between threads via k_msgq.
 */
struct sensor_message {
    struct sensor_data data; /**< Sensor data payload */
};

#endif /* STRUCTS_H */
