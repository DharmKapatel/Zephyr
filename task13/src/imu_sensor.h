#ifndef IMU_SENSOR_H
#define IMU_SENSOR_H

#include "struct.h"

int imu_sensor_init(void);
int imu_sensor_get_data(struct imu_data *imu);
void imu_thread(void *a, void *b, void *c);

#endif /* IMU_SENSOR_H */
