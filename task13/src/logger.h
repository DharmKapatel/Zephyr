#ifndef LOGGER_H
#define LOGGER_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include "struct.h"

static inline float sensor_to_float(const struct sensor_value *val)
{
    return (float)val->val1 + (float)val->val2 / 1000000.0f;
}

#define MAX_QUEUE 50
#define MAX_ENTRIES 1000

void logger_init(void);
void logger_enqueue(const struct sensor_message *msg);

#endif /* LOGGER_H */
