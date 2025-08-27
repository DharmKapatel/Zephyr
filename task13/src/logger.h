/**
 * @file logger.h
 * @author Dharm
 * @brief Logging interface for queuing sensor messages in Zephyr.
 *
 * This module provides functions to enqueue sensor messages into
 * a message queue for further processing (e.g., storage, transmission).
 * It also defines helper functions for sensor value conversions.
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include "struct.h"

/**
 * @brief Convert Zephyr sensor_value struct to float.
 *
 * Converts a Zephyr @ref sensor_value (integer + fractional part)
 * into a single-precision float for easier handling.
 *
 * @param val Pointer to sensor_value structure.
 * @return Floating-point representation of the sensor value.
 */
static inline float sensor_to_float(const struct sensor_value *val)
{
    return (float)val->val1 + (float)val->val2 / 1000000.0f;
}

/** @brief Maximum number of queued messages in memory. */
#define MAX_QUEUE 50

/** @brief Maximum number of entries stored in logger. */
#define MAX_ENTRIES 1000

/**
 * @brief Initialize the logger module.
 *
 * This function sets up internal data structures such as
 * queues and storage for incoming sensor messages.
 */
void logger_init(void);

/**
 * @brief Enqueue a sensor message into the logger queue.
 *
 * @param msg Pointer to a sensor_message structure containing sensor data.
 */
void logger_enqueue(const struct sensor_message *msg);

#endif /* LOGGER_H */
