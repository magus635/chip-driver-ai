/**
 * @file spi_reg.h
 * @brief SPI Register Definitions for STM32F103
 * @details Generated from RM0008 Rev 21
 */

#ifndef SPI_REG_H
#define SPI_REG_H

#include <stdint.h>

/**
 * @brief SPI Register Map Structure
 * @see RM0008 §25.5 p.714
 */
typedef struct
{
    volatile uint32_t CR1;      /*!< SPI control register 1,        Address offset: 0x00 */
    volatile uint32_t CR2;      /*!< SPI control register 2,        Address offset: 0x04 */
    volatile uint32_t SR;       /*!< SPI status register,           Address offset: 0x08 */
    volatile uint32_t DR;       /*!< SPI data register,             Address offset: 0x0C */
    volatile uint32_t CRCPR;    /*!< SPI CRC polynomial register,   Address offset: 0x10 */
    volatile uint32_t RXCRCR;   /*!< SPI RX CRC register,           Address offset: 0x14 */
    volatile uint32_t TXCRCR;   /*!< SPI TX CRC register,           Address offset: 0x18 */
    volatile uint32_t I2SCFGR;  /*!< SPI_I2S configuration register, Address offset: 0x1C */
    volatile uint32_t I2SPR;    /*!< SPI_I2S prescaler register,    Address offset: 0x20 */
} Spi_Reg_t;

/* --- Register Bit Definitions --- */

/* CR1 - Control Register 1 */
#define SPI_CR1_CPHA_POS         (0U)
#define SPI_CR1_CPHA_MSK         (1U << SPI_CR1_CPHA_POS)
#define SPI_CR1_CPOL_POS         (1U)
#define SPI_CR1_CPOL_MSK         (1U << SPI_CR1_CPOL_POS)
#define SPI_CR1_MSTR_POS         (2U)
#define SPI_CR1_MSTR_MSK         (1U << SPI_CR1_MSTR_POS)
#define SPI_CR1_BR_POS           (3U)
#define SPI_CR1_BR_MSK           (7U << SPI_CR1_BR_POS)
#define SPI_CR1_SPE_POS          (6U)
#define SPI_CR1_SPE_MSK          (1U << SPI_CR1_SPE_POS)
#define SPI_CR1_LSBFIRST_POS     (7U)
#define SPI_CR1_LSBFIRST_MSK     (1U << SPI_CR1_LSBFIRST_POS)
#define SPI_CR1_SSI_POS          (8U)
#define SPI_CR1_SSI_MSK          (1U << SPI_CR1_SSI_POS)
#define SPI_CR1_SSM_POS          (9U)
#define SPI_CR1_SSM_MSK          (1U << SPI_CR1_SSM_POS)
#define SPI_CR1_RXONLY_POS       (10U)
#define SPI_CR1_RXONLY_MSK       (1U << SPI_CR1_RXONLY_POS)
#define SPI_CR1_DFF_POS          (11U)
#define SPI_CR1_DFF_MSK          (1U << SPI_CR1_DFF_POS)
#define SPI_CR1_BIDIOE_POS       (14U)
#define SPI_CR1_BIDIOE_MSK       (1U << SPI_CR1_BIDIOE_POS)
#define SPI_CR1_BIDIMODE_POS     (15U)
#define SPI_CR1_BIDIMODE_MSK     (1U << SPI_CR1_BIDIMODE_POS)

/* SR - Status Register */
#define SPI_SR_RXNE_POS          (0U)
#define SPI_SR_RXNE_MSK          (1U << SPI_SR_RXNE_POS)
#define SPI_SR_TXE_POS           (1U)
#define SPI_SR_TXE_MSK           (1U << SPI_SR_TXE_POS)
#define SPI_SR_CRCERR_POS        (4U)
#define SPI_SR_CRCERR_MSK        (1U << SPI_SR_CRCERR_POS)
#define SPI_SR_MODF_POS          (5U)
#define SPI_SR_MODF_MSK          (1U << SPI_SR_MODF_POS)
#define SPI_SR_OVR_POS           (6U)
#define SPI_SR_OVR_MSK           (1U << SPI_SR_OVR_POS)
#define SPI_SR_BSY_POS           (7U)
#define SPI_SR_BSY_MSK           (1U << SPI_SR_BSY_POS)

/* --- Instance Map --- */
#define SPI1_BASE                (0x40013000U)
#define SPI2_BASE                (0x40003800U)
#define SPI3_BASE                (0x40003C00U)

#define SPI1                     ((Spi_Reg_t *)SPI1_BASE)
#define SPI2                     ((Spi_Reg_t *)SPI2_BASE)
#define SPI3                     ((Spi_Reg_t *)SPI3_BASE)

#endif /* SPI_REG_H */
