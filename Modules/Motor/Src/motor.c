#include "motor.h"

static const uint32_t motorChannels[MOTOR_MAX] =
{
    TIM_CHANNEL_1, TIM_CHANNEL_2, TIM_CHANNEL_3, TIM_CHANNEL_4,
};

void Motor_Init(void)
{
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
    Motor_Stop();
}

void Motor_SetDuty(MotorIndex_t motor, uint16_t duty)
{
    if (motor >= MOTOR_MAX) return;
    if (duty > MOTOR_PWM_MAX) duty = MOTOR_PWM_MAX;
    __HAL_TIM_SET_COMPARE(&htim3, motorChannels[motor], duty);
}

void Motor_SetAllDuty(uint16_t duty1, uint16_t duty2, uint16_t duty3, uint16_t duty4)
{
    Motor_SetDuty(MOTOR_1, duty1);
    Motor_SetDuty(MOTOR_2, duty2);
    Motor_SetDuty(MOTOR_3, duty3);
    Motor_SetDuty(MOTOR_4, duty4);
}

void Motor_Stop(void)
{
    Motor_SetAllDuty(0, 0, 0, 0);
}

void Motor_Idle(void)
{
    Motor_SetAllDuty(MOTOR_PWM_IDLE, MOTOR_PWM_IDLE, MOTOR_PWM_IDLE, MOTOR_PWM_IDLE);
}

uint16_t Motor_GetDuty(MotorIndex_t motor)
{
    if (motor >= MOTOR_MAX) return 0;
    return (uint16_t)__HAL_TIM_GET_COMPARE(&htim3, motorChannels[motor]);
}