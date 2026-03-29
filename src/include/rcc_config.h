/**
 * src/include/rcc_config.h — RCC 时钟配置头文件（存根）
 */

#ifndef RCC_CONFIG_H
#define RCC_CONFIG_H

#include <stdint.h>

/* STM32F4 RCC 结构体（简化版） */
typedef struct {
    volatile uint32_t CR;
    volatile uint32_t PLLCFGR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t AHB1RSTR;
    volatile uint32_t AHB2RSTR;
    volatile uint32_t APB1RSTR;
    volatile uint32_t APB2RSTR;
    volatile uint32_t AHB1ENR;
    volatile uint32_t AHB2ENR;
    volatile uint32_t APB1ENR;
    volatile uint32_t APB2ENR;
} RCC_TypeDef;

/* RCC 基地址 */
#define RCC ((RCC_TypeDef *)0x40023800U)

/* APB1 时钟使能位 */
#define RCC_APB1ENR_CAN1EN (1U << 25)
#define RCC_APB1ENR_CAN2EN (1U << 26)

#endif /* RCC_CONFIG_H */
#define RCC_APB2ENR_SPI1EN (1U << 12)
