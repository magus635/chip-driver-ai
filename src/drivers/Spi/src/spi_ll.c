/**
 * @file    spi_ll.c
 * @brief   STM32F103C8T6 SPI low-level driver implementation
 * @target  STM32F103C8T6
 * @ref     RM0008 Rev 21, Chapter 25 — Serial peripheral interface (SPI)
 *
 * Implements all functions declared in spi_ll.h. This is the only layer that
 * may access SPI and RCC registers directly. All higher layers (drv, api)
 * must call through this interface.
 *
 * Anti-pattern compliance (R8):
 *   W1: No |= on SR (W0C/RC_SEQ fields — see Spi_LL_ClearCrcError)
 *   W2: All bit operations use _Msk, not _Pos
 *   W3: This file does not include _drv.h or _api.h
 *   W4: Fixed-width types only (uint8_t, uint16_t, uint32_t)
 *   W5: No empty delay loops; all waits are condition-polling with timeout
 *   W6: NULL is used (via stddef.h in spi_ll.h)
 *   W10: Every register access annotated with RM0008 section
 */

#include "spi_ll.h"

/* ═══════════════════════════════════════════════════════════════════════════
 *  Clock and reset control
 * ═══════════════════════════════════════════════════════════════════════════ */

void Spi_LL_EnableClock(SPI_TypeDef *SPIx)
{
    if (SPIx == SPI1)
    {
        SPI_LL_RCC_APB2ENR |= SPI_LL_RCC_APB2ENR_SPI1EN_Msk; /* RM0008 §7.3.7 p.112 */
    }
    else
    {
        SPI_LL_RCC_APB1ENR |= SPI_LL_RCC_APB1ENR_SPI2EN_Msk; /* RM0008 §7.3.8 p.114 */
    }
}

void Spi_LL_DisableClock(SPI_TypeDef *SPIx)
{
    if (SPIx == SPI1)
    {
        SPI_LL_RCC_APB2ENR &= ~SPI_LL_RCC_APB2ENR_SPI1EN_Msk; /* RM0008 §7.3.7 p.112 */
    }
    else
    {
        SPI_LL_RCC_APB1ENR &= ~SPI_LL_RCC_APB1ENR_SPI2EN_Msk; /* RM0008 §7.3.8 p.114 */
    }
}

