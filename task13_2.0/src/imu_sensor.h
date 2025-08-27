/**
 * @file imu_sensor.h
 * @author Dharm Kapatel
 * @brief IMU (Accelerometer + Gyroscope) sensor interface header
 * @version 0.1
 * @date 2025-08-27
 *
 * This header defines the API for initializing, fetching data,
 * and running the thread for the IMU sensor.
 */

#ifndef IMU_SENSOR_H
#define IMU_SENSOR_H

#include "struct.h"

/**
 * @brief Initialize the IMU sensor and its thread
 *
 * This function checks if the IMU device is ready and 
 * starts the dedicated sampling thread.
 *
 * @return int 0 on success, -1 on failure
 */
int imu_sensor_init(void);

/**
 * @brief Fetch accelerometer and gyroscope data from the IMU sensor
 *
 * @param data Pointer to shared buffer where sensor data is stored
 * @return int 0 on success, -1 on failure
 */
int imu_sensor_get_data(sensors_shared_buf *data);

/**
 * @brief Thread function for periodic IMU sampling
 *
 * This thread continuously reads accelerometer and gyroscope values,
 * adds a timestamp, and pushes them into the message queue.
 *
 * @param a Unused
 * @param b Unused
 * @param c Unused
 */
void imu_thread(void *a, void *b, void *c);

#endif /* IMU_SENSOR_H */
