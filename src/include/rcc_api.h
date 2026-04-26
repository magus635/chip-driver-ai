#ifndef RCC_API_H
#define RCC_API_H

#include <stdint.h>

/**
 * RCC API 层 — 跨模块时钟使能接口
 * 目标芯片: STM32F103C8T6
 * 来源: RM0008 §7.3
 *
 * 注意: F103 的 RCC 基址为 0x40021000
 *       GPIO 时钟在 APB2ENR (偏移 0x18)，不在 AHB1ENR
 *       CAN1 时钟在 APB1ENR (偏移 0x1C) bit 25
 */

/* 使能 CAN1 时钟（含 GPIOA + AFIO 时钟） */
static inline void Rcc_Api_EnableCan1(void) {
    volatile uint32_t *RCC_APB2ENR = (volatile uint32_t *)0x40021018U; /* RM0008 §7.3.7 */
    volatile uint32_t *RCC_APB1ENR = (volatile uint32_t *)0x4002101CU; /* RM0008 §7.3.8 */

    *RCC_APB2ENR |= (1U << 0);   /* AFIOEN — AFIO 时钟使能 (CAN 引脚重映射需要) */
    *RCC_APB2ENR |= (1U << 2);   /* IOPAEN — GPIOA 时钟使能 (PA11/PA12 = CAN1_RX/TX) */
    *RCC_APB1ENR |= (1U << 25);  /* CAN1EN — CAN1 时钟使能 */
}

/* 使能 SPI1 时钟（含 GPIOA 时钟） */
static inline void Rcc_Api_EnableSpi1(void) {
    volatile uint32_t *RCC_APB2ENR = (volatile uint32_t *)0x40021018U; /* RM0008 §7.3.7 */

    *RCC_APB2ENR |= (1U << 2);   /* IOPAEN — GPIOA 时钟使能 */
    *RCC_APB2ENR |= (1U << 12);  /* SPI1EN — SPI1 时钟使能 */
}

/* 使能 SPI2 时钟（含 GPIOB 时钟） */
/* Source: ir/spi_ir_summary.json clock[1] — RM0008 §7.3.8 p.114 */
static inline void Rcc_Api_EnableSpi2(void) {
    volatile uint32_t *RCC_APB2ENR = (volatile uint32_t *)0x40021018U; /* RM0008 §7.3.7 */
    volatile uint32_t *RCC_APB1ENR = (volatile uint32_t *)0x4002101CU; /* RM0008 §7.3.8 */

    *RCC_APB2ENR |= (1U << 3);   /* IOPBEN — GPIOB 时钟使能 */
    *RCC_APB1ENR |= (1U << 14);  /* SPI2EN — SPI2 时钟使能 */
}

#endif /* RCC_API_H */
