/**
 * @file    i2c_ll.c
 * @brief   STM32F103C8T6 I2C LL 层复杂实现 (Layer 2)
 */

#include "i2c_ll.h"
#include <stddef.h>

void I2C_LL_ConfigureTiming(I2C_TypeDef *I2Cx, const I2c_ConfigType *pConfig)
{
    if (pConfig == NULL) return;

    /* 1. Calculate APB frequency value (FREQ bits in CR2) */
    uint16_t freq = (uint16_t)(pConfig->pclk1 / 1000000UL);
    I2Cx->CR2 = (I2Cx->CR2 & ~I2C_CR2_FREQ_Msk) | (freq & I2C_CR2_FREQ_Msk);

    /* 2. Calculate Clock Control Register (CCR) */
    /* Note: Guard INV_I2C_001 is satisfied because this function is called when PE=0 */
    uint16_t ccr = (uint16_t)(pConfig->pclk1 / (pConfig->speed * 2U));
    if (ccr < 4U) {
        ccr = 4U;
    }
    I2Cx->CCR = ccr;

    /* 3. Calculate Maximum Rise Time (TRISE) */
    I2Cx->TRISE = freq + 1U;
}
