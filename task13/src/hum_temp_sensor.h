/**
 * @file hum_temp_sensor.h
 * @brief Interface for HTS221 humidity and temperature sensor
 *
 * Provides initialization, data acquisition, and threading
 * for the HTS221 sensor using Zephyr APIs.
 */

#ifndef HUM_TEMP_SENSOR_H
#define HUM_TEMP_SENSOR_H

#include "struct.h" /**< Contains definition for sensor_data structure */

/**
 * @brief Initialize the HTS221 sensor
 *
 * @return 0 on success, -1 if the sensor device is not ready
 *
 * @details
 * - Checks if the HTS221 device is ready.
 * - Creates a dedicated Zephyr thread for periodic data acquisition.
 */
int hum_temp_sensor_init(void);

/**
 * @brief Get current humidity and temperature data
 *
 * @param data Pointer to sensor_data structure to populate
 * @return 0 on success, -1 on failure
 *
 * @details
 * - Fetches a sample from the HTS221 device.
 * - Converts the raw sensor value to float.
 * - Adds a timestamp using k_uptime_get().
 */
int hum_temp_sensor_get_data(struct sensor_data *data);

/**
 * @brief Thread function for periodic data acquisition
 *
 * @param a Unused
 * @param b Unused
 * @param c Unused
 *
 * @details
 * - Runs indefinitely, fetching sensor data and enqueuing it
 *   to the logger module at a fixed interval.
 */
void hum_temp_thread(void *a, void *b, void *c);

#endif /* HUM_TEMP_SENSOR_H */
