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

/* 寄存器命令 */
#define SI24R1_CMD_R_REGISTER        0x00
#define SI24R1_CMD_W_REGISTER        0x20
#define SI24R1_CMD_R_RX_PAYLOAD      0x61
#define SI24R1_CMD_R_RX_PL_CNT       0x60
#define SI24R1_CMD_W_TX_PAYLOAD      0xA0
#define SI24R1_CMD_FLUSH_TX          0xE1
#define SI24R1_CMD_FLUSH_RX          0xE2
#define SI24R1_CMD_NOP               0xFF

/* 寄存器地址 */
#define SI24R1_REG_CONFIG            0x00
#define SI24R1_REG_EN_AA             0x01
#define SI24R1_REG_EN_RXADDR         0x02
#define SI24R1_REG_SETUP_AW          0x03
#define SI24R1_REG_SETUP_RETR        0x04
#define SI24R1_REG_RF_CH             0x05
#define SI24R1_REG_RF_SETUP          0x06
#define SI24R1_REG_STATUS            0x07
#define SI24R1_REG_RX_ADDR_P0        0x0A
#define SI24R1_REG_TX_ADDR           0x10
#define SI24R1_REG_RX_PW_P0          0x11
#define SI24R1_REG_FIFO_STATUS       0x17
#define SI24R1_REG_DYNPD             0x1C
#define SI24R1_REG_FEATURE           0x1D

/* 函数声明 */
uint8_t SI24R1_Init(void);
uint8_t SI24R1_SendPacket(uint8_t *data, uint8_t len);
uint8_t SI24R1_ReceivePacket(uint8_t *data, uint8_t *len);
uint8_t SI24R1_ReadReg(uint8_t reg);
void    SI24R1_WriteReg(uint8_t reg, uint8_t value);
uint8_t SI24R1_ReadStatus(void);

#endif /* __SI24R1_H__ */
