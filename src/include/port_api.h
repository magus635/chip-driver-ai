#ifndef PORT_API_H
#define PORT_API_H

#include <stdint.h>

/**
 * Port API 层 — 跨模块 GPIO 引脚配置接口
 * 目标芯片: STM32F103C8T6
 * 来源: RM0008 §9.2 GPIO registers
 *
 * 注意: F103 使用 CRL/CRH (每引脚 4 位: MODE[1:0]+CNF[1:0])
 *       复用推挽输出: CNF=10, MODE=11 (50MHz)  → 4位值 = 0b1011 = 0xB
 *       浮空输入:     CNF=01, MODE=00 (输入模式) → 4位值 = 0b0100 = 0x4
 *
 * GPIOA 基址: 0x40010800 — RM0008 §3.3 Table 3
 *   CRL (pins 0-7):  偏移 0x00
 *   CRH (pins 8-15): 偏移 0x04
 *
 * CAN1 默认引脚: PA11 (CAN_RX), PA12 (CAN_TX)
 *   无需 AFIO 重映射（CAN_REMAP[1:0] = 00 为默认映射）
 */

/* 配置 CAN1 引脚: PA11=CAN_RX(浮空输入), PA12=CAN_TX(复用推挽输出) */
static inline void Port_Api_InitCanPins(void) {
    volatile uint32_t *GPIOA_CRH = (volatile uint32_t *)(0x40010800U + 0x04U);

    uint32_t crh = *GPIOA_CRH;

    /* PA11 (CAN_RX) — bits [15:12]: 浮空输入 CNF=01, MODE=00 → 0x4 */
    crh &= ~(0xFU << 12);
    crh |=  (0x4U << 12);   /* RM0008 §9.2.2 Table 20 */

    /* PA12 (CAN_TX) — bits [19:16]: 复用推挽输出 50MHz CNF=10, MODE=11 → 0xB */
    crh &= ~(0xFU << 16);
    crh |=  (0xBU << 16);   /* RM0008 §9.2.1 Table 21 */

    *GPIOA_CRH = crh;
}

/* 配置 SPI1 默认引脚: PA5=SCK, PA6=MISO, PA7=MOSI (无重映射) */
/* Source: ir/spi_ir_summary.json gpio_config[0] — RM0008 §9.3 p.175 */
static inline void Port_Api_InitSpi1Pins(void) {
    volatile uint32_t *GPIOA_CRL = (volatile uint32_t *)(0x40010800U + 0x00U);

    uint32_t crl = *GPIOA_CRL;

    /* PA5 (SCK) — bits [23:20]: AF_PP 50MHz CNF=10, MODE=11 → 0xB */
    crl &= ~(0xFU << 20);
    crl |=  (0xBU << 20);   /* RM0008 §9.2.1 Table 21 */

    /* PA6 (MISO) — bits [27:24]: Input Float CNF=01, MODE=00 → 0x4 */
    crl &= ~(0xFU << 24);
    crl |=  (0x4U << 24);   /* RM0008 §9.2.2 Table 20 */

    /* PA7 (MOSI) — bits [31:28]: AF_PP 50MHz CNF=10, MODE=11 → 0xB */
    crl &= ~(0xFU << 28);
    crl |=  (0xBU << 28);   /* RM0008 §9.2.1 Table 21 */

    *GPIOA_CRL = crl;
}

/* 配置 SPI2 引脚: PB13=SCK, PB14=MISO, PB15=MOSI (固定引脚,无重映射) */
/* Source: ir/spi_ir_summary.json gpio_config[1] — RM0008 §9.3 p.175 */
static inline void Port_Api_InitSpi2Pins(void) {
    volatile uint32_t *GPIOB_CRH = (volatile uint32_t *)(0x40010C00U + 0x04U);

    uint32_t crh = *GPIOB_CRH;

    /* PB13 (SCK) — bits [23:20]: AF_PP 50MHz → 0xB */
    crh &= ~(0xFU << 20);
    crh |=  (0xBU << 20);

    /* PB14 (MISO) — bits [27:24]: Input Float → 0x4 */
    crh &= ~(0xFU << 24);
    crh |=  (0x4U << 24);

    /* PB15 (MOSI) — bits [31:28]: AF_PP 50MHz → 0xB */
    crh &= ~(0xFU << 28);
    crh |=  (0xBU << 28);

    *GPIOB_CRH = crh;
}

#endif /* PORT_API_H */
