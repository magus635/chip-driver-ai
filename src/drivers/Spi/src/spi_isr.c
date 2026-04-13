/**
 * @file    spi_isr.c
 * @brief   STM32F103C8T6 SPI interrupt service routine entry points
 * @target  STM32F103C8T6
 * @ref     RM0008 Rev 21, §10.1.2 Table 63 — Interrupt vectors (pp.190-197)
 *          RM0008 Rev 21, §25.3   — SPI functional description
 *
 * Defines SPI1_IRQHandler (IRQ 35) and SPI2_IRQHandler (IRQ 36).
 * Each ISR reads SR once, processes error flags, then clears them using the
 * correct access-type sequence (RC_SEQ for OVR/MODF, W0C for CRCERR).
 *
 * Error handling strategy (polling-mode driver):
 *   - OVR  : clear via RC_SEQ, increment error counter via Spi_GetStatus/AbortTransfer
 *   - MODF : clear via RC_SEQ, re-enable SPE (MODF clears MSTR, recovery needed)
 *   - CRCERR: clear via W0C; notify callback with SPI_ERR_CRCERR
 *
 * Note: In the current polling-mode configuration (SPI_CFG_DMA_ENABLE = 0),
 * only the error interrupt (ERRIE) is enabled. TXE/RXNE interrupts are masked.
 * This ISR will expand to handle interrupt-driven transfers when ERRIE/TXEIE/
 * RXNEIE are enabled by higher-level transfer logic.
 */

#include <stddef.h>
#include "spi_ll.h"
#include "spi_api.h"

/* ── Forward declarations for shared ISR logic ─────────────────────────── */
static void prv_SpiIrqHandler(Spi_InstanceType instance, SPI_TypeDef *SPIx);

/* ═══════════════════════════════════════════════════════════════════════════
 *  ISR entry points — names must match the vector table in the startup file
 *  IRQ 35: SPI1 — RM0008 §10.1.2 Table 63 p.196
 *  IRQ 36: SPI2 — RM0008 §10.1.2 Table 63 p.196
 * ═══════════════════════════════════════════════════════════════════════════ */

void SPI1_IRQHandler(void)
{
    prv_SpiIrqHandler(SPI_INSTANCE_1, SPI1);
}

void SPI2_IRQHandler(void)
{
    prv_SpiIrqHandler(SPI_INSTANCE_2, SPI2);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Shared IRQ handler logic
 * ═══════════════════════════════════════════════════════════════════════════ */

static void prv_SpiIrqHandler(Spi_InstanceType instance, SPI_TypeDef *SPIx)
{
    Spi_StatusType  status;
    Spi_ReturnType  event = SPI_OK;

    /* ── OVR: overrun — RC_SEQ clear — RM0008 §25.3.10 p.693 ─────────────── */
    if (Spi_LL_IsOverrun(SPIx))
    {
        Spi_LL_ClearOverrun(SPIx); /* read DR → read SR */
        event = SPI_ERR_OVERRUN;
    }

    /* ── MODF: mode fault — RC_SEQ clear — RM0008 §25.3.10 p.693 ────────── */
    if (Spi_LL_IsModeFault(SPIx))
    {
        Spi_LL_ClearModeFault(SPIx); /* read SR → write CR1 */
        /* MODF automatically clears MSTR; re-enable SPI after recovery */
        Spi_LL_Enable(SPIx); /* RM0008 §25.5.1 — restore SPE after MODF */
        event = SPI_ERR_MODF;
    }

    /* ── CRCERR: CRC mismatch — W0C clear — RM0008 §25.5.3 p.717 ─────────── */
    if (Spi_LL_IsCrcError(SPIx))
    {
        Spi_LL_ClearCrcError(SPIx); /* write 0 to CRCERR */
        event = SPI_ERR_CRCERR;
    }

    /* ── Notify driver layer of error (driver will update state machine) ── */
    if (event != SPI_OK)
    {
        /* Retrieve current status to confirm instance is initialized */
        if (Spi_GetStatus(instance, &status) == SPI_OK)
        {
            (void)Spi_AbortTransfer(instance);
        }
    }
}
