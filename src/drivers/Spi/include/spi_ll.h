/**
 * @file    spi_ll.h
 * @brief   STM32F103C8T6 SPI LL 层 (Layer 2 - Atomic Ops & Composition)
 *
 * Source:  ir/spi_ir_summary.json v3.0
 */

#ifndef SPI_LL_H
#define SPI_LL_H

#include "spi_reg.h"
#include "spi_types.h"

/*===========================================================================*/
/* Atomic Access Macros/Functions                                            */
/*===========================================================================*/

static inline void SPI_LL_Enable(SPI_TypeDef *SPIx)
{
    /* Guard: INV_SPI_003 — Lock SPE when BSY=1 */
    if (SPIx->SR & SPI_SR_BSY_Msk) { return; }
    SPIx->CR1 |= SPI_CR1_SPE_Msk;
}

static inline void SPI_LL_Disable(SPI_TypeDef *SPIx)
{
    /* Guard: INV_SPI_003 — Lock SPE when BSY=1 */
    if (SPIx->SR & SPI_SR_BSY_Msk) { return; }
    SPIx->CR1 &= ~SPI_CR1_SPE_Msk;
}

static inline bool SPI_LL_IsEnabled(const SPI_TypeDef *SPIx)
{
    return ((SPIx->CR1 & SPI_CR1_SPE_Msk) != 0U);
}

static inline uint32_t SPI_LL_GetSR(const SPI_TypeDef *SPIx)
{
    return SPIx->SR;
}

static inline void SPI_LL_WriteDR(SPI_TypeDef *SPIx, uint16_t data)
{
    SPIx->DR = data;
}

static inline uint16_t SPI_LL_ReadDR(const SPI_TypeDef *SPIx)
{
    return (uint16_t)SPIx->DR;
}

static inline void SPI_LL_WriteCR1(SPI_TypeDef *SPIx, uint16_t val)
{
    /* Guard: INV_SPI_002/004 — Locked fields when SPE=1 */
    /* Guard: INV_SPI_003 — Lock SPE when BSY=1 */
    if (SPIx->CR1 & SPI_CR1_SPE_Msk) { return; }
    if (SPIx->SR & SPI_SR_BSY_Msk) { return; }
    SPIx->CR1 = val;
}

static inline void SPI_LL_WriteCR2(SPI_TypeDef *SPIx, uint16_t val)
{
    SPIx->CR2 = val;
}

static inline void SPI_LL_WriteCRCPR(SPI_TypeDef *SPIx, uint16_t val)
{
    /* Guard: INV_SPI_002/004 — Locked when SPE=1 */
    if (SPIx->CR1 & SPI_CR1_SPE_Msk) { return; }
    SPIx->CRCPR = val;
}

static inline void SPI_LL_EnableDmaRequests(SPI_TypeDef *SPIx)
{
    SPIx->CR2 |= (SPI_CR2_TXDMAEN_Msk | SPI_CR2_RXDMAEN_Msk);
}

static inline void SPI_LL_DisableDmaRequests(SPI_TypeDef *SPIx)
{
    SPIx->CR2 &= ~(SPI_CR2_TXDMAEN_Msk | SPI_CR2_RXDMAEN_Msk);
}

static inline void SPI_LL_ClearCRC(SPI_TypeDef *SPIx)
{
    /* Clear CRCERR by writing 0 to SR.CRCERR bit */
    SPIx->SR &= ~SPI_SR_CRCERR_Msk;
}

/*===========================================================================*/
/* Composition Helpers (Implemented in spi_ll.c per Guidelines §2)          */
/*===========================================================================*/

uint16_t SPI_LL_ComposeCR1(const Spi_ConfigType *pConfig);
uint16_t SPI_LL_ComposeCR2(const Spi_ConfigType *pConfig);

#endif /* SPI_LL_H */
