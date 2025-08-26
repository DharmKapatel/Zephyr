/**
 * @file pressure_sensor.c
 * @brief Driver for LPS22HB pressure sensor
 *
 * This module initializes the LPS22HB sensor, periodically reads
 * pressure data, and enqueues it to the logger.
 *
 * @details
 * - Uses Zephyr sensor API for sampling.
 * - Runs a dedicated thread to fetch and log sensor data.
 */

#include "pressure_sensor.h"
#include "logger.h"
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pressure); /**< Zephyr logging module registration */

/** 
 * @brief Pressure sensor device instance
 * 
 * Obtained via device tree alias "pressure_sensor".
 */
#if DT_NODE_EXISTS(DT_ALIAS(pressure_sensor))
const struct device *const pressure_dev = DEVICE_DT_GET(DT_ALIAS(pressure_sensor));
#else
#error "Pressure sensor not found."
#endif

/**
 * @brief Fetch current pressure data
 *
 * @param data Pointer to sensor_data structure to populate
 * @return 0 on success, -1 on failure
 *
 * @details
 * - Fetches a sample from the LPS22HB sensor.
 * - Converts the raw sensor value to float.
 * - Adds a timestamp using k_uptime_get().
 */
int pressure_sensor_get_data(struct sensor_data *data)
{
    if (!device_is_ready(pressure_dev)) return -1;
    if (sensor_sample_fetch(pressure_dev) < 0) return -1;

    struct sensor_value pressure;
    if (sensor_channel_get(pressure_dev, SENSOR_CHAN_PRESS, &pressure) < 0)
        return -1;

    data->sensor_id = SENSOR_TYPE_PRESSURE;
    data->value = sensor_to_float(&pressure);
    data->timestamp = k_uptime_get();

    return 0;
}

/**
 * @brief Initialize the pressure sensor and its data thread
 *
 * @return 0 on success, -1 if device is not ready
 *
 * @details
 * - Checks if the LPS22HB device is ready.
 * - Creates a Zephyr thread that periodically reads pressure data
 *   and enqueues it to the logger.
 */
int pressure_sensor_init(void)
{
    if (!device_is_ready(pressure_dev)) {
        LOG_ERR("Pressure sensor not ready");
        return -1;
    }

    static struct k_thread pressure_thread_data;
    static K_THREAD_STACK_DEFINE(pressure_stack, 1024);
    k_thread_create(&pressure_thread_data, pressure_stack,
                    K_THREAD_STACK_SIZEOF(pressure_stack),
                    pressure_thread, NULL, NULL, NULL,
                    5, 0, K_NO_WAIT);

    return 0;
}

/**
 * @brief Thread function for periodic pressure data acquisition
 *
 * @param a Unused
 * @param b Unused
 * @param c Unused
 *
 * @details
 * - Runs indefinitely, fetching pressure data every 3 seconds.
 * - Enqueues the sensor data to the logger module.
 */
void pressure_thread(void *a, void *b, void *c)
{
    struct sensor_data local;
    struct sensor_message msg;

    while (1) {
        if (pressure_sensor_get_data(&local) == 0) {
            msg.data = local;
            logger_enqueue(&msg);
        }
        k_sleep(K_SECONDS(3));
    }
}
