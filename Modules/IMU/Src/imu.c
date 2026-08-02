#include "imu.h"

IMU_Data_t g_imuData;

void IMU_Init(void)
{
    g_imuData.raw.gyro.gx      = 0.0f;
    g_imuData.raw.gyro.gy      = 0.0f;
    g_imuData.raw.gyro.gz      = 0.0f;
    g_imuData.raw.accel.ax     = 0.0f;
    g_imuData.raw.accel.ay     = 0.0f;
    g_imuData.raw.accel.az     = 0.0f;
    g_imuData.raw.temperature  = 0.0f;
    g_imuData.attitude.roll    = 0.0f;
    g_imuData.attitude.pitch   = 0.0f;
    g_imuData.attitude.yaw     = 0.0f;

    /* TODO: 初始化传感器硬件（SPI/I2C） */
    /* TODO: 配置传感器寄存器、校准 */
}

void IMU_Update(void)
{
    /* TODO: 读取传感器原始数据 */
    /* TODO: 姿态解算（Mahony / Madgwick） */
}

void IMU_GetAttitude(Attitude_t *att)
{
    if (att) { att->roll = g_imuData.attitude.roll; att->pitch = g_imuData.attitude.pitch; att->yaw = g_imuData.attitude.yaw; }
}

void IMU_GetGyro(Gyro_t *gyro)
{
    if (gyro) { gyro->gx = g_imuData.raw.gyro.gx; gyro->gy = g_imuData.raw.gyro.gy; gyro->gz = g_imuData.raw.gyro.gz; }
}

void IMU_GetAccel(Accel_t *accel)
{
    if (accel) { accel->ax = g_imuData.raw.accel.ax; accel->ay = g_imuData.raw.accel.ay; accel->az = g_imuData.raw.accel.az; }
}