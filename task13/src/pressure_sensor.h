/**
 * @file pressure_sensor.h
 * @brief Interface for LPS22HB pressure sensor
 *
 * Provides initialization, data acquisition, and threading
 * for the LPS22HB pressure sensor using Zephyr APIs.
 */

#ifndef PRESSURE_SENSOR_H
#define PRESSURE_SENSOR_H

#include "struct.h" /**< Contains definitions for sensor_data and sensor_message */

/**
 * @brief Initialize the pressure sensor
 *
 * @return 0 on success, -1 if the sensor device is not ready
 *
 * @details
 * - Checks if the LPS22HB device is ready.
 * - Creates a dedicated Zephyr thread for periodic pressure data acquisition.
 */
int pressure_sensor_init(void);

/**
 * @brief Get current pressure sensor data
 *
 * @param data Pointer to sensor_data structure to populate
 * @return 0 on success, -1 on failure
 *
 * @details
 * - Fetches a sample from the LPS22HB sensor.
 * - Converts the raw sensor value to float.
 * - Adds a timestamp using k_uptime_get().
 */
int pressure_sensor_get_data(struct sensor_data *data);

/**
 * @brief Thread function for periodic pressure data acquisition
 *
 * @param a Unused
 * @param b Unused
 * @param c Unused
 *
 * @details
 * - Runs indefinitely, fetching pressure data and enqueuing it
 *   to the logger module at a fixed interval (e.g., 3 seconds).
 */
void pressure_thread(void *a, void *b, void *c);

#endif /* PRESSURE_SENSOR_H */
