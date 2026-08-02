#ifndef __COMM_H__
#define __COMM_H__

#include "main.h"
#include "usart.h"
#include "cmsis_os.h"

#define COMM_RX_BUF_SIZE  128
#define COMM_TX_BUF_SIZE  256

typedef struct
{
    int16_t throttle;
    int16_t roll;
    int16_t pitch;
    int16_t yaw;
    int16_t aux1;
    int16_t aux2;
    int16_t aux3;
    int16_t aux4;
} RC_Data_t;

extern RC_Data_t g_rcData;

void Comm_Init(void);
void Comm_SendByte(uint8_t data);
void Comm_SendBytes(uint8_t *data, uint16_t len);
void Comm_SendString(const char *str);
void Comm_RxPush(uint8_t data);
void Comm_Process(void);
void Comm_Task(void const *argument);

#endif /* __COMM_H__ */