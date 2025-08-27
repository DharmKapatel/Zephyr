/**
 * @file pressure_sensor.c
 * @brief Pressure sensor driver integration with Zephyr and logger module.
 *
 * This module handles initialization and data acquisition from the
 * pressure sensor (LPS). Data is periodically sampled in a dedicated
 * thread, timestamped, and enqueued into the logger message queue.
 *
 * Workflow:
 * - pressure_sensor_init() starts the sampling thread.
 * - pressure_thread() fetches sensor data every 3 seconds.
 * - Data is wrapped in a sensor_message and pushed into press_msgq.
 *
 * Dependencies:
 * - Requires device tree alias: `pressure_sensor`.
 * - Uses Zephyr sensor API and logger queue.
 *
 * @author Dharm
 * @date 2025
 */

#include "pressure_sensor.h"
#include "logger.h"
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(pressure);

#if DT_NODE_EXISTS(DT_ALIAS(pressure_sensor))
/** @brief Zephyr device handle for the pressure sensor. */
const struct device *const pressure_dev = DEVICE_DT_GET(DT_ALIAS(pressure_sensor));
#else
#error "Pressure sensor not found."
#endif

/**
 * @brief Fetch pressure sensor data and store into shared buffer.
 *
 * Performs a sensor sample fetch, reads the pressure channel,
 * and updates the provided buffer with the latest value.
 *
 * @param[out] data Pointer to shared buffer where pressure is stored.
 * @return 0 on success, -1 on failure (device not ready or fetch error).
 */
int pressure_sensor_get_data(sensors_shared_buf *data)
{
    if (!device_is_ready(pressure_dev)) return -1;
    if (sensor_sample_fetch(pressure_dev) < 0) return -1;

    struct sensor_value press;
    if (sensor_channel_get(pressure_dev, SENSOR_CHAN_PRESS, &press) < 0)
        return -1;

    data->lps_data.pressure = sensor_to_float(&press);

    return 0;
}

/**
 * @brief Initialize the pressure sensor subsystem.
 *
 * - Verifies sensor readiness.
 * - Spawns a dedicated thread for periodic pressure sampling.
 *
 * @return 0 on success, -1 if device is not ready.
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
 * @brief Pressure sensor thread function.
 *
 * Periodically:
 * - Fetches latest pressure reading.
 * - Wraps result in sensor_message with timestamp.
 * - Pushes message into @ref press_msgq.
 *
 * @param a Unused.
 * @param b Unused.
 * @param c Unused.
 */
void pressure_thread(void *a, void *b, void *c)
{
    struct sensor_message msg;
    memset(&msg, 0, sizeof(msg));

    while (1) {
        if (pressure_sensor_get_data(&msg.data) == 0) {
            msg.timestamp = k_uptime_get();
            if (k_msgq_put(&press_msgq, &msg, K_NO_WAIT) != 0) {
                LOG_WRN("PRESS queue full, dropping");
            }
        }
        k_sleep(K_SECONDS(3));
    }
}
