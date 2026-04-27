/**
 * @file    gpio_afio_reg.h
 * @brief   GPIO + AFIO Layer 1 — register memory map
 * Source:  ir/gpio_afio_ir_summary.json v3.0
 *
 * IR-ref: peripheral.instances[GPIOA..G, AFIO]
 * IR-ref: peripheral.registers[*]
 */
#ifndef GPIO_AFIO_REG_H
#define GPIO_AFIO_REG_H

#include <stdint.h>

#define __IO  volatile

/*-------- GPIO bank: each bank is 0x400 bytes, 7 regs ---------------------*/
typedef struct {
    __IO uint32_t CRL;    /* 0x00 — RM0008 §9.2.1 */
    __IO uint32_t CRH;    /* 0x04 — RM0008 §9.2.2 */
    __IO uint32_t IDR;    /* 0x08 — RM0008 §9.2.3 (R) */
    __IO uint32_t ODR;    /* 0x0C — RM0008 §9.2.4 */
    __IO uint32_t BSRR;   /* 0x10 — RM0008 §9.2.5 (W) */
    __IO uint32_t BRR;    /* 0x14 — RM0008 §9.2.6 (W) */
    __IO uint32_t LCKR;   /* 0x18 — RM0008 §9.2.7 */
} GPIO_TypeDef;

#define GPIOA_BASE      (0x40010800UL)
#define GPIOB_BASE      (0x40010C00UL)
#define GPIOC_BASE      (0x40011000UL)
#define GPIOD_BASE      (0x40011400UL)
#define GPIOE_BASE      (0x40011800UL)
#define GPIOF_BASE      (0x40011C00UL)
#define GPIOG_BASE      (0x40012000UL)

#define GPIOA           ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB           ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC           ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD           ((GPIO_TypeDef *)GPIOD_BASE)
#define GPIOE           ((GPIO_TypeDef *)GPIOE_BASE)
#define GPIOF           ((GPIO_TypeDef *)GPIOF_BASE)
#define GPIOG           ((GPIO_TypeDef *)GPIOG_BASE)

/* LCKR */
#define GPIO_LCKR_LCK_Pos    (0U)
#define GPIO_LCKR_LCK_Msk    (0xFFFFUL << GPIO_LCKR_LCK_Pos)
#define GPIO_LCKR_LCKK_Pos   (16U)
#define GPIO_LCKR_LCKK_Msk   (0x1UL << GPIO_LCKR_LCKK_Pos)

/*-------- AFIO ------------------------------------------------------------*/
typedef struct {
    __IO uint32_t EVCR;     /* 0x00 — RM0008 §9.4.1 */
    __IO uint32_t MAPR;     /* 0x04 — RM0008 §9.4.2 */
    __IO uint32_t EXTICR[4];/* 0x08-0x14 — RM0008 §9.4.3-§9.4.6 */
    __IO uint32_t _RESERVED;/* 0x18 */
    __IO uint32_t MAPR2;    /* 0x1C — RM0008 §9.4.7 */
} AFIO_TypeDef;

#define AFIO_BASE       (0x40010000UL)
#define AFIO            ((AFIO_TypeDef *)AFIO_BASE)

/* EVCR */
#define AFIO_EVCR_PIN_Pos    (0U)
#define AFIO_EVCR_PIN_Msk    (0xFUL << AFIO_EVCR_PIN_Pos)
#define AFIO_EVCR_PORT_Pos   (4U)
#define AFIO_EVCR_PORT_Msk   (0x7UL << AFIO_EVCR_PORT_Pos)
#define AFIO_EVCR_EVOE_Pos   (7U)
#define AFIO_EVCR_EVOE_Msk   (0x1UL << AFIO_EVCR_EVOE_Pos)

/*-------- RCC (only what we need to enable GPIO/AFIO clocks) --------------*/
#define RCC_APB2ENR_ADDR  (0x40021018UL)
#define RCC_APB2ENR       (*(__IO uint32_t *)RCC_APB2ENR_ADDR)

#define RCC_APB2ENR_AFIOEN_Msk  (0x1UL << 0U)
#define RCC_APB2ENR_IOPxEN_BASE (2U)  /* IOPAEN at bit 2; IOPxEN at bit (2+x) */

#endif /* GPIO_AFIO_REG_H */
