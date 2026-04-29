/**
 * @file    gpioafio_api.h
 * @brief   GPIO + AFIO Layer 4 — public API
 * Source:  ir/gpioafio_ir_summary.json v3.0
 */
#ifndef GPIO_AFIO_API_H
#define GPIO_AFIO_API_H

#include "gpioafio_types.h"
#include "gpioafio_cfg.h"

/**
 * @brief Configure GPIO bank+pins per pConfig and enable IOPxEN clock.
 */
GpioAfio_ReturnType GpioAfio_Init(const GpioAfio_PinConfigType *pConfig);

/**
 * @brief Reset GPIO bank registers to power-on default (CRL/CRH=0x44444444, ODR=0).
 */
GpioAfio_ReturnType GpioAfio_DeInit(GpioAfio_Port_e port);

/**
 * @brief Atomically drive multiple pins high or low via single BSRR write.
 */
GpioAfio_ReturnType GpioAfio_WritePin(GpioAfio_Port_e port, uint16_t pin_mask, bool level);

/**
 * @brief Toggle pins (read ODR, compose set/reset masks, single BSRR write).
 */
GpioAfio_ReturnType GpioAfio_TogglePin(GpioAfio_Port_e port, uint16_t pin_mask);

/**
 * @brief Read instantaneous IDR level of a single pin.
 */
bool                GpioAfio_ReadPin(GpioAfio_Port_e port, uint8_t pin);

/**
 * @brief Run SEQ_LCKR_LOCK on a bank — irreversible until MCU reset (INV_GPIO_001).
 */
GpioAfio_ReturnType GpioAfio_LockPins(GpioAfio_Port_e port, uint16_t pin_mask);

/**
 * @brief Select GPIO port as the source for an EXTI line; auto-enables AFIOEN clock.
 */
GpioAfio_ReturnType GpioAfio_SetExtiSource(uint8_t exti_line, GpioAfio_Port_e port);

#endif /* GPIO_AFIO_API_H */
