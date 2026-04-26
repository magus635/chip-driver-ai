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
/* Public API Implementation                                                 */
/*===========================================================================*/

Dma_ReturnType Dma_InitChannel(const Dma_ConfigType *pConfig)
{
    if (pConfig == NULL || pConfig->channel >= DMA_CH_COUNT)
    {
        return DMA_PARAM;
    }

    Dma_Channel_e ch = pConfig->channel;

    /* 1. Enable DMA Clock (In real project, via RCC_API_EnableDMA1) */
    /* RCC_API_EnableDMA1(); */

    /* 2. Ensure channel is disabled (INV_DMA_001 Guard) */
    DMA_LL_DisableChannel(DMA_REG, ch);

    /* 3. Compose and write CCR */
    uint32_t ccr = DMA_LL_ComposeCCR(pConfig);
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

void Dma_StopTransfer(Dma_Channel_e ch)
{
    if (ch < DMA_CH_COUNT)
    {
        DMA_LL_DisableChannel(DMA_REG, ch);
    }
}
