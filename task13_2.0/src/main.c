/**
 * @file main.c
 * @brief Application entry point for the sensor logging system.
 *
 * This program initializes the logger and all sensor modules
 * (IMU, Pressure, Humidity/Temperature) and then runs indefinitely.
 *
 * The logger module collects sensor data, synchronizes updates,
 * and writes them into a circular buffer on LittleFS.
 *
 * @author Dharm
 * @date 2025
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "logger.h"

LOG_MODULE_REGISTER(main);

/**
 * @brief Main application entry point.
 *
 * - Starts the logger subsystem.
 * - Initializes IMU, pressure, and humidity/temperature sensors.
 * - Enters an idle loop (periodic sleep).
 */
void main(void)
{
    LOG_INF("Starting sensor logging system...");

    /* Initialize logger and sensors */
    logger_init();
    imu_sensor_init();
    pressure_sensor_init();
    hum_temp_sensor_init();

    /* Main thread sleeps; sensor threads + logger handle all work */
    while (1) {
        k_sleep(K_SECONDS(10));
    }
}
