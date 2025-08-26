/**
 * @file logger.h
 * @brief Logger module for sensor data using LittleFS and circular buffer
 *
 * Provides APIs to initialize the logger, enqueue sensor messages,
 * and helper functions to convert Zephyr sensor values to float.
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include "struct.h" /**< Contains definitions for sensor_data and sensor_message */

/**
 * @brief Convert Zephyr sensor_value to float
 *
 * @param val Pointer to sensor_value structure
 * @return Floating point representation of the sensor value
 *
 * @details
 * - Combines val1 (integer part) and val2 (microfraction) to a float.
 */
static inline float sensor_to_float(const struct sensor_value *val)
{
    return (float)val->val1 + (float)val->val2 / 1000000.0f;
}

/** @brief Maximum number of messages in the logger queue */
#define MAX_QUEUE 50

/** @brief Maximum number of entries in the persistent circular buffer */
#define MAX_ENTRIES 1000

/**
 * @brief Initialize the logger module
 *
 * @details
 * - Mounts the LittleFS filesystem.
 * - Ensures the log file exists and pre-allocates space.
 * - Loads or initializes persistent metadata.
 * - Starts the logger thread.
 */
void logger_init(void);

/**
 * @brief Enqueue a sensor message to be logged
 *
 * @param msg Pointer to sensor_message structure to enqueue
 *
 * @details
 * - Adds the message to the internal message queue.
 * - The logger thread will write the message to flash asynchronously.
 */
void logger_enqueue(const struct sensor_message *msg);

#endif /* LOGGER_H */
