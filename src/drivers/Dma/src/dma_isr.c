/**
 * @file    dma_isr.c
 * @brief   STM32F103C8T6 DMA 中断服务函数实现
 */

#include "dma_api.h"

void DMA1_Channel1_IRQHandler(void) { Dma_IRQHandler(DMA_CH1); }
void DMA1_Channel2_IRQHandler(void) { Dma_IRQHandler(DMA_CH2); }
void DMA1_Channel3_IRQHandler(void) { Dma_IRQHandler(DMA_CH3); }
void DMA1_Channel4_IRQHandler(void) { Dma_IRQHandler(DMA_CH4); }
void DMA1_Channel5_IRQHandler(void) { Dma_IRQHandler(DMA_CH5); }
void DMA1_Channel6_IRQHandler(void) { Dma_IRQHandler(DMA_CH6); }
void DMA1_Channel7_IRQHandler(void) { Dma_IRQHandler(DMA_CH7); }
