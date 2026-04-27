/**
 * @file    dma_ll.h
 * @brief   STM32F103C8T6 DMA LL 层 (Layer 2 - Atomic Ops)
 *
 * Source:  ir/dma_ir_summary.json v3.0
 */

#ifndef DMA_LL_H
#define DMA_LL_H

#include "dma_reg.h"
#include "dma_types.h"
#include <stddef.h>

/*===========================================================================*/
/* Channel Management                                                        */
/*===========================================================================*/

static inline void DMA_LL_EnableChannel(DMA_TypeDef *DMAx, Dma_Channel_e ch)
{
    DMAx->Channel[ch].CCR |= DMA_CCR_EN_Msk;
}

static inline void DMA_LL_DisableChannel(DMA_TypeDef *DMAx, Dma_Channel_e ch)
{
    DMAx->Channel[ch].CCR &= ~DMA_CCR_EN_Msk;
}

static inline bool DMA_LL_IsChannelEnabled(const DMA_TypeDef *DMAx, Dma_Channel_e ch)
{
    return ((DMAx->Channel[ch].CCR & DMA_CCR_EN_Msk) != 0U);
}

/*===========================================================================*/
/* Configuration (Guarded by INV_DMA_001)                                    */
/*===========================================================================*/

static inline void DMA_LL_SetPeripheralAddr(DMA_TypeDef *DMAx, Dma_Channel_e ch, uint32_t addr)
{
    /* Guard: INV_DMA_001 — CPAR writable only when EN=0 */
    if (DMA_LL_IsChannelEnabled(DMAx, ch)) { return; }
    DMAx->Channel[ch].CPAR = addr;
}

static inline void DMA_LL_SetMemoryAddr(DMA_TypeDef *DMAx, Dma_Channel_e ch, uint32_t addr)
{
    /* Guard: INV_DMA_001 — CMAR writable only when EN=0 */
    if (DMA_LL_IsChannelEnabled(DMAx, ch)) { return; }
    DMAx->Channel[ch].CMAR = addr;
}

static inline void DMA_LL_SetDataLength(DMA_TypeDef *DMAx, Dma_Channel_e ch, uint16_t length)
{
    /* Guard: INV_DMA_001 — CNDTR writable only when EN=0 */
    if (DMA_LL_IsChannelEnabled(DMAx, ch)) { return; }
    DMAx->Channel[ch].CNDTR = (uint32_t)length;
}

/*===========================================================================*/
/* Flag Management (W1C)                                                     */
/*===========================================================================*/

static inline uint32_t DMA_LL_GetStatus(const DMA_TypeDef *DMAx)
{
    return DMAx->ISR;
}

static inline void DMA_LL_ClearFlags(DMA_TypeDef *DMAx, uint32_t flags)
{
    /* W1C: Write 1 to clear. Do NOT use |= */
    DMAx->IFCR = flags;
}

/*===========================================================================*/
/* Composition Helpers (Implemented in dma_ll.c per Guidelines §2)          */
/*===========================================================================*/

uint32_t DMA_LL_ComposeCCR(const Dma_ConfigType *pConfig);

#endif /* DMA_LL_H */
