/**
 * @file    dma_ll.c
 * @brief   STM32F103C8T6 DMA LL 层非 inline 实现 (Composition)
 *
 * Source:  ir/dma_ir_summary.json v3.0
 */

#include "dma_ll.h"

/**
 * @brief  Compose CCR value from configuration struct.
 * @param  pConfig Pointer to DMA configuration.
 * @return 32-bit CCR register value.
 */
uint32_t DMA_LL_ComposeCCR(const Dma_ConfigType *pConfig)
{
    uint32_t ccr = 0U;

    /* Direction */
    if (pConfig->direction == DMA_DIR_M2P)
    {
        ccr |= DMA_CCR_DIR_Msk;
    }

    /* Sizes */
    ccr |= ((uint32_t)pConfig->srcSize << DMA_CCR_PSIZE_Pos) & DMA_CCR_PSIZE_Msk;
    ccr |= ((uint32_t)pConfig->dstSize << DMA_CCR_MSIZE_Pos) & DMA_CCR_MSIZE_Msk;

    /* Increments */
    if (pConfig->srcInc) { ccr |= DMA_CCR_PINC_Msk; }
    if (pConfig->dstInc) { ccr |= DMA_CCR_MINC_Msk; }

    /* Mode */
    if (pConfig->circular) { ccr |= DMA_CCR_CIRC_Msk; }
    if (pConfig->mem2mem)  { ccr |= DMA_CCR_MEM2MEM_Msk; }

    /* Priority */
    ccr |= ((uint32_t)pConfig->priority << DMA_CCR_PL_Pos) & DMA_CCR_PL_Msk;

    return ccr;
}
