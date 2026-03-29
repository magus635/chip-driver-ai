#ifndef SPI_LL_H
#define SPI_LL_H

#include "spi_reg.h"
#include "spi_types.h"
#include <stdbool.h>

/* Layer 2: Low-Level Hardware Abstraction
 * STRICT CONFORMITY: Atomic operations, isolated W1C manipulation.
 */

static inline void SPI_LL_Enable(SPI_TypeDef *SPIx) {
    SPIx->CR1 |= SPI_CR1_SPE_Msk;
}

static inline void SPI_LL_Disable(SPI_TypeDef *SPIx) {
    SPIx->CR1 &= ~SPI_CR1_SPE_Msk;
}

static inline uint32_t SPI_LL_GetSR(const SPI_TypeDef *SPIx) {
    return SPIx->SR;
}

static inline void SPI_LL_Configure(SPI_TypeDef *SPIx, const Spi_ConfigType *cfg) {
    uint32_t cr1 = 0;
    
    /* Config CR1: Prescaler, Mode, CPOL, CPHA, DFF, SSM/SSI */
    cr1 |= (cfg->baudrate_prescaler << SPI_CR1_BR_Pos) & SPI_CR1_BR_Msk;
    
    if (cfg->clk_polarity != 0U) cr1 |= SPI_CR1_CPOL_Msk;
    if (cfg->clk_phase != 0U)    cr1 |= SPI_CR1_CPHA_Msk;
    if (cfg->is_master)          cr1 |= SPI_CR1_MSTR_Msk;
    if (cfg->data_size == SPI_DATA_16BIT) cr1 |= SPI_CR1_DFF_Msk;
    
    if (cfg->use_ssm) {
        cr1 |= SPI_CR1_SSM_Msk | SPI_CR1_SSI_Msk;
    }
    
    SPIx->CR1 = cr1;
}

static inline bool SPI_LL_IsTxEmpty(const SPI_TypeDef *SPIx) {
    return ((SPIx->SR & SPI_SR_TXE_Msk) != 0U);
}

static inline bool SPI_LL_IsRxNotEmpty(const SPI_TypeDef *SPIx) {
    return ((SPIx->SR & SPI_SR_RXNE_Msk) != 0U);
}

static inline void SPI_LL_WriteData8(SPI_TypeDef *SPIx, uint8_t data) {
    *(__IO uint8_t *)&SPIx->DR = data;
}

static inline uint8_t SPI_LL_ReadData8(const SPI_TypeDef *SPIx) {
    return (uint8_t)(*(__I uint8_t *)&SPIx->DR);
}

static inline uint16_t SPI_LL_ReadData16(const SPI_TypeDef *SPIx) {
    return (uint16_t)(SPIx->DR);
}

#endif 
