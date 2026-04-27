/**
 * @file    dma_reg.h
 * @brief   STM32F103C8T6 DMA 寄存器映射层 (Layer 1)
 *
 * Source:  RM0008 Rev 21, §13.4
 *          ir/dma_ir_summary.json v3.0
 */

#ifndef DMA_REG_H
#define DMA_REG_H

#include <stdint.h>

/**
 * @brief DMA Channel registers structure
 */
typedef struct
{
    volatile uint32_t CCR;      /* 0x00: Configuration register */
    volatile uint32_t CNDTR;    /* 0x04: Number of data register */
    volatile uint32_t CPAR;     /* 0x08: Peripheral address register */
    volatile uint32_t CMAR;     /* 0x0C: Memory address register */
} DMA_Channel_TypeDef;

/**
 * @brief DMA Controller registers structure
 */
typedef struct
{
    volatile uint32_t ISR;      /* 0x00: Interrupt status register */
    volatile uint32_t IFCR;     /* 0x04: Interrupt flag clear register */
    DMA_Channel_TypeDef Channel[7]; /* 0x08 - 0x93: Channels 1..7 */
} DMA_TypeDef;

/* CCR Bit definitions */
#define DMA_CCR_MEM2MEM_Pos     (14U)
#define DMA_CCR_MEM2MEM_Msk     (0x1UL << DMA_CCR_MEM2MEM_Pos)
#define DMA_CCR_PL_Pos          (12U)
#define DMA_CCR_PL_Msk          (0x3UL << DMA_CCR_PL_Pos)
#define DMA_CCR_MSIZE_Pos       (10U)
#define DMA_CCR_MSIZE_Msk       (0x3UL << DMA_CCR_MSIZE_Pos)
#define DMA_CCR_PSIZE_Pos       (8U)
#define DMA_CCR_PSIZE_Msk       (0x3UL << DMA_CCR_PSIZE_Pos)
#define DMA_CCR_MINC_Pos        (7U)
#define DMA_CCR_MINC_Msk        (0x1UL << DMA_CCR_MINC_Pos)
#define DMA_CCR_PINC_Pos        (6U)
#define DMA_CCR_PINC_Msk        (0x1UL << DMA_CCR_PINC_Pos)
#define DMA_CCR_CIRC_Pos        (5U)
#define DMA_CCR_CIRC_Msk        (0x1UL << DMA_CCR_CIRC_Pos)
#define DMA_CCR_DIR_Pos         (4U)
#define DMA_CCR_DIR_Msk         (0x1UL << DMA_CCR_DIR_Pos)
#define DMA_CCR_TEIE_Pos        (3U)
#define DMA_CCR_TEIE_Msk        (0x1UL << DMA_CCR_TEIE_Pos)
#define DMA_CCR_HTIE_Pos        (2U)
#define DMA_CCR_HTIE_Msk        (0x1UL << DMA_CCR_HTIE_Pos)
#define DMA_CCR_TCIE_Pos        (1U)
#define DMA_CCR_TCIE_Msk        (0x1UL << DMA_CCR_TCIE_Pos)
#define DMA_CCR_EN_Pos          (0U)
#define DMA_CCR_EN_Msk          (0x1UL << DMA_CCR_EN_Pos)

/* ISR/IFCR Bit definitions (Channel 1 as example) */
#define DMA_ISR_GIF1_Pos        (0U)
#define DMA_ISR_GIF1_Msk        (0x1UL << DMA_ISR_GIF1_Pos)
#define DMA_ISR_TCIF1_Pos       (1U)
#define DMA_ISR_TCIF1_Msk       (0x1UL << DMA_ISR_TCIF1_Pos)
#define DMA_ISR_HTIF1_Pos       (2U)
#define DMA_ISR_HTIF1_Msk       (0x1UL << DMA_ISR_HTIF1_Pos)
#define DMA_ISR_TEIF1_Pos       (3U)
#define DMA_ISR_TEIF1_Msk       (0x1UL << DMA_ISR_TEIF1_Pos)

#define DMA_IFCR_CGIF1_Pos      (0U)
#define DMA_IFCR_CGIF1_Msk      (0x1UL << DMA_IFCR_CGIF1_Pos)
#define DMA_IFCR_CTCIF1_Pos     (1U)
#define DMA_IFCR_CTCIF1_Msk     (0x1UL << DMA_IFCR_CTCIF1_Pos)
#define DMA_IFCR_CHTIF1_Pos     (2U)
#define DMA_IFCR_CHTIF1_Msk     (0x1UL << DMA_IFCR_CHTIF1_Pos)
#define DMA_IFCR_CTEIF1_Pos     (3U)
#define DMA_IFCR_CTEIF1_Msk     (0x1UL << DMA_IFCR_CTEIF1_Pos)

/* Base addresses */
#define DMA1_BASE               (0x40020000UL)
#define DMA1                    ((DMA_TypeDef *)DMA1_BASE)

#endif /* DMA_REG_H */
