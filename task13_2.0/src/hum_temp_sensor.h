/**
 * @file hum_temp_sensor.h
 * @author Dharm Kapatel
 * @brief Humidity and Temperature sensor interface header
 * @version 0.1
 * @date 2025-08-27
 *
 * This header defines the API for initializing, fetching data, 
 * and running the thread for the humidity-temperature sensor.
 */

#ifndef HUM_TEMP_SENSOR_H
#define HUM_TEMP_SENSOR_H

#include "struct.h"

/**
 * @brief Initialize the humidity-temperature sensor and its thread
 *
 * This function checks if the HTS device is ready and 
 * starts the dedicated sampling thread.
 *
 * @return int 0 on success, -1 on failure
 */
int hum_temp_sensor_init(void);

/**
 * @brief Fetch humidity and temperature data from the sensor
 *
 * @param data Pointer to shared buffer where sensor data is stored
 * @return int 0 on success, -1 on failure
 */
int hum_temp_sensor_get_data(sensors_shared_buf *data);

/**
 * @brief Thread function for periodic humidity-temperature sampling
 *
 * This thread continuously reads humidity and temperature values,
 * adds a timestamp, and pushes them into the message queue.
 *
 * @param a Unused
 * @param b Unused
 * @param c Unused
 */
void hum_temp_thread(void *a, void *b, void *c);

#endif /* HUM_TEMP_SENSOR_H */
