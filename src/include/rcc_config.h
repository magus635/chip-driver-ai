/**
 * src/include/rcc_config.h — RCC 时钟配置头文件
 * 目标芯片: STM32F103C8T6 (Cortex-M3)
 * 来源: RM0008 §7.3 RCC registers (pp.99-120)
 *
 * 注意: F103 的 RCC 基地址为 0x40021000，与 F4 系列 (0x40023800) 不同
 *       F103 的 RCC 寄存器布局也不同于 F4（没有 PLLCFGR，用 CFGR 配置 PLL）
 */

#ifndef RCC_CONFIG_H
#define RCC_CONFIG_H

#include <stdint.h>

/* STM32F103 RCC 寄存器结构体 — RM0008 §7.3 */
typedef struct {
    volatile uint32_t CR;         /* 0x00 — 时钟控制寄存器 */
    volatile uint32_t CFGR;       /* 0x04 — 时钟配置寄存器 */
    volatile uint32_t CIR;        /* 0x08 — 时钟中断寄存器 */
    volatile uint32_t APB2RSTR;   /* 0x0C — APB2 外设复位寄存器 */
    volatile uint32_t APB1RSTR;   /* 0x10 — APB1 外设复位寄存器 */
    volatile uint32_t AHBENR;     /* 0x14 — AHB 外设时钟使能寄存器 */
    volatile uint32_t APB2ENR;    /* 0x18 — APB2 外设时钟使能寄存器 */
    volatile uint32_t APB1ENR;    /* 0x1C — APB1 外设时钟使能寄存器 */
    volatile uint32_t BDCR;       /* 0x20 — 备份域控制寄存器 */
    volatile uint32_t CSR;        /* 0x24 — 控制/状态寄存器 */
} RCC_TypeDef;

/* RCC 基地址 — RM0008 §3.3 Memory map, Table 3 p.50 */
#define RCC ((RCC_TypeDef *)0x40021000U)

/* APB1 时钟使能位 — RM0008 §7.3.8 p.114 */
#define RCC_APB1ENR_CAN1EN   (1U << 25)   /* CAN 时钟使能 — 注意: F103 只有 CAN1 */
#define RCC_APB1ENR_I2C1EN   (1U << 21)   /* I2C1 时钟使能 */
#define RCC_APB1ENR_I2C2EN   (1U << 22)   /* I2C2 时钟使能 */
#define RCC_APB1ENR_SPI2EN   (1U << 14)   /* SPI2 时钟使能 */
#define RCC_APB1ENR_SPI3EN   (1U << 15)   /* SPI3 时钟使能 */
#define RCC_APB1ENR_USART2EN (1U << 17)   /* USART2 时钟使能 */
#define RCC_APB1ENR_USART3EN (1U << 18)   /* USART3 时钟使能 */

/* APB2 时钟使能位 — RM0008 §7.3.7 p.112 */
#define RCC_APB2ENR_AFIOEN   (1U << 0)    /* AFIO 时钟使能 */
#define RCC_APB2ENR_IOPAEN   (1U << 2)    /* GPIOA 时钟使能 */
#define RCC_APB2ENR_IOPBEN   (1U << 3)    /* GPIOB 时钟使能 */
#define RCC_APB2ENR_IOPCEN   (1U << 4)    /* GPIOC 时钟使能 */
#define RCC_APB2ENR_IOPDEN   (1U << 5)    /* GPIOD 时钟使能 */
#define RCC_APB2ENR_SPI1EN   (1U << 12)   /* SPI1 时钟使能 */
#define RCC_APB2ENR_USART1EN (1U << 14)   /* USART1 时钟使能 */
#define RCC_APB2ENR_ADC1EN   (1U << 9)    /* ADC1 时钟使能 */
#define RCC_APB2ENR_ADC2EN   (1U << 10)   /* ADC2 时钟使能 */
#define RCC_APB2ENR_TIM1EN   (1U << 11)   /* TIM1 时钟使能 */

/* AHB 时钟使能位 — RM0008 §7.3.6 p.111 */
#define RCC_AHBENR_DMA1EN    (1U << 0)    /* DMA1 时钟使能 */
#define RCC_AHBENR_DMA2EN    (1U << 1)    /* DMA2 时钟使能 */

#endif /* RCC_CONFIG_H */
