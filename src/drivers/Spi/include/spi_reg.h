/**
 * @file    spi_reg.h
 * @brief   STM32F103C8T6 SPI 寄存器映射层 (Register Mapping)
 *
 * @note    将外设物理地址、寄存器偏移、位域使用结构体和宏定义完整映射。
 *          绝对不允许包含任何逻辑代码或 C 函数实现。
 *          位段操作使用 _Pos / _Msk 宏对。
 *
 * Source:  RM0008 Rev 21, §25.5 SPI and I2S registers (pp.714-722)
 *          ir/spi_ir_summary.json v3.0
 */

#ifndef SPI_REG_H
#define SPI_REG_H

#include <stdint.h>

/*===========================================================================*/
/* CMSIS access macros (if not already defined)                               */
/*===========================================================================*/

#ifndef __IO
#define __IO    volatile
#endif

#ifndef __I
#define __I     volatile const
#endif

#ifndef __O
#define __O     volatile
#endif

/*===========================================================================*/
/* SPI register structure                                                     */
/* Source: ir/spi_ir_summary.json registers[]                                */
/*===========================================================================*/

typedef struct
{
    __IO uint16_t CR1;       /* 0x00 — SPI control register 1          (RW)  */
         uint16_t RESERVED0;
    __IO uint16_t CR2;       /* 0x04 — SPI control register 2          (RW)  */
         uint16_t RESERVED1;
    __IO uint16_t SR;        /* 0x08 — SPI status register             (mixed) */
         uint16_t RESERVED2;
    __IO uint16_t DR;        /* 0x0C — SPI data register               (RW)  */
         uint16_t RESERVED3;
    __IO uint16_t CRCPR;     /* 0x10 — SPI CRC polynomial register     (RW)  */
         uint16_t RESERVED4;
    __I  uint16_t RXCRCR;    /* 0x14 — SPI RX CRC register             (RO)  */
         uint16_t RESERVED5;
    __I  uint16_t TXCRCR;    /* 0x18 — SPI TX CRC register             (RO)  */
         uint16_t RESERVED6;
    __IO uint16_t I2SCFGR;   /* 0x1C — SPI_I2S configuration register  (RW)  */
         uint16_t RESERVED7;
    __IO uint16_t I2SPR;     /* 0x20 — SPI_I2S prescaler register      (RW)  */
         uint16_t RESERVED8;
} SPI_TypeDef;

/*===========================================================================*/
/* CR1 — SPI control register 1 (offset 0x00)                                */
/* Source: ir/spi_ir_summary.json registers[0]                               */
/* Reset: 0x0000 — RM0008 §25.5.1 p.714                                     */
/*===========================================================================*/

#define SPI_CR1_BIDIMODE_Pos    (15U)
#define SPI_CR1_BIDIMODE_Msk    (0x1U << SPI_CR1_BIDIMODE_Pos)    /* 0x8000 */

#define SPI_CR1_BIDIOE_Pos      (14U)
#define SPI_CR1_BIDIOE_Msk      (0x1U << SPI_CR1_BIDIOE_Pos)      /* 0x4000 */

#define SPI_CR1_CRCEN_Pos       (13U)
#define SPI_CR1_CRCEN_Msk       (0x1U << SPI_CR1_CRCEN_Pos)       /* 0x2000 */

#define SPI_CR1_CRCNEXT_Pos     (12U)
#define SPI_CR1_CRCNEXT_Msk     (0x1U << SPI_CR1_CRCNEXT_Pos)     /* 0x1000 */

#define SPI_CR1_DFF_Pos         (11U)
#define SPI_CR1_DFF_Msk         (0x1U << SPI_CR1_DFF_Pos)         /* 0x0800 */

#define SPI_CR1_RXONLY_Pos      (10U)
#define SPI_CR1_RXONLY_Msk      (0x1U << SPI_CR1_RXONLY_Pos)      /* 0x0400 */

#define SPI_CR1_SSM_Pos         (9U)
#define SPI_CR1_SSM_Msk         (0x1U << SPI_CR1_SSM_Pos)         /* 0x0200 */

#define SPI_CR1_SSI_Pos         (8U)
#define SPI_CR1_SSI_Msk         (0x1U << SPI_CR1_SSI_Pos)         /* 0x0100 */

#define SPI_CR1_LSBFIRST_Pos    (7U)
#define SPI_CR1_LSBFIRST_Msk    (0x1U << SPI_CR1_LSBFIRST_Pos)    /* 0x0080 */

#define SPI_CR1_SPE_Pos         (6U)
#define SPI_CR1_SPE_Msk         (0x1U << SPI_CR1_SPE_Pos)         /* 0x0040 */

#define SPI_CR1_BR_Pos          (3U)
#define SPI_CR1_BR_Msk          (0x7U << SPI_CR1_BR_Pos)          /* 0x0038 */

#define SPI_CR1_MSTR_Pos        (2U)
#define SPI_CR1_MSTR_Msk        (0x1U << SPI_CR1_MSTR_Pos)        /* 0x0004 */

#define SPI_CR1_CPOL_Pos        (1U)
#define SPI_CR1_CPOL_Msk        (0x1U << SPI_CR1_CPOL_Pos)        /* 0x0002 */

#define SPI_CR1_CPHA_Pos        (0U)
#define SPI_CR1_CPHA_Msk        (0x1U << SPI_CR1_CPHA_Pos)        /* 0x0001 */

