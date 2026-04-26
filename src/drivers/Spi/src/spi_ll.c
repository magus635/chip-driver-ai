/**
 * @file    spi_ll.c
 * @brief   STM32F103C8T6 SPI LL 层非 inline 实现 (Composition)
 *
 * @note    遵循 Guidelines §2 判定准则：复杂寄存器合成逻辑强制下沉至此文件。
 *          Driver 层禁止直接进行位偏移拼接逻辑 (AR-01)。
 *
 * Source:  RM0008 Rev 21
 *          ir/spi_ir_summary.json v3.0
 */

#include "spi_ll.h"

/**
 * @brief  Compose CR1 value from configuration.
 *         Guard: INV_SPI_001/002/004 — logic encapsulated here.
 * @param  pConfig  Pointer to SPI configuration.
 * @return 16-bit CR1 register value.
 */
uint16_t SPI_LL_ComposeCR1(const Spi_ConfigType *pConfig)
{
    uint16_t cr1 = 0U;

    /* Mode bits — ir/spi_ir_summary.json functional_model.operating_modes[] */
    switch (pConfig->mode)
    {
        case SPI_MODE_FULL_DUPLEX_MASTER:
            cr1 |= SPI_CR1_MSTR_Msk;   /* RM0008 §25.3.1 p.675 */
            break;
        case SPI_MODE_FULL_DUPLEX_SLAVE:
            /* MSTR=0 already */
            break;
        case SPI_MODE_HALF_DUPLEX_TX:
            cr1 |= SPI_CR1_MSTR_Msk | SPI_CR1_BIDIMODE_Msk | SPI_CR1_BIDIOE_Msk;
            break;   /* RM0008 §25.3.4 p.681 */
        case SPI_MODE_HALF_DUPLEX_RX:
            cr1 |= SPI_CR1_MSTR_Msk | SPI_CR1_BIDIMODE_Msk;
            break;   /* BIDIOE=0 */
        case SPI_MODE_SIMPLEX_RX_MASTER:
            cr1 |= SPI_CR1_MSTR_Msk | SPI_CR1_RXONLY_Msk;
            break;   /* RM0008 §25.3.3 p.679 */
        default:
            break;
    }

    /* Clock config — RM0008 §25.5.1 p.715 */
    cr1 |= ((uint16_t)pConfig->baudRate << SPI_CR1_BR_Pos) & SPI_CR1_BR_Msk;

    if (pConfig->cpol == SPI_CPOL_HIGH)
    {
        cr1 |= SPI_CR1_CPOL_Msk;
    }
    if (pConfig->cpha == SPI_CPHA_2EDGE)
    {
        cr1 |= SPI_CR1_CPHA_Msk;
    }

    /* Data frame format — IR: registers[0] CR1.DFF */
    if (pConfig->dataFrame == SPI_DFF_16BIT)
    {
        cr1 |= SPI_CR1_DFF_Msk;
    }

    /* Bit order — IR: registers[0] CR1.LSBFIRST */
    if (pConfig->firstBit == SPI_FIRSTBIT_LSB)
    {
        cr1 |= SPI_CR1_LSBFIRST_Msk;
    }

    /* NSS management — IR: registers[0] CR1.SSM/SSI */
    if (pConfig->nssMode == SPI_NSS_SOFT)
    {
        cr1 |= SPI_CR1_SSM_Msk;
        /* Guard: INV_SPI_001 — MSTR=1 && SSM=1 => SSI must be 1 */
        if ((cr1 & SPI_CR1_MSTR_Msk) != 0U)
        {
            cr1 |= SPI_CR1_SSI_Msk;   /* IR: invariants[0] — RM0008 §25.3.1 p.677 */
        }
    }

    /* CRC Enable — IR: registers[0] CR1.CRCEN */
    if (pConfig->crcEnable)
    {
        cr1 |= SPI_CR1_CRCEN_Msk;
    }

    return cr1;
}

/**
 * @brief  Compose CR2 value from configuration.
 * @param  pConfig  Pointer to SPI configuration.
 * @return 16-bit CR2 register value.
 */
uint16_t SPI_LL_ComposeCR2(const Spi_ConfigType *pConfig)
{
    uint16_t cr2 = 0U;

    /* Hardware NSS output — RM0008 §25.5.2 p.716 */
    if (pConfig->nssMode == SPI_NSS_HARD_OUT)
    {
        cr2 |= SPI_CR2_SSOE_Msk;
    }

    return cr2;
}
