/**
 * @file    gpioafio_drv.c
 * @brief   GPIO + AFIO Layer 3 — driver state-machine & sequence logic
 * Source:  ir/gpio_afio_ir_summary.json v3.0
 *
 * ISR exemption: GPIO 自身无 IRQ（EXTI 中断属 EXTI 模块），按 codegen-agent
 * §2.7 豁免；占位 gpioafio_isr.c 已生成。
 */
#include "gpioafio_api.h"
#include "gpioafio_ll.h"

/* SEQ_LCKR_LOCK — RM0008 §9.2.7 (drv 层组装时序，LL 层提供原子写) */
static GpioAfio_ReturnType run_lock_sequence(GpioAfio_Port_e port, uint16_t pin_mask)
{
    /* IR: atomic_sequences[SEQ_LCKR_LOCK] — RM0008 §9.2.7 */
    uint32_t lck      = (uint32_t)pin_mask;
    uint32_t lckk_msk = GPIO_LCKR_LCKK_Msk;

    GpioAfio_LL_WriteLckr(port, lck | lckk_msk);  /* Step 1: LCKK=1 + mask */
    GpioAfio_LL_WriteLckr(port, lck);              /* Step 2: LCKK=0 + mask */
    GpioAfio_LL_WriteLckr(port, lck | lckk_msk);  /* Step 3: LCKK=1 + mask */
    (void)GpioAfio_LL_ReadLckr(port);              /* Step 4: dummy read */
    if ((GpioAfio_LL_ReadLckr(port) & lckk_msk) == 0U) {
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
        return GPIO_LOCKED;
    }

    GpioAfio_LL_EnableClockGpio(pConfig->port);
    GpioAfio_LL_ApplyPinModeMask(pConfig->port, pConfig->pin_mask, pConfig->mode);

    if (pConfig->mode == GPIO_MODE_INPUT_PUPD) {
        if (pConfig->pull == GPIO_PULL_UP) {
            GpioAfio_LL_SetPin(pConfig->port, pConfig->pin_mask);
        } else if (pConfig->pull == GPIO_PULL_DOWN) {
            GpioAfio_LL_ResetPin(pConfig->port, pConfig->pin_mask);
        }
    }
    return GPIO_OK;
}

GpioAfio_ReturnType GpioAfio_DeInit(GpioAfio_Port_e port)
{
    if (port >= GPIO_PORT_COUNT) { return GPIO_PARAM; }
    if (GpioAfio_LL_IsLocked(port)) { return GPIO_LOCKED; }
    GpioAfio_LL_ResetBank(port);
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
    GpioAfio_LL_TogglePins(port, pin_mask);
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
    GpioAfio_LL_EnableClockAfio();
    GpioAfio_LL_SetExtiSource(exti_line, port);
    return GPIO_OK;
}
