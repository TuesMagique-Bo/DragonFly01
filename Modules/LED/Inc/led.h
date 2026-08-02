#ifndef __LED_H__
#define __LED_H__

#include "main.h"
#include "cmsis_os.h"

typedef enum
{
    LED_MODE_OFF          = 0x00,
    LED_MODE_ON           = 0x01,
    LED_MODE_SLOW_BLINK   = 0x02,
    LED_MODE_FAST_BLINK   = 0x03,
    LED_MODE_VERY_FAST    = 0x04,
    LED_MODE_BREATHE      = 0x05,
} LedMode_t;

void LED_Init(void);
void LED_SetMode(LedMode_t mode);
void LED_Task(void const *argument);

#endif /* __LED_H__ */