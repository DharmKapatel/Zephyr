/**
 * @file imu_sensor.h
 * @brief Interface for IMU (Inertial Measurement Unit) sensor module.
 *
 * This header provides the APIs for initializing, sampling, and
 * running a dedicated thread to read accelerometer and gyroscope data
 * using the Zephyr sensor API.
 *
 * The data is packaged into `imu_data` structures and passed
 * to the logger module for further processing.
 *
 * @author Dharm Kapatel
 */

#ifndef IMU_SENSOR_H
#define IMU_SENSOR_H

#include "struct.h"

/**
 * @brief Initialize the IMU sensor.
 *
 * This function checks if the IMU device is ready and starts
 * a dedicated thread to periodically fetch accelerometer and gyroscope
 * data and send it to the logger.
 *
 * @retval 0 on success
 * @retval -1 on failure (device not ready)
 */
int imu_sensor_init(void);

/**
 * @brief Fetch latest IMU sensor data.
 *
 * Reads accelerometer (X, Y, Z) and gyroscope (X, Y, Z) values from
 * the IMU and fills the provided `imu_data` structure.
 *
 * @param[out] imu Pointer to user-provided struct to hold IMU readings.
 *
 * @retval 0 on success
 * @retval -1 on failure (device not ready or sensor read error)
 */
int imu_sensor_get_data(struct imu_data *imu);

/**
 * @brief Thread function for periodic IMU sampling.
 *
 * This function is designed to run as a dedicated Zephyr thread.
 * It periodically reads IMU data and enqueues the results
 * to the logger.
 *
 * @param a Unused
 * @param b Unused
 * @param c Unused
 */
void imu_thread(void *a, void *b, void *c);

#endif /* IMU_SENSOR_H */
