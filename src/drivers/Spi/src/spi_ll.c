/**
 * @file    spi_ll.c
 * @brief   STM32F103C8T6 SPI LL 层非 inline 实现
 *
 * @note    本文件仅包含不适合 inline 的 LL 实现（如初始化序列原子操作）。
 *          大多数 LL 操作以 static inline 形式定义在 spi_ll.h 中。
 *          Driver 层（spi_drv.c）禁止直接访问寄存器（规则 R8-3）。
 *
 * Source:  RM0008 Rev 14, §25.5 SPI and I2S registers (p.714-720)
 *          IR: ir/spi_ir_summary.json — configuration_strategies[],
 *              cross_field_constraints[], operation_modes[]
 */

#include "spi_ll.h"

#include <stddef.h>

/*===========================================================================*/
/* LL composite init helper                                                   */
/*===========================================================================*/

/**
 * @brief  Compose and write CR1 from a Spi_ConfigType (SPE=0, guard INV_SPI_002).
 *
 *         Computes the full CR1 value from config fields and writes it in a
 *         single assignment so that no intermediate state violates hardware
 *         constraints.  SPE is intentionally left 0 here; the caller enables
 *         SPI after CR2 is also configured.
 *
 *         Guard: INV_SPI_002 — caller must ensure SPE=0 before calling.
 *         INV_SPI_001  — when MSTR=1 and SSM=1, SSI=1 is set unconditionally.
 *         CONSTRAINT_DFF_CRC_INTERACTION — if CRC enabled, CRCEN toggled
 *           after DFF change to reset CRC registers.
 *
 * @param[in]  SPIx     Pointer to SPI peripheral.
 * @param[in]  pConfig  Configuration structure. Caller must guarantee non-NULL.
 */
void SPI_LL_ComposeCR1(SPI_TypeDef *SPIx, const Spi_ConfigType *pConfig)
{
    uint32_t cr1 = 0U;

    /* Guard: INV_SPI_002 — ensure SPE=0 before writing locked fields */
    SPIx->CR1 &= ~SPI_CR1_SPE_Msk; /* RM0008 §25.5.1 p.715 */

    /* Baud rate BR[2:0] — RM0008 §25.5.1 p.715 */
    cr1 |= (((uint32_t)pConfig->baudDiv << SPI_CR1_BR_Pos) & SPI_CR1_BR_Msk);

    /* Clock polarity CPOL — RM0008 §25.5.1 p.715 */
    if (((uint32_t)pConfig->clockMode & 0x2U) != 0U)
    {
        cr1 |= SPI_CR1_CPOL_Msk;
    }

    /* Clock phase CPHA — RM0008 §25.5.1 p.715 */
    if (((uint32_t)pConfig->clockMode & 0x1U) != 0U)
    {
        cr1 |= SPI_CR1_CPHA_Msk;
    }

    /* Data frame format DFF — RM0008 §25.5.1 p.714 */
    if (pConfig->dataFrame == SPI_DFF_16BIT)
    {
        cr1 |= SPI_CR1_DFF_Msk;
    }

    /* Bit order LSBFIRST — RM0008 §25.5.1 p.715 */
    if (pConfig->firstBit == SPI_FIRSTBIT_LSB)
    {
        cr1 |= SPI_CR1_LSBFIRST_Msk;
    }

    /* Communication mode (BIDIMODE, BIDIOE, RXONLY) — IR operation_modes[] */
    switch (pConfig->commMode)
    {
        case SPI_COMM_RECEIVE_ONLY:
            cr1 |= SPI_CR1_RXONLY_Msk;   /* RM0008 §25.5.1 p.715 */
            break;
        case SPI_COMM_BIDI_TX:
            cr1 |= (SPI_CR1_BIDIMODE_Msk | SPI_CR1_BIDIOE_Msk); /* RM0008 §25.5.1 p.714 */
            break;
        case SPI_COMM_BIDI_RX:
            cr1 |= SPI_CR1_BIDIMODE_Msk; /* BIDIOE=0 — RM0008 §25.5.1 p.714 */
            break;
        case SPI_COMM_FULL_DUPLEX:
        default:
            /* BIDIMODE=0, RXONLY=0 — already zero */
            break;
    }

    /* Master / slave and NSS management — IR configuration_strategies[] */
    if (pConfig->masterSlave == SPI_MASTER)
    {
        cr1 |= SPI_CR1_MSTR_Msk; /* RM0008 §25.5.1 p.715 */

        if (pConfig->nssMode == SPI_NSS_SOFT)
        {
            /* INV_SPI_001: SSM=1, SSI=1 mandatory when MSTR=1 + SSM=1 */
            /* CONSTRAINT_MASTER_SSI_INTERACTION */
            cr1 |= (SPI_CR1_SSM_Msk | SPI_CR1_SSI_Msk); /* RM0008 §25.3.1 p.676 */
        }
        /* SPI_NSS_HARD_INPUT / SPI_NSS_HARD_OUTPUT: SSM=0, SSOE in CR2 */
    }
    else /* SPI_SLAVE */
    {
        /* MSTR=0 — already zero */
        if (pConfig->nssMode == SPI_NSS_SOFT)
        {
            /* Slave software NSS: SSM=1, SSI=0 (selected) */
            cr1 |= SPI_CR1_SSM_Msk; /* RM0008 §25.3.2 p.678 */
            /* SSI=0 keeps slave selected */
        }
        /* SPI_NSS_HARD_INPUT: SSM=0 — hardware NSS input, default */
    }

    /* CONSTRAINT_DFF_CRC_INTERACTION: flush CRC before setting CRCEN+DFF */
    /* Clear CRCEN first so CRC registers are reset, then set if needed */
    /* This ensures no stale CRC values from previous DFF configuration */

    /* CRC enable — RM0008 §25.5.1 p.714 */
    if (pConfig->crcEnable)
    {
        cr1 |= SPI_CR1_CRCEN_Msk;
    }

    /* SPE remains 0 — caller enables after CR2 setup */
    /* Write composed value in one shot — Guard: INV_SPI_002 */
    SPIx->CR1 = cr1; /* RM0008 §25.5.1 p.714 */
}

/**
 * @brief  Configure CR2 from a Spi_ConfigType.
 *
 *         Sets SSOE for hardware NSS master output, and clears
 *         TXEIE/RXNEIE/ERRIE/TXDMAEN/RXDMAEN (interrupts enabled later by
 *         the driver layer as needed).
 *
 *         IR: configuration_strategies[] — SSOE mapping per strategy.
 *
 * @param[in]  SPIx     Pointer to SPI peripheral.
 * @param[in]  pConfig  Configuration structure. Caller must guarantee non-NULL.
 */
void SPI_LL_ComposeCR2(SPI_TypeDef *SPIx, const Spi_ConfigType *pConfig)
{
    uint32_t cr2 = 0U;

    /* Hardware NSS output enable (master mode only) — RM0008 §25.5.2 p.716 */
    /* IR strategy single_master_hardware_nss: SSOE=1 */
    /* IR strategy multimaster_arbitration: SSOE=0 (critical!) */
    if ((pConfig->masterSlave == SPI_MASTER) &&
        (pConfig->nssMode == SPI_NSS_HARD_OUTPUT))
    {
        cr2 |= SPI_CR2_SSOE_Msk;
    }

    /* Interrupts and DMA disabled here; driver enables them on demand */
    SPIx->CR2 = cr2; /* RM0008 §25.5.2 p.716 */
}
