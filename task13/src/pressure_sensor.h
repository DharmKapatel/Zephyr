#ifndef PRESSURE_SENSOR_H
#define PRESSURE_SENSOR_H

#include "struct.h"

int pressure_sensor_init(void);
int pressure_sensor_get_data(struct sensor_data *data);
void pressure_thread(void *a, void *b, void *c);

#endif /* PRESSURE_SENSOR_H */
