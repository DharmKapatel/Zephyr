/**
 * @file imu_sensor.c
 * @brief Inertial Measurement Unit (IMU) sensor interface using Zephyr sensor API.
 *
 * This module handles initialization, periodic sampling, and logging
 * of accelerometer and gyroscope data from an IMU sensor (e.g., LSM6DS3).
 *
 * The sampled data is packaged into `sensor_data` and pushed to the
 * logger message queue.
 *
 * @author Dharm Kapatel
 */

#include "imu_sensor.h"
#include "logger.h"
#include "struct.h"
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(imu);

#if DT_NODE_EXISTS(DT_ALIAS(imu_sensor))
/** Sensor device handle retrieved from devicetree alias `imu_sensor`. */
const struct device *const imu_dev = DEVICE_DT_GET(DT_ALIAS(imu_sensor));
#else
#error "IMU sensor not found."
#endif

/**
 * @brief Fetch latest IMU accelerometer and gyroscope data.
 *
 * Reads accelerometer (X, Y, Z) and gyroscope (X, Y, Z) values from
 * the IMU sensor and fills the provided `imu_data` structure.
 *
 * @param[out] imu Pointer to user-provided struct to hold IMU readings.
 *
 * @retval 0 Success
 * @retval -1 Failure (device not ready or sensor read error)
 */
int imu_sensor_get_data(struct imu_data *imu)
{
    if (!device_is_ready(imu_dev)) {
        return -1;
    }

    if (sensor_sample_fetch(imu_dev) < 0) {
        return -1;
    }

    struct sensor_value accel[3], gyro[3];

    if (sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_XYZ, accel) < 0) return -1;
    if (sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_XYZ, gyro) < 0) return -1;

    imu->ax = sensor_to_float(&accel[0]);
    imu->ay = sensor_to_float(&accel[1]);
    imu->az = sensor_to_float(&accel[2]);

    imu->gx = sensor_to_float(&gyro[0]);
    imu->gy = sensor_to_float(&gyro[1]);
    imu->gz = sensor_to_float(&gyro[2]);

    return 0;
}

/**
 * @brief Initialize IMU sensor thread.
 *
 * Ensures the IMU device is ready and starts a dedicated thread
 * for periodic sampling and logging of accelerometer and gyroscope data.
 *
 * @retval 0 Success
 * @retval -1 Failure (device not ready)
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
 * @brief Thread function for periodic IMU sampling.
 *
 * This thread runs indefinitely, fetching accelerometer and gyroscope
 * data every 4 seconds from the IMU sensor and enqueueing it to
 * the logger module.
 *
 * @param a Unused
 * @param b Unused
 * @param c Unused
 */
void imu_thread(void *a, void *b, void *c)
{
    struct imu_data imu_local;
    struct sensor_message msg;

    while (1) {
        if (imu_sensor_get_data(&imu_local) == 0) {
            msg.data.sensor_id   = SENSOR_TYPE_IMU;
            msg.data.timestamp   = k_uptime_get();
            msg.data.imu         = imu_local;  /**< Store all IMU axis data */

            logger_enqueue(&msg);

            k_sleep(K_SECONDS(4));
        }
    }
}
