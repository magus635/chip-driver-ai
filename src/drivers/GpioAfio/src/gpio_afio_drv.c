/**
 * @file    gpio_afio_drv.c
 * @brief   GPIO + AFIO Layer 3 — driver state-machine & sequence logic
 * Source:  ir/gpio_afio_ir_summary.json v3.0
 *
 * ISR exemption: GPIO 自身无 IRQ 入口（EXTI 中断属 EXTI 模块），按 codegen-agent
 * §2.7 豁免 _isr.c。AFIO 仅参与 EXTI 源选择，亦无独立 IRQ。
 */
#include "gpio_afio_api.h"
#include "gpio_afio_ll.h"

/* SEQ_LCKR_LOCK — RM0008 §9.2.7 */
static GpioAfio_ReturnType run_lock_sequence(GpioAfio_Port_e port, uint16_t pin_mask)
{
    GPIO_TypeDef *bank = GpioAfio_LL_GetBank(port);
    uint32_t lck       = (uint32_t)pin_mask & GPIO_LCKR_LCK_Msk;

    /* Step 1: write LCKK=1 + mask */
    bank->LCKR = lck | GPIO_LCKR_LCKK_Msk;
    /* Step 2: write LCKK=0 + same mask */
    bank->LCKR = lck;
    /* Step 3: write LCKK=1 + same mask */
    bank->LCKR = lck | GPIO_LCKR_LCKK_Msk;

    /* Step 4: read; expect LCKK==0 (read-back behavior per RM0008) */
    (void)bank->LCKR;
    /* Step 5: read; expect LCKK==1 confirming lock */
    if ((bank->LCKR & GPIO_LCKR_LCKK_Msk) == 0U) {
        return GPIO_LOCK_ABORT;
    }
    return GPIO_OK;
}

GpioAfio_ReturnType GpioAfio_Init(const GpioAfio_PinConfigType *pConfig)
{
    if (pConfig == NULL || pConfig->port >= GPIO_PORT_COUNT) {
        return GPIO_PARAM;
    }
    if (GpioAfio_LL_IsLocked(pConfig->port)) {
        /* Some pins on this bank may still be writable, but be conservative. */
        return GPIO_LOCKED;
    }

    /* Step 1: enable peripheral clock (INV_GPIO_002/003) */
    GpioAfio_LL_EnableClockGpio(pConfig->port);

    /* Step 2: set MODE+CNF for each pin in mask */
    for (uint8_t pin = 0U; pin < 16U; pin++) {
        if ((pConfig->pin_mask & (uint16_t)(1U << pin)) != 0U) {
            GpioAfio_LL_SetPinMode(pConfig->port, pin, pConfig->mode);
        }
    }

    /* Step 3: pull-up/down via ODR for INPUT_PUPD */
    if (pConfig->mode == GPIO_MODE_INPUT_PUPD) {
        if (pConfig->pull == GPIO_PULL_UP) {
            GpioAfio_LL_SetPin(pConfig->port, pConfig->pin_mask);
        } else if (pConfig->pull == GPIO_PULL_DOWN) {
            GpioAfio_LL_ResetPin(pConfig->port, pConfig->pin_mask);
        } else {
            /* GPIO_PULL_NONE on INPUT_PUPD is a config error — silently floats */
        }
    }

    return GPIO_OK;
}

GpioAfio_ReturnType GpioAfio_DeInit(GpioAfio_Port_e port)
{
    if (port >= GPIO_PORT_COUNT) {
        return GPIO_PARAM;
    }
    if (GpioAfio_LL_IsLocked(port)) {
        return GPIO_LOCKED;
    }
    GPIO_TypeDef *bank = GpioAfio_LL_GetBank(port);
    /* RM0008 reset values */
    bank->CRL = 0x44444444UL;
    bank->CRH = 0x44444444UL;
    bank->ODR = 0U;
    return GPIO_OK;
}

GpioAfio_ReturnType GpioAfio_WritePin(GpioAfio_Port_e port, uint16_t pin_mask, bool level)
{
    if (port >= GPIO_PORT_COUNT) { return GPIO_PARAM; }
    if (level) {
        GpioAfio_LL_SetPin(port, pin_mask);
    } else {
        GpioAfio_LL_ResetPin(port, pin_mask);
    }
    return GPIO_OK;
}

GpioAfio_ReturnType GpioAfio_TogglePin(GpioAfio_Port_e port, uint16_t pin_mask)
{
    if (port >= GPIO_PORT_COUNT) { return GPIO_PARAM; }
    GPIO_TypeDef *bank = GpioAfio_LL_GetBank(port);
    uint16_t cur       = (uint16_t)(bank->ODR & 0xFFFFU);
    uint16_t set_bits  = (uint16_t)(pin_mask & ~cur);
    uint16_t clr_bits  = (uint16_t)(pin_mask & cur);
    /* Single BSRR write performs both at once (BSx wins same-bit collision per
     * RM0008 §9.2.5; not relevant here since set/clr disjoint) */
    bank->BSRR = (uint32_t)set_bits | ((uint32_t)clr_bits << 16U);
    return GPIO_OK;
}

bool GpioAfio_ReadPin(GpioAfio_Port_e port, uint8_t pin)
{
    if (port >= GPIO_PORT_COUNT || pin >= 16U) { return false; }
    return ((GpioAfio_LL_ReadPort(port) >> pin) & 0x1U) != 0U;
}

GpioAfio_ReturnType GpioAfio_LockPins(GpioAfio_Port_e port, uint16_t pin_mask)
{
    if (port >= GPIO_PORT_COUNT) { return GPIO_PARAM; }
    return run_lock_sequence(port, pin_mask);
}

GpioAfio_ReturnType GpioAfio_SetExtiSource(uint8_t exti_line, GpioAfio_Port_e port)
{
    if (exti_line >= 16U || port >= GPIO_PORT_COUNT) { return GPIO_PARAM; }
    /* INV_GPIO_002 — must enable AFIO clock first */
    GpioAfio_LL_EnableClockAfio();
    GpioAfio_LL_SetExtiSource(exti_line, port);
    return GPIO_OK;
}
