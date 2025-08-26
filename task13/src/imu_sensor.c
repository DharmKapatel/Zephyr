/**
 * @file imu_sensor.c
 * @brief Driver for LSM6DSL IMU sensor
 *
 * This module initializes the LSM6DSL IMU sensor, periodically reads
 * accelerometer data, and sends it to the logger.
 *
 * @details
 * - Uses Zephyr sensor API for sampling.
 * - Runs a dedicated thread to fetch and log sensor data.
 * - Currently only reads the X-axis for simplicity.
 */

#include "imu_sensor.h"
#include "logger.h"
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(imu); /**< Zephyr logging module registration */

/** 
 * @brief IMU device instance
 * 
 * Obtained via device tree alias "imu_sensor".
 */
#if DT_NODE_EXISTS(DT_ALIAS(imu_sensor))
const struct device *const imu_dev = DEVICE_DT_GET(DT_ALIAS(imu_sensor));
#else
#error "IMU sensor not found."
#endif

/**
 * @brief Fetches current IMU sensor data
 *
 * @param data Pointer to sensor_data structure to populate
 * @return 0 on success, -1 on failure
 *
 * @details
 * - Fetches accelerometer data from LSM6DSL.
 * - Converts the X-axis value to float.
 * - Adds a timestamp using k_uptime_get().
 */
int imu_sensor_get_data(struct sensor_data *data)
{
    if (!device_is_ready(imu_dev)) return -1;

    struct sensor_value ax;
    if (sensor_sample_fetch_chan(imu_dev, SENSOR_CHAN_ACCEL_XYZ) < 0) return -1;
    if (sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_X, &ax) < 0) return -1;

    data->sensor_id = SENSOR_TYPE_IMU;
    data->value = sensor_to_float(&ax); // Simplified X-axis
    data->timestamp = k_uptime_get();

    return 0;
}

/**
 * @brief Initializes the IMU sensor and creates its data thread
 *
 * @return 0 on success, -1 if device is not ready
 *
 * @details
 * - Checks if the LSM6DSL device is ready.
 * - Creates a Zephyr thread that periodically reads IMU data
 *   and enqueues it to the logger.
 */
int imu_sensor_init(void)
{
    if (!device_is_ready(imu_dev)) {
        LOG_ERR("IMU sensor not ready");
        return -1;
    }

    static struct k_thread imu_thread_data;
    static K_THREAD_STACK_DEFINE(imu_stack, 1024);
    k_thread_create(&imu_thread_data, imu_stack,
                    K_THREAD_STACK_SIZEOF(imu_stack),
                    imu_thread, NULL, NULL, NULL,
                    5, 0, K_NO_WAIT);

    return 0;
}

/**
 * @brief Thread function for fetching and logging IMU data
 *
 * @param a Unused
 * @param b Unused
 * @param c Unused
 *
 * @details
 * - Runs indefinitely, fetching accelerometer data every 4 seconds.
 * - Enqueues the sensor data to the logger module.
 */
void imu_thread(void *a, void *b, void *c)
{
    struct sensor_data local;
    struct sensor_message msg;

    while (1) {
        if (imu_sensor_get_data(&local) == 0) {
            msg.data = local;
            logger_enqueue(&msg);
        }
        k_sleep(K_SECONDS(4));
    }
}
