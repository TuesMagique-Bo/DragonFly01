#ifndef __CONTROL_H__
#define __CONTROL_H__

#include "main.h"
#include "imu.h"
#include "motor.h"

typedef struct
{
    float kp, ki, kd;
    float integral, integralMax;
    float prevError;
    float outputMax, outputMin;
} PID_t;

typedef struct
{
    float rollOut, pitchOut, yawOut, throttleOut;
} ControlOutput_t;

void Control_Init(void);
void Control_SetTarget(float roll, float pitch, float yaw, float throttle);
void Control_Update(void);
void Control_ResetPID(void);
void PID_Init(PID_t *pid, float kp, float ki, float kd, float integralMax, float outputMax);
float PID_Calculate(PID_t *pid, float setpoint, float measurement, float dt);

#endif /* __CONTROL_H__ */