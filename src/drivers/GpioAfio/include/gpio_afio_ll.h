/**
 * @file    gpio_afio_ll.h
 * @brief   GPIO + AFIO Layer 2 — static-inline atomic ops
 * Source:  ir/gpio_afio_ir_summary.json v3.0
 *
 * Invariant guards (R8 #11):
 *   INV_GPIO_002 — AFIOEN must be 1 before AFIO writes
 *   INV_GPIO_003 — IOPxEN must be 1 before GPIOx writes
 *   INV_GPIO_001 — locked pin field is non-writable (runtime-detected, see drv layer)
 */
#ifndef GPIO_AFIO_LL_H
#define GPIO_AFIO_LL_H

#include "gpio_afio_reg.h"
#include "gpio_afio_types.h"

/* RCC clock helpers (declared here, not in RCC driver, to keep GPIO self-contained) */
static inline void GpioAfio_LL_EnableClockGpio(GpioAfio_Port_e port)
{
    /* Guard: INV_GPIO_003 — RM0008 §7.3.7 */
    RCC_APB2ENR |= (1UL << (RCC_APB2ENR_IOPxEN_BASE + (uint32_t)port));
}

static inline void GpioAfio_LL_EnableClockAfio(void)
{
    /* Guard: INV_GPIO_002 — RM0008 §7.3.7 */
    RCC_APB2ENR |= RCC_APB2ENR_AFIOEN_Msk;
}

static inline GPIO_TypeDef *GpioAfio_LL_GetBank(GpioAfio_Port_e port)
{
    /* RM0008 §3.3 — base addresses in linear 0x400 stride from GPIOA_BASE */
    return (GPIO_TypeDef *)(GPIOA_BASE + ((uint32_t)port * 0x400UL));
}

/* Per-pin CRL/CRH 4-bit field write (no invariant guard at LL level — driver
 * layer must check LCKR.LCKK before calling, per INV_GPIO_001). */
static inline void GpioAfio_LL_SetPinMode(GpioAfio_Port_e port,
                                           uint8_t pin,
                                           GpioAfio_Mode_e mode)
{
    GPIO_TypeDef *bank = GpioAfio_LL_GetBank(port);
    uint32_t shift     = ((uint32_t)pin & 0x7U) * 4U;
    uint32_t mask      = 0xFUL << shift;
    uint32_t value     = ((uint32_t)mode & 0xFU) << shift;

    if (pin < 8U) {
        bank->CRL = (bank->CRL & ~mask) | value;
    } else {
        bank->CRH = (bank->CRH & ~mask) | value;
    }
}

/* Atomic set/reset via BSRR (R8 anti-pattern: avoid ODR RMW races). */
static inline void GpioAfio_LL_SetPin(GpioAfio_Port_e port, uint16_t pin_mask)
{
    GpioAfio_LL_GetBank(port)->BSRR = (uint32_t)pin_mask;
}

static inline void GpioAfio_LL_ResetPin(GpioAfio_Port_e port, uint16_t pin_mask)
{
    /* BSRR upper 16 bits = reset; equivalent to BRR write */
    GpioAfio_LL_GetBank(port)->BSRR = (uint32_t)pin_mask << 16U;
}

static inline uint16_t GpioAfio_LL_ReadPort(GpioAfio_Port_e port)
{
    return (uint16_t)(GpioAfio_LL_GetBank(port)->IDR & 0xFFFFU);
}

static inline bool GpioAfio_LL_IsLocked(GpioAfio_Port_e port)
{
    return ((GpioAfio_LL_GetBank(port)->LCKR & GPIO_LCKR_LCKK_Msk) != 0U);
}

/* AFIO EXTI source select (4-bit field per EXTI line, 4 lines per EXTICR reg) */
static inline void GpioAfio_LL_SetExtiSource(uint8_t exti_line, GpioAfio_Port_e port)
{
    uint32_t reg_idx = (uint32_t)exti_line >> 2U;          /* 0..3 */
    uint32_t shift   = ((uint32_t)exti_line & 0x3U) * 4U;
    uint32_t mask    = 0xFUL << shift;
    AFIO->EXTICR[reg_idx] = (AFIO->EXTICR[reg_idx] & ~mask)
                          | (((uint32_t)port & 0xFU) << shift);
}

#endif /* GPIO_AFIO_LL_H */
