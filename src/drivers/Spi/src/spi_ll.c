/**
 * @file    spi_ll.c
 * @brief   STM32F103C8T6 SPI LL 层实现 (Complex Logic Composition)
 *
 * Source:  ir/spi_ir_summary.json v3.0
 */

#include "spi_ll.h"

/**
 * @brief  Compose CR1 register value from configuration.
 * @param  pConfig Pointer to SPI configuration.
 * @return 16-bit CR1 value.
 */
uint16_t SPI_LL_ComposeCR1(const Spi_ConfigType *pConfig)
{
    uint16_t cr1 = 0U;

    /* Mode Selection */
    switch (pConfig->mode)
    {
        case SPI_MODE_FULL_DUPLEX_MASTER:
            cr1 |= SPI_CR1_MSTR_Msk;
            break;
        case SPI_MODE_FULL_DUPLEX_SLAVE:
            /* MSTR=0 */
            break;
        case SPI_MODE_HALF_DUPLEX_TX:
            cr1 |= (SPI_CR1_MSTR_Msk | SPI_CR1_BIDIMODE_Msk | SPI_CR1_BIDIOE_Msk);
            break;
        case SPI_MODE_HALF_DUPLEX_RX:
            cr1 |= (SPI_CR1_MSTR_Msk | SPI_CR1_BIDIMODE_Msk);
            /* BIDIOE=0 */
            break;
        case SPI_MODE_SIMPLEX_RX_MASTER:
            cr1 |= (SPI_CR1_MSTR_Msk | SPI_CR1_RXONLY_Msk);
            break;
        default:
            break;
    }

    /* Clock Configuration */
    cr1 |= ((uint16_t)pConfig->baudRate << SPI_CR1_BR_Pos) & SPI_CR1_BR_Msk;
    if (pConfig->cpol == SPI_CPOL_HIGH)  { cr1 |= SPI_CR1_CPOL_Msk; }
    if (pConfig->cpha == SPI_CPHA_2EDGE) { cr1 |= SPI_CR1_CPHA_Msk; }

    /* Data Format */
    if (pConfig->dataFrame == SPI_DFF_16BIT) { cr1 |= SPI_CR1_DFF_Msk; }
    if (pConfig->firstBit == SPI_FIRSTBIT_LSB) { cr1 |= SPI_CR1_LSBFIRST_Msk; }

    /* NSS Management (INV_SPI_001 Implementation) */
    if (pConfig->nssMode == SPI_NSS_SOFT)
    {
        cr1 |= SPI_CR1_SSM_Msk;
        if (pConfig->mode != SPI_MODE_FULL_DUPLEX_SLAVE)
        {
            /* Guard: INV_SPI_001 — Master with Software NSS must have SSI=1 */
            cr1 |= SPI_CR1_SSI_Msk;
        }
    }

    /* CRC */
    if (pConfig->crcEnable) { cr1 |= SPI_CR1_CRCEN_Msk; }

    return cr1;
}

/**
 * @brief  Compose CR2 register value from configuration.
 * @param  pConfig Pointer to SPI configuration.
 * @return 16-bit CR2 value.
 */
uint16_t SPI_LL_ComposeCR2(const Spi_ConfigType *pConfig)
{
    uint16_t cr2 = 0U;

    /* Hardware NSS Output (Only valid in Master mode) */
    if (pConfig->nssMode == SPI_NSS_HARD_OUT)
    {
        cr2 |= SPI_CR2_SSOE_Msk;
    }

    return cr2;
}
