#include "comm.h"
#include "led.h"
#include "motor.h"
#include "system.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

RC_Data_t g_rcData;

/* 环形缓冲区 */
static uint8_t  rxBuf[COMM_RX_BUF_SIZE];
static volatile uint16_t rxHead = 0;
static volatile uint16_t rxTail = 0;

/* 命令解析缓冲区 */
static char  cmdBuf[COMM_RX_BUF_SIZE];
static uint8_t cmdLen = 0;

/* 中断接收字节 */
static uint8_t rxByte;

void Comm_Init(void)
{
    memset(&g_rcData, 0, sizeof(RC_Data_t));
    rxHead = 0;
    rxTail = 0;
    cmdLen = 0;
    HAL_UART_Receive_IT(&huart1, &rxByte, 1);
}

/* 环形缓冲区压入（ISR 安全） */
void Comm_RxPush(uint8_t data)
{
    uint16_t next = (rxHead + 1) % COMM_RX_BUF_SIZE;
    if (next != rxTail)  /* 未满 */
    {
        rxBuf[rxHead] = data;
        rxHead = next;
    }
}

/* 环形缓冲区弹出（返回 1 成功，0 空） */
static uint8_t Comm_RxPop(uint8_t *data)
{
    if (rxHead == rxTail) return 0;
    *data = rxBuf[rxTail];
    rxTail = (rxTail + 1) % COMM_RX_BUF_SIZE;
    return 1;
}

/* 字符串转小写 */
static void StrToLower(char *str)
{
    while (*str) { *str = tolower((unsigned char)*str); str++; }
}

