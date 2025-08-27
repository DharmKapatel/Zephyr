/**
 * @file structs.h
 * @brief Common data structures for sensor logging system.
 *
 * Defines data types for individual sensors, unified sensor snapshots,
 * and the generic message format used for inter-thread communication.
 *
 * These structures are shared between sensor drivers and the logger.
 *
 * @author Dharm
 * @date 2025
 */

#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdint.h>

/**
 * @brief Humidity + Temperature sensor data.
 */
typedef struct {
    float humidity;     /**< Relative humidity in %RH */
    float temperature;  /**< Temperature in °C */
} hum_temp_data;

/**
 * @brief Pressure sensor data.
 */
typedef struct {
    float pressure;     /**< Atmospheric pressure in hPa */
} press_data;

/**
 * @brief Single IMU axis set (x, y, z).
 */
typedef struct {
    float x;  /**< X-axis value */
    float y;  /**< Y-axis value */
    float z;  /**< Z-axis value */
} imu_data_t;

/**
 * @brief IMU sensor data (accelerometer + gyroscope).
 */
typedef struct {
    imu_data_t accel;   /**< Accelerometer readings (m/s²) */
    imu_data_t gyro;    /**< Gyroscope readings (°/s) */
} imu_sensor_data;

/**
 * @brief Unified snapshot of all available sensor data.
 *
 * Combines humidity, temperature, pressure, and IMU readings
 * into a single structure for logging and processing.
 */
typedef struct {
    hum_temp_data hts_data;   /**< Humidity + temperature data */
    press_data lps_data;      /**< Pressure data */
    imu_sensor_data imu_data; /**< IMU accelerometer + gyroscope data */
} sensors_shared_buf;

/**
 * @brief Generic message format for sensor queues.
 *
 * Encapsulates one snapshot of sensor data along with
 * a timestamp for logging and time-series analysis.
 */
struct sensor_message {
    sensors_shared_buf data;  /**< Sensor readings */
    int64_t timestamp;        /**< Capture time in ms since boot */
};

#endif /* STRUCTS_H */
