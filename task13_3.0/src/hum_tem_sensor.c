/**
 * @file hum_temp_sensor.c
 * @brief Humidity and Temperature sensor interface using Zephyr sensor API.
 *
 * This module handles initialization, periodic sampling, and logging
 * of humidity and temperature data from the HTS221 (or compatible) sensor.
 *
 * The sampled data is packaged into `sensor_data` and pushed to the
 * logger message queue.
 *
 * @author Dharm Kapatel
 */

#include "hum_temp_sensor.h"
#include "logger.h"
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hum_temp);

#if DT_NODE_EXISTS(DT_ALIAS(ht_sensor))
/** Sensor device handle retrieved from devicetree alias `ht_sensor`. */
const struct device *const hts_dev = DEVICE_DT_GET(DT_ALIAS(ht_sensor));
#else
#error "Humidity-Temperature sensor not found."
#endif

/**
 * @brief Fetch latest humidity and temperature sensor data.
 *
 * Reads values from the HTS221 sensor and fills the provided
 * `sensor_data` structure with temperature, humidity, and timestamp.
 *
 * @param[out] data Pointer to user-provided struct to hold sensor data.
 *
 * @retval 0 Success
 * @retval -1 Failure (device not ready or sensor read error)
 */
int hum_temp_sensor_get_data(struct sensor_data *data)
{
    if (!device_is_ready(hts_dev)) return -1;
    if (sensor_sample_fetch(hts_dev) < 0) return -1;

    struct sensor_value temp, hum;

    if (sensor_channel_get(hts_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0)
        return -1;
    if (sensor_channel_get(hts_dev, SENSOR_CHAN_HUMIDITY, &hum) < 0)
        return -1;

    data->sensor_id = SENSOR_TYPE_HUM_TEMP;
    data->hum_temp.temp = sensor_to_float(&temp);
    data->hum_temp.hum  = sensor_to_float(&hum);
    data->timestamp = k_uptime_get();

    return 0;
}

/**
 * @brief Initialize humidity-temperature sensor thread.
 *
 * Ensures the device is ready and starts a dedicated thread
 * for periodic sampling and logging of sensor data.
 *
 * @retval 0 Success
 * @retval -1 Failure (device not ready)
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
 * @brief Thread function for periodic sampling.
 *
 * This thread runs indefinitely, fetching data every 2 seconds
 * from the humidity-temperature sensor and enqueueing it to
 * the logger module.
 *
 * @param a Unused
 * @param b Unused
 * @param c Unused
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
