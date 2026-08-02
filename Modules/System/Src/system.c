#include "system.h"

SystemInfo_t g_sysInfo;
static volatile uint32_t sysTick = 0;

void System_Init(void)
{
    g_sysInfo.state          = SYSTEM_STATE_INIT;
    g_sysInfo.controlMode    = CONTROL_MODE_AUTO;
    g_sysInfo.flightMode     = FLIGHT_MODE_MANUAL;
    g_sysInfo.tick           = 0;
    g_sysInfo.cpuLoad        = 0;
    g_sysInfo.batteryVoltage = 0;
    g_sysInfo.errorCode      = 0;
    sysTick = 0;
}

void System_Update(void)
{
    sysTick++;
    g_sysInfo.tick = sysTick;
}

void System_SetState(SystemState_t state)
{
    g_sysInfo.state = state;
}

SystemState_t System_GetState(void)
{
    return g_sysInfo.state;
}

void System_SetFlightMode(FlightMode_t mode)
{
    g_sysInfo.flightMode = mode;
}

FlightMode_t System_GetFlightMode(void)
{
    return g_sysInfo.flightMode;
}

void System_SetControlMode(ControlMode_t mode)
{
    g_sysInfo.controlMode = mode;
}

ControlMode_t System_GetControlMode(void)
{
    return g_sysInfo.controlMode;
}

uint32_t System_GetTick(void)
{
    return sysTick;
}