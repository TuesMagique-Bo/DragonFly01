# SPL06 气压计编程教程

> 基于 DragonFly V2 飞控硬件平台，STM32F411CEU6 + I2C1 通信（与 MPU9250 共享总线）

---

## 目录

1. [SPL06 简介](#1-spl06-简介)
2. [硬件连接](#2-硬件连接)
3. [I2C 通信基础](#3-i2c-通信基础)
4. [寄存器速查表](#4-寄存器速查表)
5. [初始化流程](#5-初始化流程)
6. [数据读取与换算](#6-数据读取与换算)
7. [完整代码示例](#7-完整代码示例)
8. [高度估算与校准](#8-高度估算与校准)
9. [常见问题](#9-常见问题)

---

## 1. SPL06 简介

SPL06-001 是歌尔微电子推出的**高精度气压传感器**，内部集成 24 位 ADC，用于测量大气压力和温度，是飞控高度控制的必备传感器。

**核心特性：**

| 特性 | 参数 |
|------|------|
| 气压测量范围 | 300 - 1200 hPa |
| 气压分辨率 | 0.06 Pa（~0.5 cm 高度） |
| 相对精度 | ±6 Pa（~±50 cm） |
| 温度分辨率 | 0.005°C |
| 测量速率 | 最高 200 Hz |
| 通信接口 | I2C（最高 400kHz）或 SPI |
| 工作电压 | 1.7 - 3.6V |
| 工作电流 | 5 μA（待机） / 60 μA（测量） |
| FIFO | 32 采样深度 |
| 过采样率 | 1x / 2x / 4x / 8x / 16x / 32x / 64x / 128x |

**飞控中的作用：**

- **高度保持**：实时测量气压变化，计算当前高度，PID 控制油门
- **定高悬停**：配合加速度计，实现稳定的高度控制
- **起飞/降落检测**：通过气压变化率判断飞行状态

---

## 2. 硬件连接

### 2.1 引脚连接

SPL06 与 MPU9250 共享 **I2C1 总线**：

```
STM32F411CEU6          SPL06 (U?)
─────────────────────────────────────
PB6 (I2C1_SCL) ──────── Pin ? (SCL)
PB7 (I2C1_SDA) ──────── Pin ? (SDA)
```

### 2.2 I2C 地址

SPL06 的 7 位 I2C 地址由 SDO 引脚决定：

| SDO 引脚 | 7 位地址 | HAL 库地址 |
|----------|----------|-----------|
| SDO = 0（接地） | 0x76 | 0xEC |
| SDO = 1（接 VCC） | 0x77 | 0xEE |

> 请以实际原理图为准确认 SDO 引脚电平。HAL 库使用时需要左移 1 位。

### 2.3 I2C1 总线设备一览

| 设备 | 7 位地址 | 用途 |
|------|----------|------|
| MPU9250 | 0x69 | 9 轴 IMU |
| SPL06 | 0x76 或 0x77 | 气压 + 温度 |
| AK8963 | 0x0C | 磁力计（通过 MPU9250 旁路） |

> 三个设备地址不冲突，可以安全共享同一总线。

---

## 3. I2C 通信基础

SPL06 的 I2C 读写与 MPU9250 完全一致，使用 HAL 库的标准接口：

```c
/* 读寄存器 */
HAL_I2C_Mem_Read(&hi2c1, dev_addr, reg_addr, 1, buf, len, timeout);

/* 写寄存器 */
HAL_I2C_Mem_Write(&hi2c1, dev_addr, reg_addr, 1, &data, 1, timeout);
```

**注意事项：**
- `dev_addr` 需要左移 1 位：`SPL06_ADDR << 1`
- 气压/温度数据为 24 位，需要连续读取 3 个字节
- 建议在 I2C 操作前后加超时处理，防止总线卡死

---

## 4. 寄存器速查表

### 4.1 核心寄存器

| 地址 | 名称 | 功能 | 说明 |
|------|------|------|------|
| **0x00** | PSR_B2 | 气压原始值[23:16] | 读数前先读此寄存器 |
| **0x01** | PSR_B1 | 气压原始值[15:8] | |
| **0x02** | PSR_B0 | 气压原始值[7:0] | 读此寄存器触发下一次测量 |
| **0x03** | TMP_B2 | 温度原始值[23:16] | 读数前先读此寄存器 |
| **0x04** | TMP_B1 | 温度原始值[15:8] | |
| **0x05** | TMP_B0 | 温度原始值[7:0] | 读此寄存器触发下一次测量 |
| **0x06** | PRS_CFG | 气压配置 | [3:0]过采样率, [4]保留 |
| **0x07** | TMP_CFG | 温度配置 | [3:0]过采样率, [6:4]测量速率, [7]温度测量使能 |
| **0x08** | MEAS_CFG | 测量配置 | [0]气压测量模式, [1]温度测量模式, [2:3]工作模式, [7]FIFO 使能 |
| **0x09** | CFG_REG | 中断/FIFO 配置 | [0]FIFO 模式, [1]SPI 模式, [2]中断极性, [3]中断使能, [4]数据就绪, [5]FIFO 满, [7]移位 |
| **0x0A** | INT_STS | 中断状态 | [0]FIFO 满, [1]FIFO 水位, [2]数据就绪 |
| **0x0B** | FIFO_STS | FIFO 状态 | [7:0]FIFO 数据计数 |
| **0x0C** | RESET | 软复位 | 写 0x89 执行软复位，完成后自动清零 |
| **0x0D** | ID | 产品 ID | 固定值 **0x10**（可用于验证通信） |
| **0x10-0x20** | COEF | 校准系数 | 18 个字节，出厂校准数据（见 4.2 节） |

### 4.2 校准系数（COEF）

SPL06 每颗芯片出厂时都烧录了校准系数，存储在 0x10-0x21 共 18 个字节：

| 地址 | 系数 | 类型 | 说明 |
|------|------|------|------|
| 0x10-0x11 | c0 | int16 | 气压补偿系数 0 |
| 0x12-0x13 | c1 | int16 | 气压补偿系数 1 |
| 0x14-0x17 | c00 | int32 | 气压补偿系数 00 |
| 0x18-0x19 | c10 | int16 | 气压补偿系数 10 |
| 0x1A-0x1B | c01 | int16 | 气压补偿系数 01 |
| 0x1C-0x1D | c11 | int16 | 气压补偿系数 11 |
| 0x1E-0x1F | c20 | int16 | 气压补偿系数 20 |
| 0x20-0x21 | c21 | int16 | 气压补偿系数 21 |

> **重要：** 这些系数是出厂校准值，必须正确读取才能得到准确的气压和温度。

### 4.3 过采样率与转换时间

| 过采样率 | PRS/TMP_CFG[3:0] | 气压转换时间 | 精度 |
|----------|-------------------|-------------|------|
| 1x | 0x00 | 3.6 ms | 最低 |
| 2x | 0x01 | 5.2 ms | - |
| 4x | 0x02 | 8.4 ms | 中等 |
| 8x | 0x03 | 14.8 ms | **推荐** |
| 16x | 0x04 | 27.6 ms | 高 |
| 32x | 0x05 | 53.2 ms | - |
| 64x | 0x06 | 104.4 ms | 高精度 |
| 128x | 0x07 | 206.8 ms | 最高精度 |

> 飞控推荐：气压 8x / 温度 4x，兼顾速度和精度。

### 4.4 测量模式

| MEAS_CFG[2:0] | 模式 |
|----------------|------|
| 000 | 空闲 / 待机 |
| 001 | 气压测量（单次） |
| 010 | 温度测量（单次） |
| 011 | 温度 + 气压（单次） |
| 100 | 持续气压测量 |
| 101 | 持续温度测量 |
| 110 | 持续温度 + 气压测量 |
| 111 | FIFO 模式 |

---

## 5. 初始化流程

### 5.1 CubeMX 配置

SPL06 与 MPU9250 共享 I2C1，如果已配置 I2C1，**无需额外配置**。

如果还没配置 I2C1，参考以下参数：

| 参数 | 设置值 |
|------|--------|
| **I2C Speed Mode** | Fast Mode |
| **I2C Speed Frequency** | 400 KHz |
| **Rise Time** | 100 |
| **Fall Time** | 10 |

### 5.2 初始化代码

```c
#include "main.h"
#include "spl06.h"
#include <stdio.h>

/* I2C 句柄（CubeMX 生成） */
extern I2C_HandleTypeDef hi2c1;

/* SPL06 I2C 地址（7 位，需根据实际 SDO 电平修改） */
#define SPL06_ADDR          0x76   // SDO = 0 → 0x76; SDO = 1 → 0x77
#define SPL06_ADDR_HAL      (SPL06_ADDR << 1)  // HAL 库用 8 位地址

/* 寄存器地址 */
#define SPL06_REG_PSR_B2    0x00
#define SPL06_REG_PSR_B1    0x01
#define SPL06_REG_PSR_B0    0x02
#define SPL06_REG_TMP_B2    0x03
#define SPL06_REG_TMP_B1    0x04
#define SPL06_REG_TMP_B0    0x05
#define SPL06_REG_PRS_CFG   0x06
#define SPL06_REG_TMP_CFG   0x07
#define SPL06_REG_MEAS_CFG  0x08
#define SPL06_REG_CFG_REG   0x09
#define SPL06_REG_INT_STS   0x0A
#define SPL06_REG_FIFO_STS  0x0B
#define SPL06_REG_RESET     0x0C
#define SPL06_REG_ID        0x0D
#define SPL06_REG_COEF      0x10   // 校准系数起始地址

/* 过采样率 */
#define SPL06_PRS_OVERSAMPLING  0x03  // 气压: 8x
#define SPL06_TMP_OVERSAMPLING  0x02  // 温度: 4x

/* 校准系数结构体 */
typedef struct {
    int16_t c0;
    int16_t c1;
    int32_t c00;
    int16_t c10;
    int16_t c01;
    int16_t c11;
    int16_t c20;
    int16_t c21;
} SPL06_Calib_t;

/* 传感器数据 */
typedef struct {
    float pressure;    // 气压 (Pa)
    float temperature; // 温度 (°C)
    float altitude;    // 高度 (m)
} SPL06_Data_t;

/* 全局变量 */
static SPL06_Calib_t g_calib;
static SPL06_Data_t  g_spl06;

/* ───────────────── I2C 读写封装 ───────────────── */

static uint8_t SPL06_ReadReg(uint8_t reg)
{
    uint8_t val;
    HAL_I2C_Mem_Read(&hi2c1, SPL06_ADDR_HAL, reg, 1, &val, 1, 10);
    return val;
}

static void SPL06_WriteReg(uint8_t reg, uint8_t val)
{
    HAL_I2C_Mem_Write(&hi2c1, SPL06_ADDR_HAL, reg, 1, &val, 1, 10);
}

static void SPL06_ReadBuf(uint8_t reg, uint8_t *buf, uint8_t len)
{
    HAL_I2C_Mem_Read(&hi2c1, SPL06_ADDR_HAL, reg, 1, buf, len, 10);
}

/* ───────────────── 软复位 ───────────────── */

static void SPL06_SoftReset(void)
{
    SPL06_WriteReg(SPL06_REG_RESET, 0x89);
    HAL_Delay(10);  // 等待复位完成
}

/* ───────────────── 读取校准系数 ───────────────── */

static void SPL06_ReadCalib(void)
{
    uint8_t buf[18];

    SPL06_ReadBuf(SPL06_REG_COEF, buf, 18);

    /* 注意：SPL06 的系数是大端存储 */
    g_calib.c0  = (int16_t)((buf[0]  << 8) | buf[1]);
    g_calib.c1  = (int16_t)((buf[2]  << 8) | buf[3]);
    g_calib.c00 = (int32_t)((buf[4]  << 24) | (buf[5]  << 16) |
                            (buf[6]  << 8)  |  buf[7]);
    g_calib.c10 = (int16_t)((buf[8]  << 8) | buf[9]);
    g_calib.c01 = (int16_t)((buf[10] << 8) | buf[11]);
    g_calib.c11 = (int16_t)((buf[12] << 8) | buf[13]);
    g_calib.c20 = (int16_t)((buf[14] << 8) | buf[15]);
    g_calib.c21 = (int16_t)((buf[16] << 8) | buf[17]);
}

/* ───────────────── 初始化 ───────────────── */

uint8_t SPL06_Init(void)
{
    uint8_t id;

    /* 1. 软复位 */
    SPL06_SoftReset();

    /* 2. 读取 ID 验证通信 */
    id = SPL06_ReadReg(SPL06_REG_ID);
    if (id != 0x10) {
        printf("SPL06 ID error: 0x%02X (expected 0x10)\r\n", id);
        return 0;
    }

    /* 3. 读取出厂校准系数 */
    SPL06_ReadCalib();

    /* 4. 配置气压过采样率：8x */
    SPL06_WriteReg(SPL06_REG_PRS_CFG, SPL06_PRS_OVERSAMPLING);

    /* 5. 配置温度过采样率：4x，启用温度测量 */
    SPL06_WriteReg(SPL06_REG_TMP_CFG, (1 << 7) | SPL06_TMP_OVERSAMPLING);
    // bit7 = 1 (使能温度测量), bit4 = 0 (内部测量源), [3:0] = 过采样率

    /* 6. 设置测量模式：持续温度 + 气压 */
    SPL06_WriteReg(SPL06_REG_MEAS_CFG, 0x07);
    // [2:0] = 111 = 持续温度 + 气压测量

    /* 7. 设置中断配置（可选，不使用中断时可以跳过） */
    // SPL06_WriteReg(SPL06_REG_CFG_REG, 0x00);  // 默认值

    printf("SPL06 init OK\r\n");
    return 1;
}
```

### 5.3 初始化要点

1. **必须软复位**：上电后先执行软复位，确保寄存器处于默认状态
2. **必须读 ID**：验证 I2C 通信是否正常
3. **必须读校准系数**：没有校准系数，气压值完全不准
4. **过采样率**：越大越准但越慢，建议 8x 起步
5. **持续模式**：设置 `MEAS_CFG = 0x07` 后，芯片会自动连续测量，无需每次触发

---

## 6. 数据读取与换算

### 6.1 读取原始数据

```c
static int32_t SPL06_ReadRawPressure(void)
{
    uint8_t buf[3];
    SPL06_ReadBuf(SPL06_REG_PSR_B2, buf, 3);
    return (int32_t)((buf[0] << 16) | (buf[1] << 8) | buf[2]);
}

static int32_t SPL06_ReadRawTemperature(void)
{
    uint8_t buf[3];
    SPL06_ReadBuf(SPL06_REG_TMP_B2, buf, 3);
    return (int32_t)((buf[0] << 16) | (buf[1] << 8) | buf[2]);
}
```

### 6.2 补偿换算公式

这是 SPL06 数据手册的核心公式，将原始 ADC 值转换为实际物理量：

```c
static float SPL06_CompensateTemperature(int32_t raw_temp)
{
    float temp_raw_scaled;
    float temp_comp;

    /* 第一步：缩放原始温度值 */
    temp_raw_scaled = (float)raw_temp / 8.0f;

    /* 第二步：补偿 */
    temp_comp = (float)g_calib.c0 * 0.5f +
                (float)g_calib.c1 * temp_raw_scaled;

    return temp_comp;  // 单位：°C
}

static float SPL06_CompensatePressure(int32_t raw_press, float temp_comp)
{
    float press_raw_scaled;
    float press_comp;

    /* 第一步：缩放原始气压值 */
    press_raw_scaled = (float)raw_press / 8.0f;

    /* 第二步：补偿 */
    press_comp = (float)g_calib.c00 +
                 press_raw_scaled * ((float)g_calib.c10 +
                 press_raw_scaled * ((float)g_calib.c20 +
                 press_raw_scaled * (float)g_calib.c30)) +
                 temp_comp * (float)g_calib.c01 +
                 temp_comp * press_raw_scaled * ((float)g_calib.c11 +
                 press_raw_scaled * (float)g_calib.c21);

    return press_comp;  // 单位：Pa
}
```

### 6.3 一次读取完整数据

```c
void SPL06_Update(void)
{
    int32_t raw_press, raw_temp;
    float temp_comp;

    /* 读取原始值 */
    raw_temp = SPL06_ReadRawTemperature();
    raw_press = SPL06_ReadRawPressure();

    /* 温度补偿 */
    temp_comp = SPL06_CompensateTemperature(raw_temp);

    /* 气压补偿 */
    g_spl06.temperature = temp_comp;
    g_spl06.pressure    = SPL06_CompensatePressure(raw_press, temp_comp);
}
```

### 6.4 读取顺序

SPL06 数据手册强调：**必须先读温度，再读气压**。因为温度补偿系数需要用于气压计算，且读气压寄存器时会触发新的测量。

---

## 7. 完整代码示例

### 7.1 头文件 (`spl06.h`)

```c
#ifndef __SPL06_H__
#define __SPL06_H__

#include "main.h"

/* SPL06 数据 */
typedef struct {
    float pressure;    // 气压 (Pa)
    float temperature; // 温度 (°C)
    float altitude;    // 高度 (m)
} SPL06_Data_t;

/* 函数声明 */
uint8_t SPL06_Init(void);
void    SPL06_Update(void);
float   SPL06_GetPressure(void);
float   SPL06_GetTemperature(void);
float   SPL06_GetAltitude(void);

#endif /* __SPL06_H__ */
```

### 7.2 源文件 (`spl06.c`)

```c
#include "spl06.h"
#include <stdio.h>
#include <math.h>

extern I2C_HandleTypeDef hi2c1;

/* ─── 配置 ─── */
#define SPL06_ADDR          0x76
#define SPL06_ADDR_HAL      (SPL06_ADDR << 1)

#define SPL06_REG_PSR_B2    0x00
#define SPL06_REG_TMP_B2    0x03
#define SPL06_REG_PRS_CFG   0x06
#define SPL06_REG_TMP_CFG   0x07
#define SPL06_REG_MEAS_CFG  0x08
#define SPL06_REG_RESET     0x0C
#define SPL06_REG_ID        0x0D
#define SPL06_REG_COEF      0x10

/* ─── 校准系数 ─── */
typedef struct {
    int16_t c0;
    int16_t c1;
    int32_t c00;
    int16_t c10;
    int16_t c01;
    int16_t c11;
    int16_t c20;
    int16_t c21;
} SPL06_Calib_t;

static SPL06_Calib_t calib;
static SPL06_Data_t  spl06;

/* ─── 全局指针（供外部获取） ─── */
SPL06_Data_t *g_pSPL06 = &spl06;

/* ─── I2C 封装 ─── */

static uint8_t rreg(uint8_t reg)
{
    uint8_t v;
    HAL_I2C_Mem_Read(&hi2c1, SPL06_ADDR_HAL, reg, 1, &v, 1, 10);
    return v;
}

static void wreg(uint8_t reg, uint8_t v)
{
    HAL_I2C_Mem_Write(&hi2c1, SPL06_ADDR_HAL, reg, 1, &v, 1, 10);
}

static void rbuf(uint8_t reg, uint8_t *buf, uint8_t len)
{
    HAL_I2C_Mem_Read(&hi2c1, SPL06_ADDR_HAL, reg, 1, buf, len, 10);
}

/* ─── 初始化 ─── */

uint8_t SPL06_Init(void)
{
    /* 软复位 */
    wreg(SPL06_REG_RESET, 0x89);
    HAL_Delay(10);

    /* 验证 ID */
    if (rreg(SPL06_REG_ID) != 0x10) {
        printf("SPL06 ID error!\r\n");
        return 0;
    }

    /* 读取校准系数 */
    uint8_t buf[18];
    rbuf(SPL06_REG_COEF, buf, 18);
    calib.c0  = (int16_t)((buf[0]  << 8) | buf[1]);
    calib.c1  = (int16_t)((buf[2]  << 8) | buf[3]);
    calib.c00 = (int32_t)((buf[4]  << 24) | (buf[5]  << 16) |
                          (buf[6]  << 8)  |  buf[7]);
    calib.c10 = (int16_t)((buf[8]  << 8) | buf[9]);
    calib.c01 = (int16_t)((buf[10] << 8) | buf[11]);
    calib.c11 = (int16_t)((buf[12] << 8) | buf[13]);
    calib.c20 = (int16_t)((buf[14] << 8) | buf[15]);
    calib.c21 = (int16_t)((buf[16] << 8) | buf[17]);

    /* 配置：气压 8x, 温度 4x */
    wreg(SPL06_REG_PRS_CFG, 0x03);            // 8x oversampling
    wreg(SPL06_REG_TMP_CFG, (1 << 7) | 0x02); // 使能, 4x oversampling
    wreg(SPL06_REG_MEAS_CFG, 0x07);           // 持续温度+气压

    printf("SPL06 init OK\r\n");
    return 1;
}

/* ─── 数据读取 ─── */

void SPL06_Update(void)
{
    uint8_t buf[3];
    int32_t raw_t, raw_p;
    float t_scaled, p_scaled;

    /* 读温度原始值 */
    rbuf(SPL06_REG_TMP_B2, buf, 3);
    raw_t = (int32_t)((buf[0] << 16) | (buf[1] << 8) | buf[2]);

    /* 读气压原始值 */
    rbuf(SPL06_REG_PSR_B2, buf, 3);
    raw_p = (int32_t)((buf[0] << 16) | (buf[1] << 8) | buf[2]);

    /* 温度补偿 */
    t_scaled = (float)raw_t / 8.0f;
    spl06.temperature = (float)calib.c0 * 0.5f +
                        (float)calib.c1 * t_scaled;

    /* 气压补偿 */
    p_scaled = (float)raw_p / 8.0f;
    spl06.pressure = (float)calib.c00 +
                     p_scaled * ((float)calib.c10 +
                     p_scaled * ((float)calib.c20)) +
                     spl06.temperature * (float)calib.c01 +
                     spl06.temperature * p_scaled * ((float)calib.c11 +
                     p_scaled * (float)calib.c21);
}

float SPL06_GetPressure(void)    { return spl06.pressure; }
float SPL06_GetTemperature(void) { return spl06.temperature; }
```

### 7.3 FreeRTOS 任务示例

```c
void SPL06_Task(void *argument)
{
    if (!SPL06_Init()) {
        printf("SPL06 init failed!\r\n");
        vTaskDelete(NULL);
    }

    printf("SPL06 task started\r\n");

    for (;;) {
        SPL06_Update();

        printf("SPL06: P=%.1f Pa, T=%.2f C, Alt=%.2f m\r\n",
               spl06.pressure, spl06.temperature, spl06.altitude);

        osDelay(20);  // 50Hz 更新频率
    }
}
```

---

## 8. 高度估算与校准

### 8.1 气压转高度公式

标准气压高度公式（国际标准大气模型 ISA）：

```c
#define SEA_LEVEL_PRESSURE  101325.0f  // 海平面标准气压 (Pa)

float SPL06_GetAltitude(void)
{
    float pressure = spl06.pressure;

    /* 气压高度公式（ISA 模型，适用于 0-11000m） */
    float altitude = 44330.0f * (1.0f - powf(pressure / SEA_LEVEL_PRESSURE,
                                            1.0f / 5.255f));
    return altitude;
}
```

### 8.2 地面基准校准

气压随天气变化，每次飞行前需要校准地面基准：

```c
static float g_ground_pressure = 101325.0f;  // 地面气压
static float g_ground_altitude = 0.0f;       // 地面高度

void SPL06_CalibrateGround(void)
{
    /* 采集 N 次数据取平均 */
    float sum = 0;
    for (int i = 0; i < 100; i++) {
        SPL06_Update();
        sum += spl06.pressure;
        osDelay(10);
    }
    g_ground_pressure = sum / 100.0f;
    g_ground_altitude = 0.0f;

    printf("Ground pressure: %.1f Pa\r\n", g_ground_pressure);
}

float SPL06_GetRelativeAltitude(void)
{
    float pressure = spl06.pressure;

    /* 相对高度（相对于地面） */
    float altitude = 44330.0f * (1.0f - powf(pressure / g_ground_pressure,
                                            1.0f / 5.255f));
    return altitude - g_ground_altitude;
}
```

### 8.3 低通滤波

气压数据噪声较大，建议加低通滤波：

```c
static float g_altitude_filtered = 0.0f;

float SPL06_GetFilteredAltitude(void)
{
    float raw = SPL06_GetRelativeAltitude();
    float alpha = 0.1f;  // 滤波系数（越小越平滑，越大越灵敏）

    g_altitude_filtered = alpha * raw + (1.0f - alpha) * g_altitude_filtered;
    return g_altitude_filtered;
}
```

### 8.4 高度保持 PID 参考

```c
/* 配合 Control 模块使用 */
typedef struct {
    float target;      // 目标高度
    float current;     // 当前高度
    float error;       // 误差
    float throttle;    // 油门补偿
} AltitudeHold_t;

void AltitudeHold_Update(AltitudeHold_t *alt)
{
    alt->current = SPL06_GetFilteredAltitude();
    alt->error   = alt->target - alt->current;

    /* PID 计算油门补偿 */
    static float integral = 0;
    float kp = 50.0f;   // P 增益
    float ki = 5.0f;    // I 增益
    float kd = 10.0f;   // D 增益

    integral += alt->error * 0.02f;  // dt = 20ms
    integral = CLAMP(integral, -200.0f, 200.0f);

    alt->throttle = kp * alt->error + ki * integral;
    alt->throttle = CLAMP(alt->throttle, -300.0f, 300.0f);
}
```

---

## 9. 常见问题

### 9.1 读不到设备 ID

- 检查 I2C 地址是否正确（SDO 电平决定 0x76 或 0x77）
- 检查 SCL/SDA 是否有上拉电阻（10KΩ）
- 检查供电电压（3.3V）
- 用逻辑分析仪抓 I2C 波形，确认 ACK 位

### 9.2 气压值明显不对

- **最常见原因：校准系数读错了**
  - 确认 18 字节校准系数读取完整
  - 确认字节序（大端）解析正确
  - 打印每个系数值，检查是否在合理范围
- 确认补偿公式中温度单位是 °C（不是原始值）
- 确认气压过采样率设置正确

### 9.3 气压值不动或变化很慢

- 检查是否设置了持续测量模式（`MEAS_CFG = 0x07`）
- 检查过采样率是否过大（128x 需要 200ms+）
- 确认每次读取都按"先温度后气压"的顺序

### 9.4 高度漂移严重

- 气压本身受天气影响，短时间内漂移是正常的
- 增加低通滤波系数（降低 alpha）
- 配合加速度计做 **气压计 + IMU 互补滤波**，抑制漂移

### 9.5 温度值偏高

- SPL06 位于 PCB 上，会受到 MCU 和电源芯片发热影响
- 温度值偏高 5-10°C 是正常现象
- 飞控主要使用气压值，温度仅用于补偿，少量偏差影响不大

### 9.6 与 MPU9250 共享 I2C 总线的注意事项

- 两者地址不同，不会冲突
- 不要在中断中同时访问两个设备（I2C 不可重入）
- 建议在同一个 FreeRTOS 任务中顺序读取，或者用互斥锁保护

---

> **参考资料：**
> - [SPL06-001_datasheet_V2.0.pdf](SPL06-001_datasheet_V2.0.pdf) — 数据手册
> - [MPU9250编程教程.md](../MPU9250/MPU9250编程教程.md) — 同一总线上的 IMU 教程