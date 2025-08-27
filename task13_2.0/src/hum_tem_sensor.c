/**
 * @file hum_temp_sensor.c
 * @author Dharm Kapatel
 * @brief Humidity and Temperature sensor handling module
 * @version 0.1
 * @date 2025-08-27
 *
 * This module initializes the humidity-temperature sensor (HTS),
 * reads sensor data, and runs a dedicated thread to fetch values
 * periodically and send them to a message queue.
 */

#include "hum_temp_sensor.h"
#include "logger.h"
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(hum_temp);

#if DT_NODE_EXISTS(DT_ALIAS(ht_sensor))
/**
 * @brief Humidity-Temperature device handle
 */
const struct device *const hts_dev = DEVICE_DT_GET(DT_ALIAS(ht_sensor));
#else
#error "Humidity-Temperature sensor not found."
#endif

/**
 * @brief Fetch humidity and temperature data from the sensor
 *
 * @param data Pointer to shared sensor buffer to store readings
 * @return int 0 on success, -1 on failure
 */
int hum_temp_sensor_get_data(sensors_shared_buf *data)
{
    if (!device_is_ready(hts_dev)) return -1;
    if (sensor_sample_fetch(hts_dev) < 0) return -1;

    struct sensor_value temp, hum;

    if (sensor_channel_get(hts_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0)
        return -1;
    if (sensor_channel_get(hts_dev, SENSOR_CHAN_HUMIDITY, &hum) < 0)
        return -1;

    /* sensor_to_float returns float; struct fields are float */
    data->hts_data.temperature = sensor_to_float(&temp);
    data->hts_data.humidity    = sensor_to_float(&hum);

    return 0;
}

/**
 * @brief Initialize the humidity-temperature sensor and start its thread
 *
 * @return int 0 on success, -1 on failure
 */
int hum_temp_sensor_init(void)
{
    if (!device_is_ready(hts_dev)) {
        LOG_ERR("HTS221 device not ready");
        return -1;
    }

    static struct k_thread hum_temp_thread_data;
    static K_THREAD_STACK_DEFINE(hum_temp_stack, 1024);
    k_thread_create(&hum_temp_thread_data, hum_temp_stack,
                    K_THREAD_STACK_SIZEOF(hum_temp_stack),
                    hum_temp_thread, NULL, NULL, NULL,
                    5, 0, K_NO_WAIT);

    return 0;
}

/**
 * @brief Thread function for periodic humidity-temperature sampling
 *
 * This thread continuously fetches data from the HTS sensor,
 * attaches a timestamp, and pushes the result to the message queue.
 *
 * @param a Unused
 * @param b Unused
 * @param c Unused
 */
void hum_temp_thread(void *a, void *b, void *c)
{
    struct sensor_message msg;
    memset(&msg, 0, sizeof(msg));

    while (1) {
        if (hum_temp_sensor_get_data(&msg.data) == 0) {
            msg.timestamp = k_uptime_get();
            if (k_msgq_put(&hts_msgq, &msg, K_NO_WAIT) != 0) {
                LOG_WRN("HTS queue full, dropping");
            }
        }
        k_sleep(K_SECONDS(2));
    }
}
