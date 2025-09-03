/**
 * @file pressure_sensor.c
 * @brief Pressure sensor driver wrapper with background thread for logging system.
 *
 * This module initializes a pressure sensor device (defined in DTS as
 * `pressure_sensor` alias), periodically fetches readings in a background
 * thread, and forwards them to the logger.
 *
 * Author: Dharm Kapatel
 */

#include "pressure_sensor.h"
#include "logger.h"

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pressure, LOG_LEVEL_INF);

#if DT_NODE_EXISTS(DT_ALIAS(pressure_sensor))
/** Pressure sensor device instance */
static const struct device *const pressure_dev = DEVICE_DT_GET(DT_ALIAS(pressure_sensor));
#else
#error "Pressure sensor not found. Please define alias 'pressure_sensor' in DTS."
#endif

/** @brief Pressure sensor thread stack size (bytes). */
#define PRESSURE_THREAD_STACK_SIZE 1024
/** @brief Pressure sensor thread priority. */
#define PRESSURE_THREAD_PRIORITY   5

/** Thread control block for pressure sensor thread */
static struct k_thread pressure_thread_data;
/** Thread stack allocation for pressure sensor thread */
static K_THREAD_STACK_DEFINE(pressure_stack, PRESSURE_THREAD_STACK_SIZE);

/**
 * @brief Fetch and fill pressure sensor data.
 *
 * This function samples the pressure sensor and fills a `struct sensor_data`
 * with the latest pressure value and timestamp.
 *
 * @param[out] data Pointer to structure where sensor data will be stored.
 *
 * @retval 0       Success
 * @retval -ENODEV Device not ready
 * @retval -EIO    Fetch or channel read failure
 */
int pressure_sensor_get_data(struct sensor_data *data)
{
    if (!device_is_ready(pressure_dev)) {
        LOG_ERR("Pressure device not ready");
        return -ENODEV;
    }

    if (sensor_sample_fetch(pressure_dev) < 0) {
        LOG_ERR("Pressure sample fetch failed");
        return -EIO;
    }

    struct sensor_value press;
    if (sensor_channel_get(pressure_dev, SENSOR_CHAN_PRESS, &press) < 0) {
        LOG_ERR("Pressure channel read failed");
        return -EIO;
    }

    data->sensor_id = SENSOR_TYPE_PRESSURE;
    data->pressure.pressure = sensor_to_float(&press);
    data->timestamp = k_uptime_get();

    return 0;
}

/**
 * @brief Initialize the pressure sensor module.
 *
 * Verifies that the device is ready and starts the pressure sampling thread.
 *
 * @retval 0       Success
 * @retval -ENODEV Device not ready
 */
int pressure_sensor_init(void)
{
    if (!device_is_ready(pressure_dev)) {
        LOG_ERR("Pressure sensor not ready during init");
        return -ENODEV;
    }

    k_thread_create(&pressure_thread_data, pressure_stack,
                    K_THREAD_STACK_SIZEOF(pressure_stack),
                    pressure_thread, NULL, NULL, NULL,
                    PRESSURE_THREAD_PRIORITY, 0, K_NO_WAIT);

    k_thread_name_set(&pressure_thread_data, "pressure_thread");
    

    return 0;
}

/**
 * @brief Background thread for pressure sensor data collection.
 *
 * This thread periodically fetches sensor data and enqueues it into the
 * logger system.
 *
 * @param a Unused
 * @param b Unused
 * @param c Unused
 */
void pressure_thread(void *a, void *b, void *c)
{
    ARG_UNUSED(a);
    ARG_UNUSED(b);
    ARG_UNUSED(c);

    struct sensor_data local;
    struct sensor_message msg;

    while (1) {
        if (pressure_sensor_get_data(&local) == 0) {
            msg.data = local;
            logger_enqueue(&msg);
        } else {
            LOG_WRN("Pressure read failed, retrying...");
        }
        k_sleep(K_SECONDS(3));
    }
}
