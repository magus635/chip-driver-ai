/**
 * @file spi_ll.h
 * @brief SPI Low-Level Driver for STM32F103
 * @details Implements register-level operations with chip-specific quirk handling.
 */

#ifndef SPI_LL_H
#define SPI_LL_H

#include "spi_reg.h"
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Initialize SPI peripheral
 * @param SPIx Pointer to SPI register base
 * @param config Configuration value for CR1
 */
static inline void Spi_LL_Init(Spi_Reg_t *SPIx, uint32_t config)
{
    /* Apply configuration to CR1 */
    SPIx->CR1 = config;

    /**
     * @note Chip Knowledge Quirk: STM32F1 Master Mode SSM/SSI requirements.
     * If SSM (Software Slave Management) is enabled and MSTR is set,
     * SSI must be set to 1 to avoid MODF (Mode Fault) error.
     */
    if ((SPIx->CR1 & SPI_CR1_SSM_MSK) && (SPIx->CR1 & SPI_CR1_MSTR_MSK))
    {
        SPIx->CR1 |= SPI_CR1_SSI_MSK;
    }
}

/**
 * @brief Enable SPI peripheral
 * @param SPIx Pointer to SPI register base
 */
static inline void Spi_LL_Enable(Spi_Reg_t *SPIx)
{
    SPIx->CR1 |= SPI_CR1_SPE_MSK;
}

/**
 * @brief Disable SPI peripheral
 * @param SPIx Pointer to SPI register base
 */
static inline void Spi_LL_Disable(Spi_Reg_t *SPIx)
{
    SPIx->CR1 &= ~SPI_CR1_SPE_MSK;
}

/**
 * @brief Transmit data
 * @param SPIx Pointer to SPI register base
 * @param data Data to transmit (8 or 16 bits based on DFF)
 */
static inline void Spi_LL_WriteData(Spi_Reg_t *SPIx, uint16_t data)
{
    SPIx->DR = (uint32_t)data;
}

/**
 * @brief Read received data
 * @param SPIx Pointer to SPI register base
 * @return uint16_t Received data
 */
static inline uint16_t Spi_LL_ReadData(Spi_Reg_t *SPIx)
{
    return (uint16_t)(SPIx->DR & 0xFFFFU);
}

/**
 * @brief Check if TX buffer is empty
 * @param SPIx Pointer to SPI register base
 * @return true if TXE=1
 */
static inline bool Spi_LL_IsTxEmpty(Spi_Reg_t *SPIx)
{
    return (bool)(SPIx->SR & SPI_SR_TXE_MSK);
}

/**
 * @brief Check if RX buffer is not empty
 * @param SPIx Pointer to SPI register base
 * @return true if RXNE=1
 */
static inline bool Spi_LL_IsRxNotEmpty(Spi_Reg_t *SPIx)
{
    return (bool)(SPIx->SR & SPI_SR_RXNE_MSK);
}

/**
 * @brief Clear Overrun (OVR) flag
 * @param SPIx Pointer to SPI register base
 * @details 
 * Implementation follows @ref SEQ_OVR_CLEAR from IR and @ref Q001 from chip-knowledge.json.
 * OVR is cleared by a read operation to the SPI_DR register followed by 
 * a read operation to the SPI_SR register.
 */
static inline void Spi_LL_ClearOverrun(Spi_Reg_t *SPIx)
{
    /**
     * @note Chip Knowledge Application (Quirk Q001):
     * Explicitly perform the two-step read sequence to ensure hardware flag reset.
     */
    volatile uint32_t tmpreg;
    tmpreg = SPIx->DR;  /* Step 1: Read DR */
    tmpreg = SPIx->SR;  /* Step 2: Read SR */
    (void)tmpreg;       /* Prevent unused variable warning */
}

#endif /* SPI_LL_H */
