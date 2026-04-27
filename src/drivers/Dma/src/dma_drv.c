/**
 * @file    dma_drv.c
 * @brief   STM32F103C8T6 DMA 驱动实现层
 *
 * Source:  ir/dma_ir_summary.json v3.0
 */

#include "dma_api.h"
#include "dma_ll.h"
#include <stddef.h>

/* Register base macro for internal use */
#define DMA_REG         DMA1

/*===========================================================================*/
/* Static Storage                                                            */
/*===========================================================================*/

static Dma_CallbackType g_dma_callbacks[DMA_CH_COUNT] = { NULL };
static void *g_dma_contexts[DMA_CH_COUNT] = { NULL };

/*===========================================================================*/
/* Public API Implementation                                                 */
/*===========================================================================*/

Dma_ReturnType Dma_InitChannel(const Dma_ConfigType *pConfig)
{
    if (pConfig == NULL || pConfig->channel >= DMA_CH_COUNT)
    {
        return DMA_PARAM;
    }

    Dma_Channel_e ch = pConfig->channel;

    /* 1. Register Callback */
    g_dma_callbacks[ch] = pConfig->pCallback;
    g_dma_contexts[ch] = pConfig->pContext;

    /* 2. Ensure channel is disabled (INV_DMA_001 Guard) */
    DMA_LL_DisableChannel(DMA_REG, ch);

    /* 3. Compose and write CCR (Implicitly enables TCIE if callback is provided) */
    uint32_t ccr = DMA_LL_ComposeCCR(pConfig);
    if (pConfig->pCallback != NULL)
    {
        ccr |= DMA_CCR_TCIE_Msk; /* Enable Transfer Complete Interrupt */
    }
    DMA_REG->Channel[ch].CCR = ccr;

    return DMA_OK;
}

Dma_ReturnType Dma_StartTransfer(Dma_Channel_e ch, uint32_t srcAddr, uint32_t dstAddr, uint16_t length)
{
    if (ch >= DMA_CH_COUNT || length == 0U)
    {
        return DMA_PARAM;
    }

    /* Guard: INV_DMA_001 — Must be disabled to change addresses/length */
    if (DMA_LL_IsChannelEnabled(DMA_REG, ch))
    {
        return DMA_BUSY;
    }

    /* 1. Set addresses and length */
    DMA_LL_SetPeripheralAddr(DMA_REG, ch, srcAddr);
    DMA_LL_SetMemoryAddr(DMA_REG, ch, dstAddr);
    DMA_LL_SetDataLength(DMA_REG, ch, length);

    /* 2. Clear flags before starting (W1C) */
    uint32_t clear_mask = (DMA_IFCR_CGIF1_Msk << (ch * 4U));
    DMA_LL_ClearFlags(DMA_REG, clear_mask);

    /* 3. Enable channel */
    DMA_LL_EnableChannel(DMA_REG, ch);

    return DMA_OK;
}

bool Dma_IsTransferComplete(Dma_Channel_e ch)
{
    if (ch >= DMA_CH_COUNT) { return false; }

    uint32_t status = DMA_LL_GetStatus(DMA_REG);
    uint32_t tc_mask = (DMA_ISR_TCIF1_Msk << (ch * 4U));

    return (status & tc_mask) != 0U;
}

/* Feature DMA-IRQ-HTTE: HT/TE query + flag clear API */
bool Dma_IsHalfTransfer(Dma_Channel_e ch)
{
    if (ch >= DMA_CH_COUNT) { return false; }
    uint32_t status = DMA_LL_GetStatus(DMA_REG);
    uint32_t ht_mask = (DMA_ISR_HTIF1_Msk << (ch * 4U));
    return (status & ht_mask) != 0U;
}

bool Dma_HasError(Dma_Channel_e ch)
{
    if (ch >= DMA_CH_COUNT) { return false; }
    uint32_t status = DMA_LL_GetStatus(DMA_REG);
    uint32_t te_mask = (DMA_ISR_TEIF1_Msk << (ch * 4U));
    return (status & te_mask) != 0U;
}

void Dma_ClearFlags(Dma_Channel_e ch)
{
    if (ch >= DMA_CH_COUNT) { return; }
    /* W1C: clear all 4 flag bits for this channel (CGIF, CTCIF, CHTIF, CTEIF) */
    uint32_t clear_mask = (DMA_IFCR_CGIF1_Msk << (ch * 4U));
    DMA_LL_ClearFlags(DMA_REG, clear_mask);
}

void Dma_StopTransfer(Dma_Channel_e ch)
{
    if (ch < DMA_CH_COUNT)
    {
        DMA_LL_DisableChannel(DMA_REG, ch);
    }
}

/* Feature DMA-ISR: callback registration */
Dma_ReturnType Dma_RegisterCallback(Dma_Channel_e ch, Dma_CallbackType cb, void *pContext)
{
    if (ch >= DMA_CH_COUNT) { return DMA_PARAM; }
    g_dma_callbacks[ch] = cb;
    g_dma_contexts[ch] = pContext;
    return DMA_OK;
}

/* Feature DMA-ISR: multi-event dispatcher (TC + HT + TE) */
void Dma_HandleInterrupt(Dma_Channel_e ch)
{
    if (ch >= DMA_CH_COUNT) { return; }

    uint32_t status   = DMA_LL_GetStatus(DMA_REG);
    uint32_t shift    = ch * 4U;
    uint32_t tc_mask  = (DMA_ISR_TCIF1_Msk  << shift);
    uint32_t ht_mask  = (DMA_ISR_HTIF1_Msk  << shift);
    uint32_t te_mask  = (DMA_ISR_TEIF1_Msk  << shift);
    uint32_t to_clear = 0U;

    if (status & tc_mask) { to_clear |= (DMA_IFCR_CTCIF1_Msk << shift); }
    if (status & ht_mask) { to_clear |= (DMA_IFCR_CHTIF1_Msk << shift); }
    if (status & te_mask) { to_clear |= (DMA_IFCR_CTEIF1_Msk << shift); }

    if (to_clear != 0U)
    {
        /* W1C: clear acknowledged events */
        DMA_LL_ClearFlags(DMA_REG, to_clear);

        if (g_dma_callbacks[ch] != NULL)
        {
            g_dma_callbacks[ch](g_dma_contexts[ch]);
        }
    }
}

/* Backward-compatible alias used by dma_isr.c */
void Dma_IRQHandler(Dma_Channel_e ch)
{
    Dma_HandleInterrupt(ch);
}
