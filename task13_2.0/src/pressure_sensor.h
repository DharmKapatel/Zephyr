/**
 * @file pressure_sensor.h
 * @brief Interface for pressure sensor integration in Zephyr.
 *
 * Provides APIs to initialize the pressure sensor, read data into
 * the shared buffer, and run a dedicated sampling thread.
 *
 * Usage:
 * - Call pressure_sensor_init() at startup to spawn the sampling thread.
 * - The thread periodically fetches pressure values and pushes
 *   them into the logger message queue.
 *
 * Dependencies:
 * - struct.h (for sensors_shared_buf).
 * - Zephyr sensor driver framework.
 *
 * @author Dharm
 * @date 2025
 */

#ifndef PRESSURE_SENSOR_H
#define PRESSURE_SENSOR_H

#include "struct.h"

/**
 * @brief Initialize the pressure sensor and sampling thread.
 *
 * This function checks if the sensor device is ready and then
 * starts a dedicated thread for periodic data acquisition.
 *
 * @return 0 on success, -1 if the device is not ready.
 */
int pressure_sensor_init(void);

/**
 * @brief Fetch the latest pressure sensor data.
 *
 * Reads a new sample from the sensor and stores the converted
 * pressure value into the provided buffer.
 *
 * @param[out] data Pointer to the shared buffer for storing results.
 * @return 0 on success, -1 on failure.
 */
int pressure_sensor_get_data(sensors_shared_buf *data);

/**
 * @brief Thread function for periodic pressure sampling.
 *
 * This function should not be called directly.
 * It is automatically spawned by pressure_sensor_init().
 * It fetches data every 3 seconds and enqueues it in press_msgq.
 *
 * @param a Unused.
 * @param b Unused.
 * @param c Unused.
 */
void pressure_thread(void *a, void *b, void *c);

#endif /* PRESSURE_SENSOR_H */
