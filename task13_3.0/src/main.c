/**
 * @file main.c
 * @brief Entry point for the sensor logging system (Zephyr RTOS).
 *
 * This module initializes the logger and all available sensors:
 * - IMU (Inertial Measurement Unit)
 * - Pressure sensor
 * - Humidity/Temperature sensor
 *
 * Once initialized, the main thread enters an idle loop
 * (`K_FOREVER`) while sensor-specific threads manage data
 * acquisition and enqueue sensor messages to the logger.
 *
 * @author Dharm Kapatel
 * @date 2025
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "logger.h"
#include "imu_sensor.h"
#include "pressure_sensor.h"
#include "hum_temp_sensor.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/**
 * @brief Main entry point.
 *
 * Initializes the logging system and all sensor modules.
 * After initialization, the thread sleeps indefinitely,
 * leaving sensor threads to handle acquisition and logging.
 */
int main(void)
{
    LOG_INF("Starting sensor logging system...");

    // Initialize logger first (must be ready before sensor threads start)
    logger_init();

    // Initialize all sensors
    if (imu_sensor_init() < 0) {
        LOG_ERR("Failed to initialize IMU sensor");
    }

    if (pressure_sensor_init() < 0) {
        LOG_ERR("Failed to initialize pressure sensor");
    }

    if (hum_temp_sensor_init() < 0) {
        LOG_ERR("Failed to initialize humidity/temp sensor");
    }

    LOG_INF("All modules initialized, entering idle loop...");

    // Main thread can sleep indefinitely
    while (1) {
        k_sleep(K_FOREVER);  
    }
}
