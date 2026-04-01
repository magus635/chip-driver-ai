#ifndef SPI_LL_H
#define SPI_LL_H

#include "spi_reg.h"
#include "spi_types.h"
#include <stdbool.h>

/* Layer 2: Low-Level Hardware Abstraction Layer
 * STRICT CONFORMITY: Atomic hardware operations, no state machine.
 */

static inline void SPI_LL_Enable(SPI_TypeDef *SPIx)
{
    SPIx->CR1 |= SPI_CR1_SPE_Msk;
}

static inline void SPI_LL_Disable(SPI_TypeDef *SPIx)
{
    SPIx->CR1 &= ~SPI_CR1_SPE_Msk;
}

static inline bool SPI_LL_IsEnabled(const SPI_TypeDef *SPIx)
{
    return ((SPIx->CR1 & SPI_CR1_SPE_Msk) != 0U);
}

static inline void SPI_LL_Configure(SPI_TypeDef *SPIx, const Spi_ConfigType *Config)
{
    uint32_t cr1 = 0U;

    /* Baudrate Prescaler: BR[2:0] */
    cr1 |= ((uint32_t)Config->baudrate_prescaler << SPI_CR1_BR_Pos) & SPI_CR1_BR_Msk;

    /* Polarity and Phase */
    if (Config->clk_polarity != 0U) cr1 |= SPI_CR1_CPOL_Msk;
    if (Config->clk_phase    != 0U) cr1 |= SPI_CR1_CPHA_Msk;

    /* Data Size */
    if (Config->data_size == SPI_DATA_16BIT) cr1 |= SPI_CR1_DFF_Msk;

    /* Master/Slave */
    if (Config->is_master) cr1 |= SPI_CR1_MSTR_Pos;

    /* SSM (Software Slave Management) */
    if (Config->use_ssm) {
        cr1 |= SPI_CR1_SSM_Msk;
        cr1 |= SPI_CR1_SSI_Msk; /* Set SSI to prevent Mode Fault in Master */
    }

    /* Apply CR1 */
    SPIx->CR1 = cr1;
}

static inline void SPI_LL_WriteData(SPI_TypeDef *SPIx, uint16_t Data)
{
    SPIx->DR = (uint32_t)Data;
}

static inline uint16_t SPI_LL_ReadData(const SPI_TypeDef *SPIx)
{
    return (uint16_t)(SPIx->DR & 0xFFFFU);
}

static inline bool SPI_LL_IsTxEmpty(const SPI_TypeDef *SPIx)
{
    return ((SPIx->SR & SPI_SR_TXE_Msk) != 0U);
}

static inline bool SPI_LL_IsRxNotEmpty(const SPI_TypeDef *SPIx)
{
    return ((SPIx->SR & SPI_SR_RXNE_Msk) != 0U);
}

static inline bool SPI_LL_IsBusy(const SPI_TypeDef *SPIx)
{
    return ((SPIx->SR & SPI_SR_BSY_Msk) != 0U);
}

/* 错误标志清除（W1C 封装） */
static inline void SPI_LL_ClearOverrun(SPI_TypeDef *SPIx)
{
    /* STM32F1xx OVR Clear Sequence: Read DR, then Read SR */
    (void)SPIx->DR;
    (void)SPIx->SR;
}

static inline uint32_t SPI_LL_GetStatusFlags(const SPI_TypeDef *SPIx)
{
    return SPIx->SR;
}

#endif
