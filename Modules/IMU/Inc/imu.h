#ifndef __IMU_H__
#define __IMU_H__

#include "main.h"

typedef struct
{
    float roll;
    float pitch;
    float yaw;
} Attitude_t;

typedef struct
{
    float gx, gy, gz;
} Gyro_t;

typedef struct
{
    float ax, ay, az;
} Accel_t;

typedef struct
{
    Gyro_t   gyro;
    Accel_t  accel;
    float    temperature;
} IMU_RawData_t;

typedef struct
{
    IMU_RawData_t raw;
    Attitude_t    attitude;
} IMU_Data_t;

extern IMU_Data_t g_imuData;

void IMU_Init(void);
void IMU_Update(void);
void IMU_GetAttitude(Attitude_t *att);
void IMU_GetGyro(Gyro_t *gyro);
void IMU_GetAccel(Accel_t *accel);

#endif /* __IMU_H__ */