/*===========================================================================*/
/* CR2 — SPI control register 2 (offset 0x04)                                */
/* Source: ir/spi_ir_summary.json registers[1]                               */
/* Reset: 0x0000 — RM0008 §25.5.2 p.716                                     */
/*===========================================================================*/

#define SPI_CR2_TXEIE_Pos       (7U)
#define SPI_CR2_TXEIE_Msk       (0x1U << SPI_CR2_TXEIE_Pos)       /* 0x0080 */

#define SPI_CR2_RXNEIE_Pos      (6U)
#define SPI_CR2_RXNEIE_Msk      (0x1U << SPI_CR2_RXNEIE_Pos)      /* 0x0040 */

#define SPI_CR2_ERRIE_Pos       (5U)
#define SPI_CR2_ERRIE_Msk       (0x1U << SPI_CR2_ERRIE_Pos)       /* 0x0020 */

#define SPI_CR2_SSOE_Pos        (2U)
#define SPI_CR2_SSOE_Msk        (0x1U << SPI_CR2_SSOE_Pos)        /* 0x0004 */

#define SPI_CR2_TXDMAEN_Pos     (1U)
#define SPI_CR2_TXDMAEN_Msk     (0x1U << SPI_CR2_TXDMAEN_Pos)     /* 0x0002 */

#define SPI_CR2_RXDMAEN_Pos     (0U)
#define SPI_CR2_RXDMAEN_Msk     (0x1U << SPI_CR2_RXDMAEN_Pos)     /* 0x0001 */

/*===========================================================================*/
/* SR — SPI status register (offset 0x08)                                    */
/* Source: ir/spi_ir_summary.json registers[2]                               */
/* Reset: 0x0002 — RM0008 §25.5.3 p.717                                     */
/*===========================================================================*/

#define SPI_SR_BSY_Pos          (7U)
#define SPI_SR_BSY_Msk          (0x1U << SPI_SR_BSY_Pos)          /* 0x0080 */

#define SPI_SR_OVR_Pos          (6U)
#define SPI_SR_OVR_Msk          (0x1U << SPI_SR_OVR_Pos)          /* 0x0040 */

#define SPI_SR_MODF_Pos         (5U)
#define SPI_SR_MODF_Msk         (0x1U << SPI_SR_MODF_Pos)         /* 0x0020 */

#define SPI_SR_CRCERR_Pos       (4U)
#define SPI_SR_CRCERR_Msk       (0x1U << SPI_SR_CRCERR_Pos)      /* 0x0010 */

#define SPI_SR_UDR_Pos          (3U)
#define SPI_SR_UDR_Msk          (0x1U << SPI_SR_UDR_Pos)          /* 0x0008 */

#define SPI_SR_CHSIDE_Pos       (2U)
#define SPI_SR_CHSIDE_Msk       (0x1U << SPI_SR_CHSIDE_Pos)      /* 0x0004 */

#define SPI_SR_TXE_Pos          (1U)
#define SPI_SR_TXE_Msk          (0x1U << SPI_SR_TXE_Pos)          /* 0x0002 */

#define SPI_SR_RXNE_Pos         (0U)
#define SPI_SR_RXNE_Msk         (0x1U << SPI_SR_RXNE_Pos)         /* 0x0001 */

/*===========================================================================*/
/* DR — SPI data register (offset 0x0C)                                      */
/* Source: ir/spi_ir_summary.json registers[3]                               */
/* Reset: 0x0000 — RM0008 §25.5.4 p.718                                     */
/*===========================================================================*/

#define SPI_DR_DR_Pos           (0U)
#define SPI_DR_DR_Msk           (0xFFFFU << SPI_DR_DR_Pos)        /* 0xFFFF */

/*===========================================================================*/
/* CRCPR — SPI CRC polynomial register (offset 0x10)                         */
/* Source: ir/spi_ir_summary.json registers[4]                               */
/* Reset: 0x0007 — RM0008 §25.5.5 p.718                                     */
/*===========================================================================*/

#define SPI_CRCPR_CRCPOLY_Pos   (0U)
#define SPI_CRCPR_CRCPOLY_Msk   (0xFFFFU << SPI_CRCPR_CRCPOLY_Pos) /* 0xFFFF */

/*===========================================================================*/
/* I2SCFGR — SPI_I2S configuration register (offset 0x1C)                    */
/* Source: ir/spi_ir_summary.json registers[7]                               */
/* Reset: 0x0000 — RM0008 §25.5.8 p.719                                     */
/*===========================================================================*/

#define SPI_I2SCFGR_I2SMOD_Pos  (11U)
#define SPI_I2SCFGR_I2SMOD_Msk  (0x1U << SPI_I2SCFGR_I2SMOD_Pos)  /* 0x0800 */

/*===========================================================================*/
/* Peripheral base addresses                                                  */
/* Source: ir/spi_ir_summary.json instances[]                                */
/* STM32F103C8T6: SPI1 (APB2), SPI2 (APB1), no SPI3                         */
/*===========================================================================*/

#define SPI1_BASE       (0x40013000UL)   /* RM0008 §3.3 Table 3 p.51 */
#define SPI2_BASE       (0x40003800UL)   /* RM0008 §3.3 Table 3 p.52 */

#define SPI1            ((SPI_TypeDef *)SPI1_BASE)
#define SPI2            ((SPI_TypeDef *)SPI2_BASE)

#endif /* SPI_REG_H */
