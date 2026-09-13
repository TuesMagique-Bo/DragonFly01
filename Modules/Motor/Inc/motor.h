#ifndef __MOTOR_H__
#define __MOTOR_H__

#include "main.h"
#include "tim.h"

typedef enum
{
    MOTOR_1 = 0,   /* PA6 - TIM3_CH1 */
    MOTOR_2 = 1,   /* PA7 - TIM3_CH2 */
    MOTOR_3 = 2,   /* PB0 - TIM3_CH3 */
    MOTOR_4 = 3,   /* PB1 - TIM3_CH4 */
    MOTOR_MAX,
} MotorIndex_t;

#define MOTOR_PWM_PERIOD  999
#define MOTOR_PWM_IDLE    50
#define MOTOR_PWM_MAX     999

void Motor_Init(void);
void Motor_SetDuty(MotorIndex_t motor, uint16_t duty);
void Motor_SetAllDuty(uint16_t duty1, uint16_t duty2, uint16_t duty3, uint16_t duty4);
void Motor_Stop(void);
void Motor_Idle(void);
uint16_t Motor_GetDuty(MotorIndex_t motor);

#endif /* __MOTOR_H__ */