/* 命令解析 */
static void Comm_ParseCommand(const char *cmd)
{
    char buf[COMM_RX_BUF_SIZE];
    char *token;
    int mode, motor, duty, d1, d2, d3, d4;

    strncpy(buf, cmd, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    StrToLower(buf);

    token = strtok(buf, " \t\r\n");

    /* --- INFO --- */
    if (token && strcmp(token, "info") == 0)
    {
        printf("=== DragonFly Info ===\r\n");
        printf("Tick: %lu\r\n", System_GetTick());
        printf("State: %d\r\n", (int)System_GetState());
        printf("Control: %s\r\n",
               System_GetControlMode() == CONTROL_MODE_MANUAL ? "MANUAL" : "AUTO");
        printf("Motor: %d %d %d %d\r\n",
               (int)__HAL_TIM_GET_COMPARE(&htim3, TIM_CHANNEL_1),
               (int)__HAL_TIM_GET_COMPARE(&htim3, TIM_CHANNEL_2),
               (int)__HAL_TIM_GET_COMPARE(&htim3, TIM_CHANNEL_3),
               (int)__HAL_TIM_GET_COMPARE(&htim3, TIM_CHANNEL_4));
        printf("=====================\r\n");
        return;
    }

    /* --- MODE --- */
    if (token && strcmp(token, "mode") == 0)
    {
        token = strtok(NULL, " \t\r\n");
        if (!token)
        {
            printf("Usage: MODE <AUTO|MANUAL>\r\n");
            return;
        }
        if (strcmp(token, "manual") == 0)
        {
            System_SetControlMode(CONTROL_MODE_MANUAL);
            printf("Control mode: MANUAL (motor values preserved)\r\n");
            return;
        }
        if (strcmp(token, "auto") == 0)
        {
            System_SetControlMode(CONTROL_MODE_AUTO);
            printf("Control mode: AUTO (PID control)\r\n");
            return;
        }
        printf("MODE: invalid '%s', use AUTO or MANUAL\r\n", token);
        return;
    }

    /* --- LED --- */
    if (token && strcmp(token, "led") == 0)
    {
        token = strtok(NULL, " \t\r\n");
        if (!token)
        {
            printf("Usage: LED <0-5|?>\r\n");
            return;
        }
        if (strcmp(token, "?") == 0)
        {
            printf("LED mode query not available (use known value)\r\n");
            return;
        }
        mode = atoi(token);
        if (mode < 0 || mode > 5)
        {
            printf("LED: invalid mode %d (0-5)\r\n", mode);
            return;
        }
        LED_SetMode((LedMode_t)mode);
        printf("LED mode set to %d\r\n", mode);
        return;
    }

    /* --- MOTOR --- */
    if (token && strcmp(token, "motor") == 0)
    {
        token = strtok(NULL, " \t\r\n");
        if (!token)
        {
            printf("Usage: MOTOR <1-4|ALL|STOP|IDLE> [duty]\r\n");
            return;
        }

        if (strcmp(token, "stop") == 0)
        {
            Motor_Stop();
            printf("MOTOR: all stopped\r\n");
            return;
        }

        if (strcmp(token, "idle") == 0)
        {
            Motor_Idle();
            printf("MOTOR: idle\r\n");
            return;
        }

        if (strcmp(token, "all") == 0)
        {
            char *a = strtok(NULL, " \t\r\n");
            char *b = strtok(NULL, " \t\r\n");
            char *c = strtok(NULL, " \t\r\n");
            char *d = strtok(NULL, " \t\r\n");
            if (!a || !b || !c || !d)
            {
                printf("Usage: MOTOR ALL <d1> <d2> <d3> <d4>\r\n");
                return;
            }
            d1 = atoi(a); d2 = atoi(b); d3 = atoi(c); d4 = atoi(d);
            if (d1 < 0 || d1 > 999 || d2 < 0 || d2 > 999 ||
                d3 < 0 || d3 > 999 || d4 < 0 || d4 > 999)
            {
                printf("MOTOR: duty must be 0-999\r\n");
                return;
            }
            Motor_SetAllDuty((uint16_t)d1, (uint16_t)d2, (uint16_t)d3, (uint16_t)d4);
            printf("MOTOR: M1=%d M2=%d M3=%d M4=%d\r\n", d1, d2, d3, d4);
            return;
        }

        motor = atoi(token);
        if (motor < 1 || motor > 4)
        {
            printf("MOTOR: invalid motor %d (1-4)\r\n", motor);
            return;
        }

        token = strtok(NULL, " \t\r\n");
        if (!token)
        {
            printf("Usage: MOTOR <1-4> <duty>\r\n");
            return;
        }
        duty = atoi(token);
        if (duty < 0 || duty > 999)
        {
            printf("MOTOR: duty must be 0-999\r\n", duty);
            return;
        }
        Motor_SetDuty((MotorIndex_t)(motor - 1), (uint16_t)duty);
        printf("MOTOR %d set to %d\r\n", motor, duty);
        return;
    }

    /* --- 未知命令 --- */
    if (token && strlen(token) > 0)
    {
        printf("Unknown: '%s'. Try: MODE, LED, MOTOR, INFO\r\n", token);
    }
}

/* 命令处理（Comm Task 中循环调用） */
void Comm_Process(void)
{
    uint8_t ch;

    while (Comm_RxPop(&ch))
    {
        if (ch == '\n')
        {
            /* 一行命令结束 */
            if (cmdLen > 0 && cmdBuf[cmdLen - 1] == '\r')
            {
                cmdBuf[cmdLen - 1] = '\0';
            }
            else
            {
                cmdBuf[cmdLen] = '\0';
            }

            if (cmdLen > 0)
            {
                Comm_ParseCommand(cmdBuf);
            }
            cmdLen = 0;
        }
        else if (ch == '\r')
        {
            /* 存入但不立即处理，等 \n */
            if (cmdLen < COMM_RX_BUF_SIZE - 1)
            {
                cmdBuf[cmdLen++] = ch;
            }
        }
        else
        {
            /* 普通字符 */
            if (cmdLen < COMM_RX_BUF_SIZE - 1)
            {
                cmdBuf[cmdLen++] = ch;
            }
        }
    }
}

void Comm_Task(void const *argument)
{
    for (;;)
    {
        Comm_Process();
        osDelay(1);
    }
}

/* USART 接收中断回调 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        Comm_RxPush(rxByte);
        HAL_UART_Receive_IT(&huart1, &rxByte, 1);
    }
}