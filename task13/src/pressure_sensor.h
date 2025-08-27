/**
 * @file pressure_sensor.h
 * @brief Pressure sensor module interface.
 *
 * This header provides function prototypes for initializing,
 * sampling, and running the pressure sensor background thread.
 *
 * Author: Dharm Kapatel
 */

#ifndef PRESSURE_SENSOR_H
#define PRESSURE_SENSOR_H

#include "struct.h"



/**
 * @brief Initialize the pressure sensor module.
 *
 * Checks device readiness and starts the background thread for
 * periodic pressure sampling.
 *
 * @retval 0  Success
 * @retval -1 Device not ready
 */
int pressure_sensor_init(void);

/**
 * @brief Get the latest pressure data sample.
 *
 * Fetches a single reading from the sensor and fills the given
 * @ref sensor_data structure with the result.
 *
 * @param[out] data Pointer to structure where data is stored.
 *
 * @retval 0  Success
 * @retval -1 Failure (device not ready or fetch error)
 */
int pressure_sensor_get_data(struct sensor_data *data);

/**
 * @brief Pressure sensor background thread.
 *
 * This thread continuously fetches pressure samples at a fixed
 * interval and enqueues them into the logger system.
 *
 * @param a Unused
 * @param b Unused
 * @param c Unused
 */
void pressure_thread(void *a, void *b, void *c);



#endif /* PRESSURE_SENSOR_H */
