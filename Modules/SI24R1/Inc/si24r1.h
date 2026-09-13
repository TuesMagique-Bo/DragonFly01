#ifndef __SI24R1_H__
#define __SI24R1_H__

#include "main.h"

/* ──────────── 引脚定义（与 CubeMX / main.h 一致） ──────────── */
#define SI24R1_CSN_PORT     SI24R1_CSN_GPIO_Port
#define SI24R1_CSN_PIN      SI24R1_CSN_Pin
#define SI24R1_CE_PORT      SI24R1_CE_GPIO_Port
#define SI24R1_CE_PIN       SI24R1_CE_Pin
#define SI24R1_IRQ_PORT     SI24R1_IRQ_GPIO_Port
#define SI24R1_IRQ_PIN      SI24R1_IRQ_Pin

/* 引脚控制宏 */
#define SI24R1_CSN_LOW()    HAL_GPIO_WritePin(SI24R1_CSN_PORT, SI24R1_CSN_PIN, GPIO_PIN_RESET)
#define SI24R1_CSN_HIGH()   HAL_GPIO_WritePin(SI24R1_CSN_PORT, SI24R1_CSN_PIN, GPIO_PIN_SET)
#define SI24R1_CE_LOW()     HAL_GPIO_WritePin(SI24R1_CE_PORT, SI24R1_CE_PIN, GPIO_PIN_RESET)
#define SI24R1_CE_HIGH()    HAL_GPIO_WritePin(SI24R1_CE_PORT, SI24R1_CE_PIN, GPIO_PIN_SET)
#define SI24R1_IRQ_READ()   HAL_GPIO_ReadPin(SI24R1_IRQ_PORT, SI24R1_IRQ_PIN)

/* SI24R1 RF_SETUP 功率选项 */
#define SI24R1_PWR_N12dBm  0x00
#define SI24R1_PWR_N6dBm   0x04
#define SI24R1_PWR_N4dBm   0x08
#define SI24R1_PWR_0dBm    0x0C
#define SI24R1_PWR_P1dBm   0x10
#define SI24R1_PWR_P3dBm   0x14
#define SI24R1_PWR_P4dBm   0x18
#define SI24R1_PWR_P7dBm   0x1C

/* 载荷宽度 */
#define SI24R1_PAYLOAD_WIDTH  32

/* 函数声明 */
uint8_t SI24R1_Init(void);
uint8_t SI24R1_SendPacket(uint8_t *data, uint8_t len);
uint8_t SI24R1_ReceivePacket(uint8_t *data, uint8_t *len);
uint8_t SI24R1_ReadReg(uint8_t reg);
void    SI24R1_WriteReg(uint8_t reg, uint8_t value);
uint8_t SI24R1_ReadStatus(void);

#endif /* __SI24R1_H__ */
