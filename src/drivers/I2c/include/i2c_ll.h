/**
 * @file    i2c_ll.h
 * @brief   STM32F103C8T6 I2C 底层抽象 (Layer 2)
 */

#ifndef I2C_LL_H
#define I2C_LL_H

#include "i2c_reg.h"
#include "i2c_types.h"
#include <stdbool.h>

void I2C_LL_ConfigureTiming(I2C_TypeDef *I2Cx, const I2c_ConfigType *pConfig);

static inline void I2C_LL_Enable(I2C_TypeDef *I2Cx)  { I2Cx->CR1 |= I2C_CR1_PE_Msk; }
static inline void I2C_LL_Disable(I2C_TypeDef *I2Cx) { I2Cx->CR1 &= ~I2C_CR1_PE_Msk; }

static inline bool I2C_LL_IsEnabled(const I2C_TypeDef *I2Cx)
{
    return ((I2Cx->CR1 & I2C_CR1_PE_Msk) != 0U);
}

static inline void I2C_LL_WriteCR2(I2C_TypeDef *I2Cx, uint16_t val)
{
    I2Cx->CR2 = val;
}

static inline void I2C_LL_WriteCCR(I2C_TypeDef *I2Cx, uint16_t val)
{
    /* Guard: INV_I2C_001 (R16) — PE must be 0 to modify CCR */
    if (!I2C_LL_IsEnabled(I2Cx))
    {
        I2Cx->CCR = val;
    }
}

static inline void I2C_LL_WriteTRISE(I2C_TypeDef *I2Cx, uint16_t val)
{
    /* Guard: INV_I2C_001 (R16) — PE must be 0 to modify TRISE */
    if (!I2C_LL_IsEnabled(I2Cx))
    {
        I2Cx->TRISE = val;
    }
}

static inline uint32_t I2C_LL_GetDRAddress(const I2C_TypeDef *I2Cx)
{
    return (uint32_t)&(I2Cx->DR);
}

static inline void I2C_LL_GenerateStart(I2C_TypeDef *I2Cx) { I2Cx->CR1 |= I2C_CR1_START_Msk; }
static inline void I2C_LL_GenerateStop(I2C_TypeDef *I2Cx)  { I2Cx->CR1 |= I2C_CR1_STOP_Msk; }

static inline void I2C_LL_WriteDR(I2C_TypeDef *I2Cx, uint16_t val) { I2Cx->DR = val; }
static inline uint16_t I2C_LL_ReadDR(const I2C_TypeDef *I2Cx)      { return (uint16_t)I2Cx->DR; }

static inline void I2C_LL_EnableAck(I2C_TypeDef *I2Cx)  { I2Cx->CR1 |= I2C_CR1_ACK_Msk; }
static inline void I2C_LL_DisableAck(I2C_TypeDef *I2Cx) { I2Cx->CR1 &= ~I2C_CR1_ACK_Msk; }

static inline void I2C_LL_EnableDMA(I2C_TypeDef *I2Cx)  { I2Cx->CR2 |= I2C_CR2_DMAEN_Msk; }
static inline void I2C_LL_DisableDMA(I2C_TypeDef *I2Cx) { I2Cx->CR2 &= ~I2C_CR2_DMAEN_Msk; }

static inline void I2C_LL_EnableLastDMA(I2C_TypeDef *I2Cx) { I2Cx->CR2 |= I2C_CR2_LAST_Msk; }

static inline uint32_t I2C_LL_GetSR1(const I2C_TypeDef *I2Cx) { return I2Cx->SR1; }
static inline void I2C_LL_ClearAddrFlag(const I2C_TypeDef *I2Cx)
{
    (void)I2Cx->SR1;
    (void)I2Cx->SR2;
}

#endif /* I2C_LL_H */
