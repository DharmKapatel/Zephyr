#ifndef HUM_TEMP_SENSOR_H
#define HUM_TEMP_SENSOR_H

#include "struct.h"

int hum_temp_sensor_init(void);
int hum_temp_sensor_get_data(struct sensor_data *data);
void hum_temp_thread(void *a, void *b, void *c);

#endif /* HUM_TEMP_SENSOR_H */
