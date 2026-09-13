#include "imu.h"
#include "i2c.h"
#include <stdio.h>
#include <math.h>

extern I2C_HandleTypeDef hi2c1;extern I2C_HandleTypeDef hi2c1;

#define MPU9250_ADDR        0x69
#define MPU9250_ADDR_HAL    (MPU9250_ADDR << 1)

#define REG_SMPLRT_DIV      0x19
#define REG_CONFIG          0x1A
#define REG_GYRO_CONFIG     0x1B
#define REG_ACCEL_CONFIG    0x1C
#define REG_INT_PIN_CFG     0x37
#define REG_ACCEL_XOUT_H    0x3B
#define REG_PWR_MGMT_1      0x6B
#define REG_WHO_AM_I        0x75

#define ACC_LSB_PER_G       4096.0f    /* ±8g */
#define GYRO_LSB_PER_DPS    16.4f      /* ±2000 dps */
#define RAD2DEG             57.2957795f

IMU_Data_t g_imuData;
static float gyro_bias[3];

static uint8_t MPU_Read(uint8_t reg)
{
    uint8_t v = 0;
    HAL_I2C_Mem_Read(&hi2c1, MPU9250_ADDR_HAL, reg, 1, &v, 1, 100);
    return v;
}

static void MPU_Write(uint8_t reg, uint8_t val)
{
    HAL_I2C_Mem_Write(&hi2c1, MPU9250_ADDR_HAL, reg, 1, &val, 1, 100);
}

/* 上电时飞机必须静止。连采 200 次陀螺，取平均作为零偏。
 * 静止时理想输出为 0，实际会有固定偏差；后续 IMU_Update 会减去该值，
 * 否则积分 yaw / 互补滤波会一直往一个方向飘。 */
static void Gyro_Calibrate(void)
{
    int32_t sx = 0, sy = 0, sz = 0;
    uint8_t buf[14];

    for (int i = 0; i < 200; i++) {
        /* 从 0x3B 连读 14 字节：accel(6) + temp(2) + gyro(6)，陀螺在 buf[8..13] */
        HAL_I2C_Mem_Read(&hi2c1, MPU9250_ADDR_HAL, REG_ACCEL_XOUT_H, 1, buf, 14, 100);
        sx += (int16_t)((buf[8] << 8) | buf[9]);
        sy += (int16_t)((buf[10] << 8) | buf[11]);
        sz += (int16_t)((buf[12] << 8) | buf[13]);
        HAL_Delay(2);
    }

    /* 原始 LSB 平均后再换成 °/s，与 IMU_Update 中的单位一致 */
    gyro_bias[0] = (sx / 200.0f) / GYRO_LSB_PER_DPS;
    gyro_bias[1] = (sy / 200.0f) / GYRO_LSB_PER_DPS;
    gyro_bias[2] = (sz / 200.0f) / GYRO_LSB_PER_DPS;
}

void IMU_Init(void)
{
    HAL_Delay(100);
    MPU_Write(REG_PWR_MGMT_1, 0x80);
    HAL_Delay(100);

    uint8_t id = MPU_Read(REG_WHO_AM_I);
    printf("MPU9250 WHO_AM_I=0x%02X (expect 0x71)\r\n", id);

    MPU_Write(REG_PWR_MGMT_1, 0x01);   /* 唤醒 + PLL */
    HAL_Delay(10);
    MPU_Write(REG_GYRO_CONFIG,  0x18); /* ±2000 dps */
    MPU_Write(REG_ACCEL_CONFIG, 0x10); /* ±8 g */
    MPU_Write(REG_CONFIG,       0x03); /* DLPF 41 Hz */
    MPU_Write(REG_SMPLRT_DIV,   0x00); /* 1 kHz */
    //MPU_Write(REG_INT_PIN_CFG,  0x02); /* BYPASS_EN，可选 */

    Gyro_Calibrate();
}

/* 1 ms 周期调用：读 MPU9250 → 换成物理单位 → 互补滤波得到姿态。
 * dt 必须与 StartDefaultTask 的 osDelay(1) 一致。 */
void IMU_Update(void)
{
    uint8_t buf[14];
    const float dt = 0.001f;     /* 与 1 ms 任务周期对应，单位 s */
    const float alpha = 0.98f;   /* 互补滤波：越接近 1 越信陀螺（动态好，慢漂） */

    /* 从 ACCEL_XOUT_H 连读 14 字节：ax ay az | temp | gx gy gz，大端有符号 16 位 */
    if (HAL_I2C_Mem_Read(&hi2c1, MPU9250_ADDR_HAL, REG_ACCEL_XOUT_H,
                         1, buf, 14, 100) != HAL_OK)
        return;

    int16_t ax = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t ay = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t az = (int16_t)((buf[4] << 8) | buf[5]);
    /* buf[6..7] 是温度，此处未用 */
    int16_t gx = (int16_t)((buf[8] << 8) | buf[9]);
    int16_t gy = (int16_t)((buf[10] << 8) | buf[11]);
    int16_t gz = (int16_t)((buf[12] << 8) | buf[13]);

    /* 原始 LSB → g 和 °/s，并减去上电标定的陀螺零偏 */
    float axg = ax / ACC_LSB_PER_G;
    float ayg = ay / ACC_LSB_PER_G;
    float azg = az / ACC_LSB_PER_G;
    float gxd = gx / GYRO_LSB_PER_DPS - gyro_bias[0];
    float gyd = gy / GYRO_LSB_PER_DPS - gyro_bias[1];
    float gzd = gz / GYRO_LSB_PER_DPS - gyro_bias[2];

    g_imuData.raw.accel.ax = axg;
    g_imuData.raw.accel.ay = ayg;
    g_imuData.raw.accel.az = azg;
    g_imuData.raw.gyro.gx  = gxd;
    g_imuData.raw.gyro.gy  = gyd;
    g_imuData.raw.gyro.gz  = gzd;

    /* 加速度估计重力方向，得到静态 roll/pitch（机动时不准，只作长期校正） */
    float acc_roll  = atan2f(ayg, azg) * RAD2DEG;
    float acc_pitch = atan2f(-axg, sqrtf(ayg * ayg + azg * azg)) * RAD2DEG;

    /* 互补滤波：陀螺积分跟动态，加速度拉回水平，抑制陀螺慢漂 */
    g_imuData.attitude.roll  = alpha * (g_imuData.attitude.roll  + gxd * dt)
                             + (1.0f - alpha) * acc_roll;
    g_imuData.attitude.pitch = alpha * (g_imuData.attitude.pitch + gyd * dt)
                             + (1.0f - alpha) * acc_pitch;
    /* yaw 暂无磁力计，只能陀螺积分，会随时间漂移 */
    g_imuData.attitude.yaw  += gzd * dt;
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