/**
  ******************************************************************************
  * @file           : system.h
  * @brief          : 系统管理模块
  ******************************************************************************
  */
#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "cmsis_os.h"

/* 系统状态枚举 */
typedef enum
{
    SYSTEM_STATE_INIT       = 0x00,
    SYSTEM_STATE_STANDBY    = 0x01,
    SYSTEM_STATE_ARMED      = 0x02,
    SYSTEM_STATE_FLYING     = 0x03,
    SYSTEM_STATE_ERROR      = 0xFF,
} SystemState_t;

/* 控制模式 */
typedef enum
{
    CONTROL_MODE_AUTO   = 0x00,  /* PID 自动控制 */
    CONTROL_MODE_MANUAL = 0x01,  /* 串口手动控制 */
} ControlMode_t;

/* 飞行模式 */
typedef enum
{
    FLIGHT_MODE_MANUAL      = 0x00,
    FLIGHT_MODE_ATTI        = 0x01,
    FLIGHT_MODE_ALTHOLD     = 0x02,
    FLIGHT_MODE_AUTO        = 0x03,
} FlightMode_t;

/* 系统信息结构体 */
typedef struct
{
    SystemState_t   state;
    ControlMode_t   controlMode;
    FlightMode_t    flightMode;
    uint32_t        tick;
    uint32_t        cpuLoad;
    uint8_t         batteryVoltage;
    uint8_t         errorCode;
} SystemInfo_t;

extern SystemInfo_t g_sysInfo;

void System_Init(void);
void System_Update(void);
void System_SetState(SystemState_t state);
SystemState_t System_GetState(void);
void System_SetFlightMode(FlightMode_t mode);
FlightMode_t System_GetFlightMode(void);
void System_SetControlMode(ControlMode_t mode);
ControlMode_t System_GetControlMode(void);
uint32_t System_GetTick(void);

#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_H__ */