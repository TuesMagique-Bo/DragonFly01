#include "si24r1.h"
#include <stdio.h>

/* SPI 句柄（CubeMX 生成） */
extern SPI_HandleTypeDef hspi2;

/* ─── 寄存器命令 ─── */
#define CMD_R_REGISTER    0x00
#define CMD_W_REGISTER    0x20
#define CMD_R_RX_PAYLOAD  0x61
#define CMD_W_TX_PAYLOAD  0xA0
#define CMD_FLUSH_TX      0xE1
#define CMD_FLUSH_RX      0xE2
#define CMD_NOP           0xFF

/* ─── 寄存器地址 ─── */
#define REG_CONFIG        0x00
#define REG_EN_AA         0x01
#define REG_EN_RXADDR     0x02
#define REG_SETUP_AW      0x03
#define REG_SETUP_RETR    0x04
#define REG_RF_CH         0x05
#define REG_RF_SETUP      0x06
#define REG_STATUS        0x07
#define REG_RX_ADDR_P0    0x0A
#define REG_TX_ADDR       0x10
#define REG_RX_PW_P0      0x11
#define REG_DYNPD         0x1C
#define REG_FEATURE       0x1D

#define ADDR_WIDTH        5
#define RF_CHANNEL        40

/* ─── SPI 底层 ─── */

static uint8_t SPI_RW(uint8_t tx)
{
    uint8_t rx = 0xFF;
    if (HAL_SPI_TransmitReceive(&hspi2, &tx, &rx, 1, 100) != HAL_OK) {
        return 0xFF;
    }
    return rx;
}

/* ─── 寄存器操作 ─── */

uint8_t SI24R1_ReadReg(uint8_t reg)
{
    uint8_t val;
    SI24R1_CSN_LOW();
    SPI_RW(CMD_R_REGISTER | reg);
    val = SPI_RW(0xFF);
    SI24R1_CSN_HIGH();
    return val;
}

void SI24R1_WriteReg(uint8_t reg, uint8_t val)
{
    SI24R1_CSN_LOW();
    SPI_RW(CMD_W_REGISTER | reg);
    SPI_RW(val);
    SI24R1_CSN_HIGH();
}

static void ReadBuf(uint8_t reg, uint8_t *buf, uint8_t len)
{
    SI24R1_CSN_LOW();
    SPI_RW(CMD_R_REGISTER | reg);
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = SPI_RW(0xFF);
    }
    SI24R1_CSN_HIGH();
}

static void WriteBuf(uint8_t reg, uint8_t *buf, uint8_t len)
{
    SI24R1_CSN_LOW();
    SPI_RW(CMD_W_REGISTER | reg);
    for (uint8_t i = 0; i < len; i++) {
        SPI_RW(buf[i]);
    }
    SI24R1_CSN_HIGH();
}

uint8_t SI24R1_ReadStatus(void)
{
    uint8_t s;
    SI24R1_CSN_LOW();
    s = SPI_RW(CMD_NOP);
    SI24R1_CSN_HIGH();
    return s;
}

/* ─── 初始化 ─── */

