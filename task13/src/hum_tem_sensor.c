/**
 * @file hum_temp_sensor.c
 * @brief Driver for the HTS221 humidity and temperature sensor
 *
 * This module initializes the HTS221 sensor, periodically reads
 * temperature and humidity data, and sends it to the logger.
 *
 * @details
 * - Uses Zephyr sensor API for sampling.
 * - Runs a dedicated thread to fetch and log sensor data.
 * - Converts sensor readings to float before storing.
 */

#include "hum_temp_sensor.h"
#include "logger.h"
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hum_temp); /**< Zephyr logging module registration */

/** 
 * @brief HTS221 device instance
 * 
 * Obtained via device tree alias "ht_sensor".
 */
#if DT_NODE_EXISTS(DT_ALIAS(ht_sensor))
const struct device *const hts_dev = DEVICE_DT_GET(DT_ALIAS(ht_sensor));
#else
#error "Humidity-Temperature sensor not found."
#endif

/**
 * @brief Fetches current sensor data
 * 
 * @param data Pointer to sensor_data structure to populate
 * @return 0 on success, -1 on failure
 * 
 * @details
 * - Fetches a sample from the HTS221 device.
 * - Converts the raw sensor value to float.
 * - Adds a timestamp using k_uptime_get().
 */
int hum_temp_sensor_get_data(struct sensor_data *data)
{
    if (!device_is_ready(hts_dev)) return -1;
    if (sensor_sample_fetch(hts_dev) < 0) return -1;

    struct sensor_value temp;
    if (sensor_channel_get(hts_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0)
        return -1;

    data->sensor_id = SENSOR_TYPE_HUM_TEMP;
    data->value = sensor_to_float(&temp);
    data->timestamp = k_uptime_get();

    return 0;
}

/**
 * @brief Initializes the HTS221 sensor and creates its data thread
 * 
 * @return 0 on success, -1 if device is not ready
 * 
 * @details
 * - Checks if the device is ready.
 * - Creates a Zephyr thread that periodically reads sensor data
 *   and enqueues it to the logger.
 */
int hum_temp_sensor_init(void)
{
    if (!device_is_ready(hts_dev)) {
        LOG_ERR("HTS221 device not ready");
        return -1;
    }

    // Create the thread
    static struct k_thread hum_temp_thread_data;
    static K_THREAD_STACK_DEFINE(hum_temp_stack, 1024);
    k_thread_create(&hum_temp_thread_data, hum_temp_stack,
                    K_THREAD_STACK_SIZEOF(hum_temp_stack),
                    hum_temp_thread, NULL, NULL, NULL,
                    5, 0, K_NO_WAIT);

    return 0;
}

/**
 * @brief Thread function for fetching and logging HTS221 data
 * 
 * @param a Not used
 * @param b Not used
 * @param c Not used
 * 
 * @details
 * - Runs indefinitely, fetching data every 2 seconds.
 * - Enqueues the sensor data to the logger module.
 */
void hum_temp_thread(void *a, void *b, void *c)
{
    struct sensor_data local;
    struct sensor_message msg;

    while (1) {
        if (hum_temp_sensor_get_data(&local) == 0) {
            msg.data = local;
            logger_enqueue(&msg);
        }
        k_sleep(K_SECONDS(2));
    }
}