void Spi_LL_ResetPeripheral(SPI_TypeDef *SPIx)
{
    if (SPIx == SPI1)
    {
        SPI_LL_RCC_APB2RSTR |=  SPI_LL_RCC_APB2RSTR_SPI1RST_Msk; /* RM0008 §7.3.3 p.105 — assert reset */
        SPI_LL_RCC_APB2RSTR &= ~SPI_LL_RCC_APB2RSTR_SPI1RST_Msk; /* RM0008 §7.3.3 p.105 — release reset */
    }
    else
    {
        SPI_LL_RCC_APB1RSTR |=  SPI_LL_RCC_APB1RSTR_SPI2RST_Msk; /* RM0008 §7.3.4 p.107 — assert reset */
        SPI_LL_RCC_APB1RSTR &= ~SPI_LL_RCC_APB1RSTR_SPI2RST_Msk; /* RM0008 §7.3.4 p.107 — release reset */
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  CR1.SPE — enable / disable
 * ═══════════════════════════════════════════════════════════════════════════ */

void Spi_LL_Enable(SPI_TypeDef *SPIx)
{
    SPIx->CR1 |= SPI_CR1_SPE_Msk; /* RM0008 §25.5.1 p.715 — set SPE */
}

void Spi_LL_Disable(SPI_TypeDef *SPIx)
{
    /* Guard: INV_SPI_003 — caller must have called Spi_LL_WaitNotBusy() first */
    SPIx->CR1 &= (uint16_t)(~SPI_CR1_SPE_Msk); /* RM0008 §25.5.1 p.715 — clear SPE */
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  CR1 configuration (SPE=0 required — Guard: INV_SPI_002)
 * ═══════════════════════════════════════════════════════════════════════════ */

void Spi_LL_ConfigureCR1(SPI_TypeDef *SPIx, const Spi_ConfigType *cfg)
{
    uint16_t cr1 = 0U;

    /* Guard: INV_SPI_002 — disable SPI before modifying BR/CPOL/CPHA */
    SPIx->CR1 &= (uint16_t)(~SPI_CR1_SPE_Msk); /* RM0008 §25.5.1 p.715 */

    /* Master / slave mode */
    if (cfg->mode == SPI_MODE_MASTER)
    {
        cr1 |= SPI_CR1_MSTR_Msk; /* RM0008 §25.5.1 p.715 */
    }

    /* Baud rate prescaler — Guard: INV_SPI_002 */
    cr1 |= (uint16_t)((uint16_t)cfg->baud_prescaler << SPI_CR1_BR_Pos); /* RM0008 §25.5.1 p.715 */

    /* Clock polarity — Guard: INV_SPI_002 */
    if (cfg->cpol == SPI_CPOL_HIGH)
    {
        cr1 |= SPI_CR1_CPOL_Msk; /* RM0008 §25.5.1 p.715 */
    }

    /* Clock phase — Guard: INV_SPI_002 */
    if (cfg->cpha == SPI_CPHA_2EDGE)
    {
        cr1 |= SPI_CR1_CPHA_Msk; /* RM0008 §25.5.1 p.715 */
    }

    /* Data frame format — Guard: INV_SPI_004 (only valid when SPE=0) */
    if (cfg->data_frame == SPI_FRAME_16BIT)
    {
        cr1 |= SPI_CR1_DFF_Msk; /* RM0008 §25.5.1 p.715 */
    }

    /* NSS management */
    if (cfg->nss_manage == SPI_NSS_SOFT)
    {
        cr1 |= SPI_CR1_SSM_Msk; /* RM0008 §25.5.1 p.715 */

        /* Guard: INV_SPI_001 — MSTR=1 && SSM=1 => SSI must be 1
         * Without SSI=1, NSS is seen as low and MODF error triggers. */
        if (cfg->mode == SPI_MODE_MASTER)
        {
            cr1 |= SPI_CR1_SSI_Msk; /* RM0008 §25.5.1 p.715 */
        }
    }

    /* Bit order */
    if (cfg->bit_order == SPI_LSB_FIRST)
    {
        cr1 |= SPI_CR1_LSBFIRST_Msk; /* RM0008 §25.5.1 p.715 */
    }

    /* Atomic write — SPE remains 0; caller calls Spi_LL_Enable() after CR2 setup */
    SPIx->CR1 = cr1; /* RM0008 §25.5.1 — full CR1 write, SPE=0 */
}

void Spi_LL_EnableCRC(SPI_TypeDef *SPIx, uint16_t polynomial)
{
    /* Guard: INV_SPI_004 — must be called with SPE=0 */
    SPIx->CRCPR = polynomial;                   /* RM0008 §25.5.5 p.718 — set polynomial */
    SPIx->CR1  |= SPI_CR1_CRCEN_Msk;           /* RM0008 §25.5.1 p.715 — enable CRC */
}

void Spi_LL_DisableCRC(SPI_TypeDef *SPIx)
{
    /* Guard: INV_SPI_004 — must be called with SPE=0 */
    SPIx->CR1 &= (uint16_t)(~SPI_CR1_CRCEN_Msk); /* RM0008 §25.5.1 p.715 */
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  CR2 — interrupt and DMA control
 * ═══════════════════════════════════════════════════════════════════════════ */

void Spi_LL_EnableTxeIrq(SPI_TypeDef *SPIx)
{
    SPIx->CR2 |= SPI_CR2_TXEIE_Msk; /* RM0008 §25.5.2 p.716 */
}

void Spi_LL_DisableTxeIrq(SPI_TypeDef *SPIx)
{
    SPIx->CR2 &= (uint16_t)(~SPI_CR2_TXEIE_Msk); /* RM0008 §25.5.2 p.716 */
}

void Spi_LL_EnableRxneIrq(SPI_TypeDef *SPIx)
{
    SPIx->CR2 |= SPI_CR2_RXNEIE_Msk; /* RM0008 §25.5.2 p.716 */
}

void Spi_LL_DisableRxneIrq(SPI_TypeDef *SPIx)
{
    SPIx->CR2 &= (uint16_t)(~SPI_CR2_RXNEIE_Msk); /* RM0008 §25.5.2 p.716 */
}

void Spi_LL_EnableErrIrq(SPI_TypeDef *SPIx)
{
    SPIx->CR2 |= SPI_CR2_ERRIE_Msk; /* RM0008 §25.5.2 p.716 */
}

void Spi_LL_DisableErrIrq(SPI_TypeDef *SPIx)
{
    SPIx->CR2 &= (uint16_t)(~SPI_CR2_ERRIE_Msk); /* RM0008 §25.5.2 p.716 */
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  SR — status flag queries
 * ═══════════════════════════════════════════════════════════════════════════ */

uint8_t Spi_LL_IsTxEmpty(const SPI_TypeDef *SPIx)
{
    return (uint8_t)((SPIx->SR & SPI_SR_TXE_Msk) != 0U); /* RM0008 §25.5.3 p.717 — RO */
}

uint8_t Spi_LL_IsRxNotEmpty(const SPI_TypeDef *SPIx)
{
    return (uint8_t)((SPIx->SR & SPI_SR_RXNE_Msk) != 0U); /* RM0008 §25.5.3 p.717 — RO */
}

uint8_t Spi_LL_IsBusy(const SPI_TypeDef *SPIx)
{
    return (uint8_t)((SPIx->SR & SPI_SR_BSY_Msk) != 0U); /* RM0008 §25.5.3 p.718 — RO */
}

uint8_t Spi_LL_IsOverrun(const SPI_TypeDef *SPIx)
{
    return (uint8_t)((SPIx->SR & SPI_SR_OVR_Msk) != 0U); /* RM0008 §25.5.3 p.718 — RC_SEQ */
}

uint8_t Spi_LL_IsModeFault(const SPI_TypeDef *SPIx)
{
    return (uint8_t)((SPIx->SR & SPI_SR_MODF_Msk) != 0U); /* RM0008 §25.5.3 p.717 — RC_SEQ */
}

uint8_t Spi_LL_IsCrcError(const SPI_TypeDef *SPIx)
{
    return (uint8_t)((SPIx->SR & SPI_SR_CRCERR_Msk) != 0U); /* RM0008 §25.5.3 p.717 — W0C */
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  SR — error flag clearing (access-type specific)
 * ═══════════════════════════════════════════════════════════════════════════ */

void Spi_LL_ClearOverrun(SPI_TypeDef *SPIx)
{
    volatile uint16_t tmp;
    /* RC_SEQ clear sequence for OVR — RM0008 §25.3.10 p.693:
     * Step 1: read DR (this also clears RXNE) */
    tmp = SPIx->DR; /* RM0008 §25.5.4 — read DR */
    /* Step 2: read SR */
    tmp = SPIx->SR; /* RM0008 §25.5.3 — read SR clears OVR */
    (void)tmp;
}

void Spi_LL_ClearModeFault(SPI_TypeDef *SPIx)
{
    volatile uint16_t tmp;
    /* RC_SEQ clear sequence for MODF — RM0008 §25.3.10 p.693:
     * Step 1: read SR */
    tmp = SPIx->SR; /* RM0008 §25.5.3 — read SR */
    (void)tmp;
    /* Step 2: write CR1 (write current value to satisfy the sequence; SPE=0 already) */
    SPIx->CR1 = SPIx->CR1; /* RM0008 §25.5.1 — write CR1 clears MODF */
}

void Spi_LL_ClearCrcError(SPI_TypeDef *SPIx)
{
    /* W0C: write 0 to CRCERR (bit 4) — RM0008 §25.5.3 p.717
     * Read current SR and write back with CRCERR cleared.
     * Other SR bits: RO bits ignore writes; RC_SEQ bits are not write-triggered.
     * Do NOT use a constant 0x0000 write — that would affect reserved bits. */
    SPIx->SR = (uint16_t)(SPIx->SR & (uint16_t)(~SPI_SR_CRCERR_Msk)); /* RM0008 §25.5.3 — W0C */
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  DR — data transfer and polling waits
 * ═══════════════════════════════════════════════════════════════════════════ */

void Spi_LL_WriteDR(SPI_TypeDef *SPIx, uint16_t data)
{
    SPIx->DR = data; /* RM0008 §25.5.4 p.718 — write to TX buffer */
}

uint16_t Spi_LL_ReadDR(SPI_TypeDef *SPIx)
{
    return SPIx->DR; /* RM0008 §25.5.4 p.718 — read from RX buffer, clears RXNE */
}

Spi_ReturnType Spi_LL_WaitTxEmpty(SPI_TypeDef *SPIx, uint32_t timeout_ticks)
{
    uint32_t ticks = timeout_ticks;

    while ((SPIx->SR & SPI_SR_TXE_Msk) == 0U) /* RM0008 §25.5.3 p.717 — poll TXE */
    {
        if (ticks == 0U)
        {
            return SPI_ERR_TIMEOUT;
        }
        ticks--;
    }
    return SPI_OK;
}

Spi_ReturnType Spi_LL_WaitRxNotEmpty(SPI_TypeDef *SPIx, uint32_t timeout_ticks)
{
    uint32_t ticks = timeout_ticks;

    while ((SPIx->SR & SPI_SR_RXNE_Msk) == 0U) /* RM0008 §25.5.3 p.717 — poll RXNE */
    {
        if (ticks == 0U)
        {
            return SPI_ERR_TIMEOUT;
        }
        ticks--;
    }
    return SPI_OK;
}

Spi_ReturnType Spi_LL_WaitNotBusy(SPI_TypeDef *SPIx, uint32_t timeout_ticks)
{
    uint32_t ticks = timeout_ticks;

    while ((SPIx->SR & SPI_SR_BSY_Msk) != 0U) /* RM0008 §25.5.3 p.718 — poll BSY */
    {
        if (ticks == 0U)
        {
            return SPI_ERR_TIMEOUT;
        }
        ticks--;
    }
    return SPI_OK;
}