uint8_t SI24R1_Init(void)
{
    uint8_t addr[ADDR_WIDTH] = {0x34, 0x43, 0x10, 0x10, 0x01};

    /* 确保 CSN 空闲为高、CE 为低后再配置 */
    SI24R1_CSN_HIGH();
    SI24R1_CE_LOW();
    HAL_Delay(5);

    /* 上电复位默认值抽查：CONFIG 应为 0x08，EN_AA 应为 0x3F */
    printf("SI24R1 power-on defaults:\r\n");
    printf("  CONFIG=0x%02X (expect 0x08)\r\n", SI24R1_ReadReg(REG_CONFIG));
    printf("  EN_AA =0x%02X (expect 0x3F)\r\n", SI24R1_ReadReg(REG_EN_AA));
    printf("  STATUS=0x%02X\r\n", SI24R1_ReadStatus());

    SI24R1_WriteReg(REG_SETUP_AW, 0x03);                      /* 5 字节地址 */
    SI24R1_WriteReg(REG_SETUP_RETR, (5 << 4) | 5);            /* 1000μs, 重传 5 次 */
    SI24R1_WriteReg(REG_RF_CH, RF_CHANNEL);                    /* 2.440GHz */
    SI24R1_WriteReg(REG_RF_SETUP, SI24R1_PWR_0dBm | (0 << 1) | 1); /* 0dBm, 2Mbps */
    WriteBuf(REG_RX_ADDR_P0, addr, ADDR_WIDTH);                /* RX 地址 */
    WriteBuf(REG_TX_ADDR, addr, ADDR_WIDTH);                   /* TX 地址 */
    SI24R1_WriteReg(REG_RX_PW_P0, SI24R1_PAYLOAD_WIDTH);      /* Payload 宽度 */
    SI24R1_WriteReg(REG_EN_AA, 0x01);                          /* 使能 Pipe 0 ACK */
    SI24R1_WriteReg(REG_EN_RXADDR, 0x01);                      /* 使能 Pipe 0 */
    SI24R1_WriteReg(REG_DYNPD, 0x01);                          /* 动态长度 */
    SI24R1_WriteReg(REG_FEATURE, 0x04);                        /* EN_DPL */
    SI24R1_WriteReg(REG_CONFIG, 0x0F);                         /* 上电, 接收模式 */

    HAL_Delay(2);  /* 等待晶振稳定 */

    /* 清空 FIFO */
    SI24R1_CSN_LOW(); SPI_RW(CMD_FLUSH_TX); SI24R1_CSN_HIGH();
    SI24R1_CSN_LOW(); SPI_RW(CMD_FLUSH_RX); SI24R1_CSN_HIGH();

    SI24R1_CE_HIGH();

    printf("SI24R1 after init:\r\n");
    printf("  CONFIG=0x%02X\r\n", SI24R1_ReadReg(REG_CONFIG));
    printf("  EN_AA =0x%02X\r\n", SI24R1_ReadReg(REG_EN_AA));
    printf("  RF_CH =0x%02X\r\n", SI24R1_ReadReg(REG_RF_CH));
    printf("  RF_SETUP=0x%02X\r\n", SI24R1_ReadReg(REG_RF_SETUP));

    if (SI24R1_ReadReg(REG_CONFIG) != 0x0F) {
        printf("SI24R1 init failed! CONFIG=0x%02X\r\n", SI24R1_ReadReg(REG_CONFIG));
        return 0;
    }

    return 1;
}

/* ─── 发送 ─── */

uint8_t SI24R1_SendPacket(uint8_t *data, uint8_t len)
{
    if (len > SI24R1_PAYLOAD_WIDTH) len = SI24R1_PAYLOAD_WIDTH;

    SI24R1_CE_LOW();

    /* 切换发送模式 */
    SI24R1_WriteReg(REG_CONFIG, SI24R1_ReadReg(REG_CONFIG) & ~0x01);

    /* 清空 TX FIFO */
    SI24R1_CSN_LOW(); SPI_RW(CMD_FLUSH_TX); SI24R1_CSN_HIGH();

    /* 写入数据 */
    SI24R1_CSN_LOW();
    SPI_RW(CMD_W_TX_PAYLOAD);
    for (uint8_t i = 0; i < len; i++) SPI_RW(data[i]);
    SI24R1_CSN_HIGH();

    /* CE 脉冲触发发送 */
    SI24R1_CE_HIGH();
    HAL_Delay(1);
    SI24R1_CE_LOW();

    /* 等待结果 */
    for (uint32_t t = 0; t < 10000; t++) {
        uint8_t s = SI24R1_ReadStatus();
        if (s & 0x20) { SI24R1_WriteReg(REG_STATUS, 0x20); return 1; }
        if (s & 0x10) {
            SI24R1_WriteReg(REG_STATUS, 0x10);
            SI24R1_CSN_LOW(); SPI_RW(CMD_FLUSH_TX); SI24R1_CSN_HIGH();
            return 0;
        }
    }
    return 0;
}

/* ─── 接收 ─── */

uint8_t SI24R1_ReceivePacket(uint8_t *data, uint8_t *len)
{
    /* 确保接收模式 */
    SI24R1_WriteReg(REG_CONFIG, SI24R1_ReadReg(REG_CONFIG) | 0x01);
    SI24R1_CE_HIGH();

    if (!(SI24R1_ReadStatus() & 0x40)) return 0;

    *len = SI24R1_PAYLOAD_WIDTH;

    SI24R1_CSN_LOW();
    SPI_RW(CMD_R_RX_PAYLOAD);
    for (uint8_t i = 0; i < *len; i++) data[i] = SPI_RW(0xFF);
    SI24R1_CSN_HIGH();

    SI24R1_WriteReg(REG_STATUS, 0x40);
    return 1;
}