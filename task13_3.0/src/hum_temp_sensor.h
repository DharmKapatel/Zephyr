/**
 * @file hum_temp_sensor.h
 * @brief Interface for Humidity-Temperature sensor module.
 *
 * This header provides the APIs for initializing, sampling, and
 * running a dedicated thread to read humidity and temperature data
 * using the Zephyr sensor API.
 *
 * The data is packaged into `sensor_data` structures and passed
 * to the logger module for further processing.
 *
 * @author Dharm Kapatel
 */

#ifndef HUM_TEMP_SENSOR_H
#define HUM_TEMP_SENSOR_H

#include "struct.h"

/**
 * @brief Initialize the Humidity-Temperature sensor.
 *
 * This function checks if the sensor device is ready and starts
 * a dedicated thread to periodically fetch sensor data and send it
 * to the logger.
 *
 * @retval 0 on success
 * @retval -1 if the device is not ready
 */
int hum_temp_sensor_init(void);

/**
 * @brief Fetch latest Humidity and Temperature data.
 *
 * Reads sensor values from the hardware and fills the provided
 * `sensor_data` structure with temperature, humidity, and timestamp.
 *
 * @param[out] data Pointer to user-provided struct to hold the results.
 *
 * @retval 0 on success
 * @retval -1 on failure (device not ready or sensor read error)
 */
int hum_temp_sensor_get_data(struct sensor_data *data);

/**
 * @brief Thread function for periodic humidity-temperature sampling.
 *
 * This function is designed to run as a dedicated Zephyr thread.
 * It periodically reads humidity and temperature data and enqueues
 * the results to the logger.
 *
 * @param a Unused
 * @param b Unused
 * @param c Unused
 */
void hum_temp_thread(void *a, void *b, void *c);

#endif /* HUM_TEMP_SENSOR_H */
