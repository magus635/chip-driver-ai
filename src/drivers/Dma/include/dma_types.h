/**
 * @file    dma_types.h
 * @brief   STM32F103C8T6 DMA 类型定义层
 *
 * Source:  ir/dma_ir_summary.json v3.0
 */

#ifndef DMA_TYPES_H
#define DMA_TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    DMA_OK      = 0,
    DMA_ERR     = 1,
    DMA_BUSY    = 2,
    DMA_PARAM   = 3,
    DMA_TIMEOUT = 4
} Dma_ReturnType;

typedef enum
{
    DMA_CH1 = 0,
    DMA_CH2 = 1,
    DMA_CH3 = 2,
    DMA_CH4 = 3,
    DMA_CH5 = 4,
    DMA_CH6 = 5,
    DMA_CH7 = 6,
    DMA_CH_COUNT = 7
} Dma_Channel_e;

typedef enum { DMA_DIR_P2M = 0U, DMA_DIR_M2P = 1U } Dma_Direction_e;
typedef enum { DMA_SIZE_8BIT = 0U, DMA_SIZE_16BIT = 1U, DMA_SIZE_32BIT = 2U } Dma_DataSize_e;
typedef enum { DMA_PRIO_LOW = 0U, DMA_PRIO_MEDIUM = 1U, DMA_PRIO_HIGH = 2U, DMA_PRIO_VERYHIGH = 3U } Dma_Priority_e;

typedef struct
{
    Dma_Channel_e    channel;
    Dma_Direction_e  direction;
    Dma_DataSize_e   srcSize;
    Dma_DataSize_e   dstSize;
    bool             srcInc;
    bool             dstInc;
    bool             circular;
    Dma_Priority_e   priority;
    bool             mem2mem;
} Dma_ConfigType;

#endif /* DMA_TYPES_H */
