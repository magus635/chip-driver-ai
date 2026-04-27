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

/* ---- Feature DMA-ISR: callback registration + multi-event dispatcher ---- */

/**
 * @brief  Register a callback for a channel (replaces InitChannel-time binding).
 * @param  ch        Channel index.
 * @param  cb        Callback (NULL to clear).
 * @param  pContext  User context passed to callback.
 */
Dma_ReturnType Dma_RegisterCallback(Dma_Channel_e ch, Dma_CallbackType cb, void *pContext);

/**
 * @brief  Channel-level interrupt dispatcher (called by *_IRQHandler entries).
 *         Reads ISR, invokes callback for TC/HT/TE events, and W1C-clears flags.
 */
void Dma_HandleInterrupt(Dma_Channel_e ch);

/* ---- Feature DMA-IRQ-HTTE: HT/TE flag query + W1C clear ---- */

/**
 * @brief  Check half-transfer flag for a channel.
 * IR-ref: registers.ISR.HTIF1 (RM0008 §13.4.1)
 */
bool Dma_IsHalfTransfer(Dma_Channel_e ch);

/**
 * @brief  Check transfer-error flag for a channel.
 * IR-ref: registers.ISR.TEIF1 (RM0008 §13.4.1)
 */
bool Dma_HasError(Dma_Channel_e ch);

/**
 * @brief  Clear HT/TE/TC/GIF flags via W1C on IFCR.
 * IR-ref: registers.IFCR.{CHTIFx,CTEIFx,CTCIFx,CGIFx} (RM0008 §13.4.2)
 */
void Dma_ClearFlags(Dma_Channel_e ch);

#endif /* DMA_API_H */
