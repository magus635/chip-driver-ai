/**
 * @file    gpio_afio_api.h
 * @brief   GPIO + AFIO Layer 4 — public API
 * Source:  ir/gpio_afio_ir_summary.json v3.0
 */
#ifndef GPIO_AFIO_API_H
#define GPIO_AFIO_API_H

#include "gpio_afio_types.h"
#include "gpio_afio_cfg.h"

/* Init / DeInit */
GpioAfio_ReturnType GpioAfio_Init(const GpioAfio_PinConfigType *pConfig);
GpioAfio_ReturnType GpioAfio_DeInit(GpioAfio_Port_e port);

/* Pin I/O */
GpioAfio_ReturnType GpioAfio_WritePin(GpioAfio_Port_e port, uint16_t pin_mask, bool level);
GpioAfio_ReturnType GpioAfio_TogglePin(GpioAfio_Port_e port, uint16_t pin_mask);
bool                GpioAfio_ReadPin(GpioAfio_Port_e port, uint8_t pin);

/* Locking — runs SEQ_LCKR_LOCK; irreversible until reset (INV_GPIO_001) */
GpioAfio_ReturnType GpioAfio_LockPins(GpioAfio_Port_e port, uint16_t pin_mask);

/* AFIO — EXTI source select */
GpioAfio_ReturnType GpioAfio_SetExtiSource(uint8_t exti_line, GpioAfio_Port_e port);

#endif /* GPIO_AFIO_API_H */
