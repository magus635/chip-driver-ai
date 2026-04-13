/**
 * @file    spi_drv.c
 * @brief   STM32F103C8T6 SPI driver — polling-mode state machine
 * @target  STM32F103C8T6
 * @ref     RM0008 Rev 21, Chapter 25 — Serial peripheral interface (SPI)
 *
 * Implements the public API declared in spi_api.h. Calls exclusively through
 * the LL layer (spi_ll.h); direct register access is forbidden here (R8.3).
 *
 * State machine:
 *   UNINIT ──(Init)──► IDLE ──(Transfer)──► BUSY_TX / BUSY_RX / BUSY_TX_RX
 *                       ▲                          │
 *                       │◄──────(done)────────────┘
 *                       │◄──────(AbortTransfer)────── ERROR
 *                  (DeInit)
 *                       │
 *                     UNINIT
 */

#include <stddef.h>
#include "spi_api.h"
#include "spi_ll.h"

/* ── Instance-to-register mapping ────────────────────────────────────────── */
static SPI_TypeDef * const g_spi_hw[SPI_CFG_INSTANCE_COUNT] =
{
    SPI1,  /* SPI_INSTANCE_1 — APB2 — RM0008 §3.3 */
    SPI2   /* SPI_INSTANCE_2 — APB1 — RM0008 §3.3 */
};

/* ── Per-instance driver state ───────────────────────────────────────────── */
static Spi_StatusType   g_spi_status[SPI_CFG_INSTANCE_COUNT];
static Spi_CallbackType g_spi_callback[SPI_CFG_INSTANCE_COUNT];

/* ── Internal helpers ────────────────────────────────────────────────────── */

/** Validate instance index and return the hardware pointer, or NULL on error. */
static SPI_TypeDef *prv_GetHw(Spi_InstanceType instance)
{
    if (instance >= (Spi_InstanceType)SPI_CFG_INSTANCE_COUNT)
    {
        return NULL;
    }
    return g_spi_hw[instance];
}

/** Convert ms to LL tick count. */
static uint32_t prv_MsToTicks(uint32_t ms)
{
    return ms * SPI_LL_TICKS_PER_MS;
}

