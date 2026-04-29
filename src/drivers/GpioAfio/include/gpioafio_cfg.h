/**
 * @file    gpioafio_cfg.h
 * @brief   GPIO + AFIO Layer 0 — compile-time configuration
 * Source:  ir/gpioafio_ir_summary.json v3.0
 */
#ifndef GPIO_AFIO_CFG_H
#define GPIO_AFIO_CFG_H

#include "gpioafio_types.h"

/* STM32F103C8T6 LQFP48 实际引出引脚掩码（IR generation_metadata） */
#define GPIO_PINS_AVAILABLE_PA  ((uint16_t)0xFFFFU)
#define GPIO_PINS_AVAILABLE_PB  ((uint16_t)0xFFFFU)
#define GPIO_PINS_AVAILABLE_PC  ((uint16_t)0xE000U)  /* PC13..15 */
#define GPIO_PINS_AVAILABLE_PD  ((uint16_t)0x0003U)  /* PD0,PD1 */
#define GPIO_PINS_AVAILABLE_PE  ((uint16_t)0x0000U)
#define GPIO_PINS_AVAILABLE_PF  ((uint16_t)0x0000U)
#define GPIO_PINS_AVAILABLE_PG  ((uint16_t)0x0000U)

/* LCKR LOCK 序列超时（轮询读回的极限循环数） */
#define GPIO_LCKR_LOCK_TIMEOUT  (1000U)

#endif /* GPIO_AFIO_CFG_H */
