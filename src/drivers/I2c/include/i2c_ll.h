#ifndef I2C_LL_H
#define I2C_LL_H

#include "i2c_reg.h"
#include "i2c_types.h"
#include <stdbool.h>

/**
 * @file i2c_ll.h
 * @brief I2C Low-Level Hardware Abstraction Layer
 * @note V2.1 Atomic Sequence Compliance
 * 
 * Layer 2: Low-Level Hardware Abstraction
 * STRICT CONFORMITY: Atomic hardware operations, no state machine.
 */

/* Peripheral Control */

static inline void I2C_LL_Enable(I2C_TypeDef *I2Cx)
{
    I2Cx->CR1 |= I2C_CR1_PE_Msk;
}

static inline void I2C_LL_Disable(I2C_TypeDef *I2Cx)
{
    I2Cx->CR1 &= ~I2C_CR1_PE_Msk;
}

static inline bool I2C_LL_IsEnabled(const I2C_TypeDef *I2Cx)
{
    return ((I2Cx->CR1 & I2C_CR1_PE_Msk) != 0U);
}

/* Generation of Start/Stop */

static inline void I2C_LL_GenerateStart(I2C_TypeDef *I2Cx)
{
    I2Cx->CR1 |= I2C_CR1_START_Msk;
}

static inline void I2C_LL_GenerateStop(I2C_TypeDef *I2Cx)
{
    I2Cx->CR1 |= I2C_CR1_STOP_Msk;
}

static inline void I2C_LL_AcknowledgeNext(I2C_TypeDef *I2Cx, bool AckEnable)
{
    if (AckEnable) {
        I2Cx->CR1 |= I2C_CR1_ACK_Msk;
    } else {
        I2Cx->CR1 &= ~I2C_CR1_ACK_Msk;
    }
}

/* Status Checking */

static inline bool I2C_LL_IsStartSent(const I2C_TypeDef *I2Cx)
{
    return ((I2Cx->SR1 & I2C_SR1_SB_Msk) != 0U);
}

static inline bool I2C_LL_IsAddressSent(const I2C_TypeDef *I2Cx)
{
    return ((I2Cx->SR1 & I2C_SR1_ADDR_Msk) != 0U);
}

static inline bool I2C_LL_IsTxEmpty(const I2C_TypeDef *I2Cx)
{
    return ((I2Cx->SR1 & I2C_SR1_TXE_Msk) != 0U);
}

static inline bool I2C_LL_IsRxNotEmpty(const I2C_TypeDef *I2Cx)
{
    return ((I2Cx->SR1 & I2C_SR1_RXNE_Msk) != 0U);
}

static inline bool I2C_LL_IsByteFinished(const I2C_TypeDef *I2Cx)
{
    return ((I2Cx->SR1 & I2C_SR1_BTF_Msk) != 0U);
}

static inline bool I2C_LL_IsBusy(const I2C_TypeDef *I2Cx)
{
    return ((I2Cx->SR2 & I2C_SR2_BUSY_Msk) != 0U);
}

/* Atomic Flag Clearing (Strict Compliance) */

/**
 * @brief SEQ_I2C_ADDR_CLEAR: Clears the ADDR flag.
 * @details RM0008 §26.6.7: Cleared by software by reading I2C_SR1, then I2C_SR2.
 */
static inline void I2C_LL_ClearAddressFlag(I2C_TypeDef *I2Cx)
{
    uint32_t tmp;
    tmp = I2Cx->SR1; /* Read SR1 (ADDR is set) */
    tmp = I2Cx->SR2; /* Read SR2 (Clears ADDR) */
    (void)tmp;       /* Prevent compiler optimization */
}

/* Error Management */

static inline bool I2C_LL_IsAckFailure(const I2C_TypeDef *I2Cx)
{
    return ((I2Cx->SR1 & I2C_SR1_AF_Msk) != 0U);
}

static inline void I2C_LL_ClearAckFailure(I2C_TypeDef *I2Cx)
{
    I2Cx->SR1 &= ~I2C_SR1_AF_Msk;
}

static inline uint32_t I2C_LL_GetErrorFlags(const I2C_TypeDef *I2Cx)
{
    /* BERR, ARLO, AF, OVR, PECERR, TIMEOUT */
    return (I2Cx->SR1 & (I2C_SR1_BERR_Msk | I2C_SR1_ARLO_Msk | I2C_SR1_AF_Msk | 
                         I2C_SR1_OVR_Msk | I2C_SR1_PECERR_Msk | I2C_SR1_TIMEOUT_Msk));
}

/* Data Transfer */

static inline void I2C_LL_WriteData(I2C_TypeDef *I2Cx, uint8_t Data)
{
    I2Cx->DR = (uint32_t)Data;
}

static inline uint8_t I2C_LL_ReadData(const I2C_TypeDef *I2Cx)
{
    return (uint8_t)(I2Cx->DR & 0xFFU);
}

/* Configuration */

static inline void I2C_LL_Configure(I2C_TypeDef *I2Cx, const I2C_ConfigType *Config)
{
    uint32_t tmp;

    /* 1. Configure Clock Frequency FREQ[5:0] in CR2 */
    tmp = I2Cx->CR2 & ~I2C_CR2_FREQ_Msk;
    tmp |= (Config->pclk1_freq / 1000000U) & I2C_CR2_FREQ_Msk;
    I2Cx->CR2 = tmp;

    /* 2. Disable I2C to configure CCR and TRISE */
    I2Cx->CR1 &= ~I2C_CR1_PE_Msk;

    /* 3. Configure CCR */
    uint32_t ccr = 0U;
    if (Config->clock_speed <= I2C_SPEED_STANDARD) {
        /* Standard Mode */
        ccr = Config->pclk1_freq / (Config->clock_speed * 2U);
        if (ccr < 4U) ccr = 4U;
    } else {
        /* Fast Mode */
        ccr |= I2C_CCR_FS_Msk;
        if (Config->duty_cycle == I2C_DUTYCYCLE_2) {
            ccr |= Config->pclk1_freq / (Config->clock_speed * 3U);
        } else {
            ccr |= I2C_CCR_DUTY_Msk;
            ccr |= Config->pclk1_freq / (Config->clock_speed * 25U);
        }
        if ((ccr & I2C_CCR_CCR_Msk) == 0U) ccr |= 1U;
    }
    I2Cx->CCR = ccr & (I2C_CCR_FS_Msk | I2C_CCR_DUTY_Msk | I2C_CCR_CCR_Msk);

    /* 4. Configure TRISE */
    if (Config->clock_speed <= I2C_SPEED_STANDARD) {
        I2Cx->TRISE = (Config->pclk1_freq / 1000000U) + 1U;
    } else {
        I2Cx->TRISE = ((Config->pclk1_freq * 300U) / 1000000000U) + 1U;
    }
}

#endif /* I2C_LL_H */
