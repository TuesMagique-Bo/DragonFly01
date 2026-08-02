#include "control.h"
#include "system.h"

static PID_t pidRoll, pidPitch, pidYaw;
static float  rollTarget, pitchTarget, yawTarget, throttleTarget;

void PID_Init(PID_t *pid, float kp, float ki, float kd, float integralMax, float outputMax)
{
    pid->kp = kp; pid->ki = ki; pid->kd = kd;
    pid->integral = 0.0f; pid->integralMax = integralMax;
    pid->prevError = 0.0f;
    pid->outputMax = outputMax; pid->outputMin = -outputMax;
}

float PID_Calculate(PID_t *pid, float setpoint, float measurement, float dt)
{
    float error = setpoint - measurement;
    float output = pid->kp * error;

    pid->integral += error * dt;
    if (pid->integral > pid->integralMax) pid->integral = pid->integralMax;
    else if (pid->integral < -pid->integralMax) pid->integral = -pid->integralMax;
    output += pid->ki * pid->integral;

    if (dt > 0.0001f)
        output += pid->kd * (error - pid->prevError) / dt;
    pid->prevError = error;

    if (output > pid->outputMax) output = pid->outputMax;
    if (output < pid->outputMin) output = pid->outputMin;
    return output;
}

void Control_Init(void)
{
    PID_Init(&pidRoll,  1.5f, 0.02f, 10.0f, 100.0f, 400.0f);
    PID_Init(&pidPitch, 1.5f, 0.02f, 10.0f, 100.0f, 400.0f);
    PID_Init(&pidYaw,   3.0f, 0.01f,  5.0f, 100.0f, 400.0f);
    rollTarget = pitchTarget = yawTarget = throttleTarget = 0.0f;
}

void Control_SetTarget(float roll, float pitch, float yaw, float throttle)
{
    rollTarget = roll; pitchTarget = pitch; yawTarget = yaw; throttleTarget = throttle;
}

void Control_Update(void)
{
    Attitude_t att;
    float dt = 0.001f;

    /* 手动模式下不干预电机，由串口命令控制 */
    if (System_GetControlMode() == CONTROL_MODE_MANUAL)
        return;

    IMU_GetAttitude(&att);

    if (System_GetState() == SYSTEM_STATE_ARMED ||
        System_GetState() == SYSTEM_STATE_FLYING)
    {
        float r = PID_Calculate(&pidRoll,  rollTarget,  att.roll,  dt);
        float p = PID_Calculate(&pidPitch, pitchTarget, att.pitch, dt);
        float y = PID_Calculate(&pidYaw,   yawTarget,   att.yaw,   dt);
        float t = throttleTarget;

        /* X 型四轴混合 */
        float m1 = t + r - p - y;
        float m2 = t - r + p - y;
        float m3 = t + r + p + y;
        float m4 = t - r - p + y;

        if (m1 < 0) m1 = 0; if (m1 > MOTOR_PWM_MAX) m1 = MOTOR_PWM_MAX;
        if (m2 < 0) m2 = 0; if (m2 > MOTOR_PWM_MAX) m2 = MOTOR_PWM_MAX;
        if (m3 < 0) m3 = 0; if (m3 > MOTOR_PWM_MAX) m3 = MOTOR_PWM_MAX;
        if (m4 < 0) m4 = 0; if (m4 > MOTOR_PWM_MAX) m4 = MOTOR_PWM_MAX;

        Motor_SetAllDuty((uint16_t)m1, (uint16_t)m2, (uint16_t)m3, (uint16_t)m4);
    }
    else
    {
        Motor_Stop();
    }
}

void Control_ResetPID(void)
{
    pidRoll.integral = pidRoll.prevError = 0.0f;
    pidPitch.integral = pidPitch.prevError = 0.0f;
    pidYaw.integral = pidYaw.prevError = 0.0f;
}