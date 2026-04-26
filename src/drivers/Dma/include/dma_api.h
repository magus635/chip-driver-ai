/**
 * @file    dma_api.h
 * @brief   STM32F103C8T6 DMA 公共接口层
 */

#ifndef DMA_API_H
#define DMA_API_H

#include "dma_types.h"

/**
 * @brief  Initialize and configure a DMA channel.
 * @param  pConfig Pointer to configuration.
 * @return DMA_OK on success.
 */
Dma_ReturnType Dma_InitChannel(const Dma_ConfigType *pConfig);

/**
 * @brief  Start a DMA transfer.
 * @param  ch         Channel index.
 * @param  srcAddr    Source address.
 * @param  dstAddr    Destination address.
 * @param  length     Number of data units.
 * @return DMA_OK if started.
 */
Dma_ReturnType Dma_StartTransfer(Dma_Channel_e ch, uint32_t srcAddr, uint32_t dstAddr, uint16_t length);

/**
 * @brief  Check if a DMA transfer is complete.
 * @param  ch         Channel index.
 * @return true if complete.
 */
bool Dma_IsTransferComplete(Dma_Channel_e ch);

/**
 * @brief  Stop a DMA transfer.
 * @param  ch         Channel index.
 */
void Dma_StopTransfer(Dma_Channel_e ch);

/**
 * @brief  Internal ISR handler for DMA channels.
 */
void Dma_IRQHandler(Dma_Channel_e ch);

#endif /* DMA_API_H */
