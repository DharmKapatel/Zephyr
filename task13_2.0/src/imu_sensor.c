/**
 * @file imu_sensor.c
 * @author Dharm Kapatel
 * @brief IMU (Accelerometer + Gyroscope) sensor handling module
 * @version 0.1
 * @date 2025-08-27
 *
 * This module initializes the IMU sensor, fetches accelerometer and gyroscope
 * readings, and runs a dedicated thread to periodically push the values into
 * a message queue for further processing.
 */

#include "imu_sensor.h"
#include "logger.h"
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(imu);

#if DT_NODE_EXISTS(DT_ALIAS(imu_sensor))
/**
 * @brief IMU device handle
 */
const struct device *const imu_dev = DEVICE_DT_GET(DT_ALIAS(imu_sensor));
#else
#error "IMU sensor not found."
#endif

/**
 * @brief Fetch accelerometer and gyroscope data from the IMU sensor
 *
 * Reads 3-axis accelerometer and gyroscope values and stores them in
 * the shared buffer.
 *
 * @param data Pointer to shared buffer where sensor data is stored
 * @return int 0 on success, -1 on failure
 */
int imu_sensor_get_data(sensors_shared_buf *data)
{
    if (!device_is_ready(imu_dev)) return -1;
    if (sensor_sample_fetch(imu_dev) < 0) return -1;

    struct sensor_value accel[3], gyro[3];

    if (sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_XYZ, accel) < 0) return -1;
    if (sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_XYZ, gyro) < 0) return -1;

    data->imu_data.accel.x = sensor_to_float(&accel[0]);
    data->imu_data.accel.y = sensor_to_float(&accel[1]);
    data->imu_data.accel.z = sensor_to_float(&accel[2]);

    data->imu_data.gyro.x = sensor_to_float(&gyro[0]);
    data->imu_data.gyro.y = sensor_to_float(&gyro[1]);
    data->imu_data.gyro.z = sensor_to_float(&gyro[2]);

    return 0;
}

/**
 * @brief Initialize the IMU sensor and start its thread
 *
 * This function checks IMU readiness and spawns the sampling thread.
 *
 * @return int 0 on success, -1 on failure
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
 * @brief Thread function for periodic IMU sampling
 *
 * This thread continuously fetches accelerometer and gyroscope data,
 * attaches a timestamp, and pushes the result to the IMU message queue.
 *
 * @param a Unused
 * @param b Unused
 * @param c Unused
 */
void imu_thread(void *a, void *b, void *c)
{
    struct sensor_message msg;
    memset(&msg, 0, sizeof(msg));

    while (1) {
        if (imu_sensor_get_data(&msg.data) == 0) {
            msg.timestamp = k_uptime_get();
            if (k_msgq_put(&imu_msgq, &msg, K_NO_WAIT) != 0) {
                LOG_WRN("IMU queue full, dropping");
            }
        }
        k_sleep(K_SECONDS(4));
    }
}
