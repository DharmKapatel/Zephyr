#include "pressure_sensor.h"
#include "logger.h"
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pressure);

#if DT_NODE_EXISTS(DT_ALIAS(pressure_sensor))
const struct device *const pressure_dev = DEVICE_DT_GET(DT_ALIAS(pressure_sensor));
#else
#error "Pressure sensor not found."
#endif

int pressure_sensor_get_data(struct sensor_data *data)
{
    if (!device_is_ready(pressure_dev)) return -1;
    if (sensor_sample_fetch(pressure_dev) < 0) return -1;

    struct sensor_value press;
    if (sensor_channel_get(pressure_dev, SENSOR_CHAN_PRESS, &press) < 0)
        return -1;

    data->sensor_id = SENSOR_TYPE_PRESSURE;
    data->pressure.pressure = sensor_to_float(&press);
    data->timestamp = k_uptime_get();

    return 0;
}

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
