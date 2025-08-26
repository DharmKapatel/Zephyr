/**
 * @file main.c
 * @brief Entry point for the sensor logging system
 *
 * Initializes the logger and all sensor modules, then
 * enters an indefinite sleep loop as the main thread.
 *
 * @details
 * - Initializes the persistent logger using LittleFS.
 * - Initializes all sensor modules (IMU, pressure, humidity/temperature).
 * - Sensor data is periodically enqueued to the logger by each sensor thread.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "logger.h"
#include "imu_sensor.h"
#include "pressure_sensor.h"
#include "hum_temp_sensor.h"

LOG_MODULE_REGISTER(main); /**< Zephyr logging module registration */

/**
 * @brief Main entry point
 *
 * @details
 * - Starts the logger and all sensor threads.
 * - Main thread sleeps indefinitely while sensor threads handle data acquisition and logging.
 */
int main(void)
{
    LOG_INF("Starting sensor logging system...");

    /* Initialize logger and sensors */
    logger_init();
    imu_sensor_init();
    pressure_sensor_init();
    hum_temp_sensor_init();

    /* Main thread sleeps indefinitely */
    while (1) {
        k_sleep(K_SECONDS(10));
    }
}
