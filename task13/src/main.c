#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "logger.h"

LOG_MODULE_REGISTER(main);

void main(void)
{
    LOG_INF("Starting sensor logging system...");

    // Initialize logger
    logger_init();
    imu_sensor_init();
    pressure_sensor_init();
    hum_temp_sensor_init();



    // Main thread can sleep indefinitely
    while (1) {
        k_sleep(K_SECONDS(10));
    }
}
