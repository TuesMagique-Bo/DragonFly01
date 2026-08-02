#include "led.h"
#include "gpio.h"

#define LED_PORT  GPIOB
#define LED_PIN   GPIO_PIN_8

static LedMode_t ledMode = LED_MODE_OFF;

void LED_Init(void)
{
    ledMode = LED_MODE_SLOW_BLINK;
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
}

void LED_SetMode(LedMode_t mode)
{
    ledMode = mode;
}

void LED_Task(void const *argument)
{
    uint32_t period = 500;
    uint8_t  ledState = 0;

    for (;;)
    {
        switch (ledMode)
        {
            case LED_MODE_OFF:          period = 500; HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET); break;
            case LED_MODE_ON:           period = 500; HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);   break;
            case LED_MODE_SLOW_BLINK:   period = 500; break;
            case LED_MODE_FAST_BLINK:   period = 200; break;
            case LED_MODE_VERY_FAST:    period = 100; break;
            case LED_MODE_BREATHE:      period = 20;  break;
            default:                    period = 500; break;
        }

        if (ledMode == LED_MODE_SLOW_BLINK ||
            ledMode == LED_MODE_FAST_BLINK ||
            ledMode == LED_MODE_VERY_FAST)
        {
            ledState = !ledState;
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, ledState ? GPIO_PIN_SET : GPIO_PIN_RESET);
        }

        osDelay(period);
    }
}