/** Handle a transfer error: set state, increment counter, invoke callback. */
static void prv_SetError(Spi_InstanceType instance, Spi_ReturnType err)
{
    g_spi_status[instance].state = SPI_STATE_ERROR;
    g_spi_status[instance].error_count++;
    if (g_spi_callback[instance] != NULL)
    {
        g_spi_callback[instance](instance, err);
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Lifecycle
 * ═══════════════════════════════════════════════════════════════════════════ */

Spi_ReturnType Spi_Init(Spi_InstanceType instance, const Spi_ConfigType *cfg)
{
    SPI_TypeDef *SPIx;

    if (cfg == NULL)
    {
        return SPI_ERR_PARAM;
    }

    SPIx = prv_GetHw(instance);
    if (SPIx == NULL)
    {
        return SPI_ERR_PARAM;
    }

    /* Enable APB peripheral clock — RM0008 §7.3.7/§7.3.8 */
    Spi_LL_EnableClock(SPIx);

    /* Reset peripheral to known state — RM0008 §7.3.3/§7.3.4 */
    Spi_LL_ResetPeripheral(SPIx);

    /* Configure CR1 (SPE=0 enforced inside — Guard: INV_SPI_002) */
    Spi_LL_ConfigureCR1(SPIx, cfg);

    /* Configure CRC if requested — Guard: INV_SPI_004 (SPE is still 0) */
    if (cfg->crc_enable)
    {
        Spi_LL_EnableCRC(SPIx, cfg->crc_polynomial);
    }

    /* CR2: enable error interrupt; TX/RX interrupts enabled on-demand during transfer */
    Spi_LL_EnableErrIrq(SPIx);

    /* Enable peripheral — CR1.SPE=1 (after this: BR/CPOL/CPHA/DFF locked) */
    Spi_LL_Enable(SPIx);

    /* Initialise driver state */
    g_spi_status[instance].state       = SPI_STATE_IDLE;
    g_spi_status[instance].error_count = 0U;
    g_spi_status[instance].tx_count    = 0U;
    g_spi_status[instance].rx_count    = 0U;

    return SPI_OK;
}

Spi_ReturnType Spi_DeInit(Spi_InstanceType instance)
{
    SPI_TypeDef    *SPIx;
    Spi_ReturnType  ret;

    SPIx = prv_GetHw(instance);
    if (SPIx == NULL)
    {
        return SPI_ERR_PARAM;
    }

    /* Disable all CR2 interrupts before touching the peripheral */
    Spi_LL_DisableTxeIrq(SPIx);
    Spi_LL_DisableRxneIrq(SPIx);
    Spi_LL_DisableErrIrq(SPIx);

    /* Guard: INV_SPI_003 — wait for BSY=0 before clearing SPE */
    ret = Spi_LL_WaitNotBusy(SPIx, prv_MsToTicks(SPI_CFG_TX_TIMEOUT_MS));
    if (ret != SPI_OK)
    {
        /* BSY stuck — force disable anyway and report timeout */
        Spi_LL_Disable(SPIx);
        Spi_LL_DisableClock(SPIx);
        g_spi_status[instance].state = SPI_STATE_UNINIT;
        return SPI_ERR_TIMEOUT;
    }

    Spi_LL_Disable(SPIx);
    Spi_LL_DisableClock(SPIx);

    g_spi_status[instance].state    = SPI_STATE_UNINIT;
    g_spi_callback[instance]        = NULL;

    return SPI_OK;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Polling-mode data transfer
 * ═══════════════════════════════════════════════════════════════════════════ */

Spi_ReturnType Spi_Transmit(Spi_InstanceType  instance,
                             const uint8_t    *p_data,
                             uint16_t          size,
                             uint32_t          timeout_ms)
{
    SPI_TypeDef    *SPIx;
    Spi_ReturnType  ret;
    uint16_t        idx;
    uint32_t        ticks;

    if (p_data == NULL || size == 0U)
    {
        return SPI_ERR_PARAM;
    }

    SPIx = prv_GetHw(instance);
    if (SPIx == NULL)
    {
        return SPI_ERR_PARAM;
    }

    if (g_spi_status[instance].state == SPI_STATE_UNINIT)
    {
        return SPI_ERR_NOT_INIT;
    }
    if (g_spi_status[instance].state != SPI_STATE_IDLE)
    {
        return SPI_ERR_BUSY;
    }

    g_spi_status[instance].state = SPI_STATE_BUSY_TX;
    ticks = prv_MsToTicks(timeout_ms);

    for (idx = 0U; idx < size; idx++)
    {
        /* Wait for TX buffer empty */
        ret = Spi_LL_WaitTxEmpty(SPIx, ticks);
        if (ret != SPI_OK)
        {
            prv_SetError(instance, SPI_ERR_TIMEOUT);
            return SPI_ERR_TIMEOUT;
        }

        /* Write data frame — 8-bit DFF assumed (mask upper byte if 8-bit) */
        Spi_LL_WriteDR(SPIx, (uint16_t)p_data[idx]);

        g_spi_status[instance].tx_count++;
    }

    /* Wait for last frame to complete (BSY=0) before returning */
    ret = Spi_LL_WaitNotBusy(SPIx, ticks);
    if (ret != SPI_OK)
    {
        prv_SetError(instance, SPI_ERR_TIMEOUT);
        return SPI_ERR_TIMEOUT;
    }

    /* Drain RX buffer to prevent overrun on next transfer */
    if (Spi_LL_IsRxNotEmpty(SPIx))
    {
        (void)Spi_LL_ReadDR(SPIx);
    }

    g_spi_status[instance].state = SPI_STATE_IDLE;
    return SPI_OK;
}

Spi_ReturnType Spi_Receive(Spi_InstanceType  instance,
                            uint8_t          *p_buf,
                            uint16_t          size,
                            uint32_t          timeout_ms)
{
    SPI_TypeDef    *SPIx;
    Spi_ReturnType  ret;
    uint16_t        idx;
    uint32_t        ticks;

    if (p_buf == NULL || size == 0U)
    {
        return SPI_ERR_PARAM;
    }

    SPIx = prv_GetHw(instance);
    if (SPIx == NULL)
    {
        return SPI_ERR_PARAM;
    }

    if (g_spi_status[instance].state == SPI_STATE_UNINIT)
    {
        return SPI_ERR_NOT_INIT;
    }
    if (g_spi_status[instance].state != SPI_STATE_IDLE)
    {
        return SPI_ERR_BUSY;
    }

    g_spi_status[instance].state = SPI_STATE_BUSY_RX;
    ticks = prv_MsToTicks(timeout_ms);

    for (idx = 0U; idx < size; idx++)
    {
        /* Send dummy byte to generate clock */
        ret = Spi_LL_WaitTxEmpty(SPIx, ticks);
        if (ret != SPI_OK)
        {
            prv_SetError(instance, SPI_ERR_TIMEOUT);
            return SPI_ERR_TIMEOUT;
        }
        Spi_LL_WriteDR(SPIx, 0xFFU); /* dummy TX — RM0008 §25.3 */

        /* Wait for received data */
        ret = Spi_LL_WaitRxNotEmpty(SPIx, ticks);
        if (ret != SPI_OK)
        {
            prv_SetError(instance, SPI_ERR_TIMEOUT);
            return SPI_ERR_TIMEOUT;
        }

        /* Check overrun before reading */
        if (Spi_LL_IsOverrun(SPIx))
        {
            Spi_LL_ClearOverrun(SPIx);
            prv_SetError(instance, SPI_ERR_OVERRUN);
            return SPI_ERR_OVERRUN;
        }

        p_buf[idx] = (uint8_t)Spi_LL_ReadDR(SPIx);
        g_spi_status[instance].rx_count++;
    }

    /* Wait for BSY=0 — transfer complete */
    ret = Spi_LL_WaitNotBusy(SPIx, ticks);
    if (ret != SPI_OK)
    {
        prv_SetError(instance, SPI_ERR_TIMEOUT);
        return SPI_ERR_TIMEOUT;
    }

    g_spi_status[instance].state = SPI_STATE_IDLE;
    return SPI_OK;
}

Spi_ReturnType Spi_TransmitReceive(Spi_InstanceType  instance,
                                    const uint8_t    *p_tx_data,
                                    uint8_t          *p_rx_buf,
                                    uint16_t          size,
                                    uint32_t          timeout_ms)
{
    SPI_TypeDef    *SPIx;
    Spi_ReturnType  ret;
    uint16_t        idx;
    uint32_t        ticks;

    if (p_tx_data == NULL || p_rx_buf == NULL || size == 0U)
    {
        return SPI_ERR_PARAM;
    }

    SPIx = prv_GetHw(instance);
    if (SPIx == NULL)
    {
        return SPI_ERR_PARAM;
    }

    if (g_spi_status[instance].state == SPI_STATE_UNINIT)
    {
        return SPI_ERR_NOT_INIT;
    }
    if (g_spi_status[instance].state != SPI_STATE_IDLE)
    {
        return SPI_ERR_BUSY;
    }

    g_spi_status[instance].state = SPI_STATE_BUSY_TX_RX;
    ticks = prv_MsToTicks(timeout_ms);

    for (idx = 0U; idx < size; idx++)
    {
        /* Wait for TX buffer empty, then write */
        ret = Spi_LL_WaitTxEmpty(SPIx, ticks);
        if (ret != SPI_OK)
        {
            prv_SetError(instance, SPI_ERR_TIMEOUT);
            return SPI_ERR_TIMEOUT;
        }
        Spi_LL_WriteDR(SPIx, (uint16_t)p_tx_data[idx]);
        g_spi_status[instance].tx_count++;

        /* Wait for received data */
        ret = Spi_LL_WaitRxNotEmpty(SPIx, ticks);
        if (ret != SPI_OK)
        {
            prv_SetError(instance, SPI_ERR_TIMEOUT);
            return SPI_ERR_TIMEOUT;
        }

        if (Spi_LL_IsOverrun(SPIx))
        {
            Spi_LL_ClearOverrun(SPIx);
            prv_SetError(instance, SPI_ERR_OVERRUN);
            return SPI_ERR_OVERRUN;
        }

        p_rx_buf[idx] = (uint8_t)Spi_LL_ReadDR(SPIx);
        g_spi_status[instance].rx_count++;
    }

    /* Wait for BSY=0 — all frames shifted out */
    ret = Spi_LL_WaitNotBusy(SPIx, ticks);
    if (ret != SPI_OK)
    {
        prv_SetError(instance, SPI_ERR_TIMEOUT);
        return SPI_ERR_TIMEOUT;
    }

    g_spi_status[instance].state = SPI_STATE_IDLE;
    return SPI_OK;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Status and control
 * ═══════════════════════════════════════════════════════════════════════════ */

Spi_ReturnType Spi_GetStatus(Spi_InstanceType instance, Spi_StatusType *p_status)
{
    if (p_status == NULL || instance >= (Spi_InstanceType)SPI_CFG_INSTANCE_COUNT)
    {
        return SPI_ERR_PARAM;
    }
    *p_status = g_spi_status[instance];
    return SPI_OK;
}

Spi_ReturnType Spi_AbortTransfer(Spi_InstanceType instance)
{
    SPI_TypeDef *SPIx;

    if (instance >= (Spi_InstanceType)SPI_CFG_INSTANCE_COUNT)
    {
        return SPI_ERR_PARAM;
    }

    SPIx = g_spi_hw[instance];

    /* Disable data interrupts to stop in-progress interrupt-mode transfer */
    Spi_LL_DisableTxeIrq(SPIx);
    Spi_LL_DisableRxneIrq(SPIx);

    /* Clear any pending error flags */
    if (Spi_LL_IsOverrun(SPIx))
    {
        Spi_LL_ClearOverrun(SPIx);
    }
    if (Spi_LL_IsModeFault(SPIx))
    {
        Spi_LL_ClearModeFault(SPIx);
    }
    if (Spi_LL_IsCrcError(SPIx))
    {
        Spi_LL_ClearCrcError(SPIx);
    }

    g_spi_status[instance].state = SPI_STATE_IDLE;
    return SPI_OK;
}

Spi_ReturnType Spi_RegisterCallback(Spi_InstanceType instance, Spi_CallbackType callback)
{
    if (instance >= (Spi_InstanceType)SPI_CFG_INSTANCE_COUNT)
    {
        return SPI_ERR_PARAM;
    }
    g_spi_callback[instance] = callback;
    return SPI_OK;
}
