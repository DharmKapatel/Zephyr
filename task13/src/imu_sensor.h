/**
 * @file imu_sensor.h
 * @brief Interface for LSM6DSL IMU sensor
 *
 * Provides initialization, data acquisition, and threading
 * for the LSM6DSL IMU sensor using Zephyr APIs.
 */

#ifndef IMU_SENSOR_H
#define IMU_SENSOR_H

#include "struct.h" /**< Contains definition for sensor_data structure */

/**
 * @brief Initialize the IMU sensor
 *
 * @return 0 on success, -1 if the sensor device is not ready
 *
 * @details
 * - Checks if the LSM6DSL device is ready.
 * - Creates a dedicated Zephyr thread for periodic data acquisition.
 */
int imu_sensor_init(void);

/**
 * @brief Get current IMU sensor data
 *
 * @param data Pointer to sensor_data structure to populate
 * @return 0 on success, -1 on failure
 *
 * @details
 * - Fetches accelerometer data from the LSM6DSL.
 * - Converts the X-axis value to float.
 * - Adds a timestamp using k_uptime_get().
 */
int imu_sensor_get_data(struct sensor_data *data);

/**
 * @brief Thread function for periodic IMU data acquisition
 *
 * @param a Unused
 * @param b Unused
 * @param c Unused
 *
 * @details
 * - Runs indefinitely, fetching accelerometer data and enqueuing it
 *   to the logger module at a fixed interval.
 */
void imu_thread(void *a, void *b, void *c);

#endif /* IMU_SENSOR_H */
