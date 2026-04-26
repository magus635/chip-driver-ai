/**
 * @file    dma_isr.c
 * @brief   STM32F103C8T6 DMA 中断服务函数入口 (Layer 3)
 */

#include "dma_reg.h"
#include "dma_ll.h"

/**
 * @brief  Shared DMA1 Channel 1 ISR
 */
void DMA1_Channel1_IRQHandler(void)
{
    /* Clear global interrupt flag for Channel 1 */
    DMA_LL_ClearFlags(DMA1, DMA_IFCR_CGIF1_Msk);
}

/* ... Additional IRQHandlers for Channels 2-7 ... */
