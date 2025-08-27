/**
 * @file structs.h
 * @brief Common sensor data structures and message formats.
 *
 * This header defines sensor type IDs, sensor-specific data
 * structures, and the unified data format used across the
 * logging system.
 *
 * Author: Dharm Kapatel
 */

#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdint.h>

/**
 * @enum sensor_type_t
 * @brief Identifiers for supported sensor types.
 */
typedef enum {
    SENSOR_TYPE_HUM_TEMP = 0,  /**< Humidity and Temperature sensor */
    SENSOR_TYPE_PRESSURE = 1,  /**< Pressure sensor */
    SENSOR_TYPE_IMU      = 2,  /**< Inertial Measurement Unit (IMU) */
} sensor_type_t;

/**
 * @struct hum_temp_data
 * @brief Data format for humidity and temperature sensor.
 */
struct hum_temp_data {
    float temp; /**< Temperature in °C */
    float hum;  /**< Relative humidity in % */
};

/**
 * @struct pressure_data
 * @brief Data format for pressure sensor.
 */
struct pressure_data {
    float pressure; /**< Pressure in hPa (or device units) */
};

/**
 * @struct imu_data
 * @brief Data format for IMU sensor.
 */
struct imu_data {
    float ax, ay, az; /**< Acceleration in m/s² */
    float gx, gy, gz; /**< Gyroscope values in rad/s */
};

/**
 * @struct sensor_data
 * @brief Unified data structure for all sensor types.
 *
 * Contains sensor type ID, timestamp, and sensor-specific data.
 */
struct sensor_data {
    int sensor_id;       /**< Sensor type (see @ref sensor_type_t) */
    int64_t timestamp;   /**< Time of sample in ms since boot */

    union {
        struct hum_temp_data hum_temp; /**< Humidity/Temperature data */
        struct pressure_data pressure; /**< Pressure data */
        struct imu_data imu;           /**< IMU data */
        float value;                   /**< Generic single value fallback */
    };
};

/**
 * @struct sensor_message
 * @brief Generic message wrapper for passing sensor data.
 *
 * Used when enqueuing data into the logger system.
 */
struct sensor_message {
    struct sensor_data data; /**< Sensor data payload */
};

#endif /* STRUCTS_H */
