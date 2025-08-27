/**
 * @file logger.h
 * @brief Public API and definitions for the sensor data logger.
 *
 * This header exposes the logger interface and per-sensor message queues
 * used by the logging module. The logger collects sensor data (HTS, Pressure,
 * IMU) from Zephyr threads, synchronizes them, and writes them to a circular
 * buffer on LittleFS.
 *
 * Usage:
 * - Call logger_init() once at startup.
 * - Enqueue sensor data using logger_enqueue() or directly via sensor queues.
 *
 * @author Dharm
 * @date 2025
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include "struct.h"

/**
 * @brief Convert Zephyr sensor_value to floating-point.
 *
 * Helper function to convert a sensor_value struct (val1, val2) to float.
 *
 * @param val Pointer to sensor_value to convert.
 * @return Floating-point representation of the sensor value.
 */
static inline float sensor_to_float(const struct sensor_value *val)
{
    return (float)val->val1 + (float)val->val2 / 1000000.0f;
}

/** @brief Maximum number of messages per sensor queue. */
#define MAX_QUEUE 50

/** @brief Maximum number of sensor entries stored in flash log. */
#define MAX_ENTRIES 1000

/** 
 * @brief Per-sensor message queues (defined in logger.c).
 *
 * These are exposed for direct sensor thread usage.
 */
extern struct k_msgq hts_msgq;   /**< Queue for HTS (humidity & temperature) data. */
extern struct k_msgq press_msgq; /**< Queue for pressure sensor data. */
extern struct k_msgq imu_msgq;   /**< Queue for IMU (accelerometer & gyroscope) data. */

/**
 * @brief Initialize the logger module.
 *
 * - Mounts the filesystem.
 * - Ensures data & metadata files exist.
 * - Spawns the logger thread.
 */
void logger_init(void);

/**
 * @brief Enqueue a sensor message to the logger (default HTS queue).
 *
 * Compatibility helper: sensors typically enqueue directly into their
 * dedicated queues, but this provides a fallback.
 *
 * @param msg Pointer to sensor message to enqueue.
 */
void logger_enqueue(const struct sensor_message *msg);

#endif /* LOGGER_H */
