/**
 * @file    spi_reg.h
 * @brief   STM32F103C8T6 SPI 寄存器映射层
 *
 * @note    仅包含物理地址、寄存器偏移、位域宏定义和寄存器结构体。
 *          禁止包含任何逻辑代码或函数实现。
 *
 * Source:  RM0008 Rev 14, §25.5 SPI and I2S registers (p.714-720)
 *          RM0008 §3.3 Table 3 p.50 (base addresses)
 */

#ifndef SPI_REG_H
#define SPI_REG_H

#include <stdint.h>

/*===========================================================================*/
/* CMSIS-style access qualifier macros                                        */
/*===========================================================================*/

#ifndef __IO
#define __IO   volatile         /* Read/Write */
#endif

#ifndef __I
#define __I    volatile const   /* Read only  */
#endif

#ifndef __O
#define __O    volatile         /* Write only */
#endif

/*===========================================================================*/
/* SPI base addresses                                                        */
/* Source: RM0008 §3.3 Table 3 p.50                                          */
/*===========================================================================*/

#define SPI1_BASE_ADDR          ((uint32_t)0x40013000UL)  /* APB2 */
#define SPI2_BASE_ADDR          ((uint32_t)0x40003800UL)  /* APB1 */

/*===========================================================================*/
/* SPI register structure                                                     */
/* Source: RM0008 §25.5 p.714; 16-bit registers, 4-byte stride              */
/*===========================================================================*/

typedef struct
{
    __IO uint32_t CR1;      /* Control Register 1,        offset: 0x00, RM0008 §25.5.1 p.714 */
    __IO uint32_t CR2;      /* Control Register 2,        offset: 0x04, RM0008 §25.5.2 p.716 */
    __IO uint32_t SR;       /* Status Register,           offset: 0x08, RM0008 §25.5.3 p.717 */
    __IO uint32_t DR;       /* Data Register,             offset: 0x0C, RM0008 §25.5.4 p.718 */
    __IO uint32_t CRCPR;    /* CRC Polynomial Register,   offset: 0x10, RM0008 §25.5.5 p.718 */
    __I  uint32_t RXCRCR;   /* RX CRC Register (RO),      offset: 0x14, RM0008 §25.5.6 p.719 */
    __I  uint32_t TXCRCR;   /* TX CRC Register (RO),      offset: 0x18, RM0008 §25.5.7 p.719 */
} SPI_TypeDef;

/*===========================================================================*/
/* SPI instance pointers                                                      */
/*===========================================================================*/

#define SPI1    ((SPI_TypeDef *)SPI1_BASE_ADDR)
#define SPI2    ((SPI_TypeDef *)SPI2_BASE_ADDR)

/*===========================================================================*/
/* CR1 bitfield definitions                                                   */
/* Source: RM0008 §25.5.1 p.714-715                                          */
/*===========================================================================*/

#define SPI_CR1_BIDIMODE_Pos    ((uint32_t)15U)
#define SPI_CR1_BIDIMODE_Msk    ((uint32_t)(1UL << SPI_CR1_BIDIMODE_Pos))

#define SPI_CR1_BIDIOE_Pos      ((uint32_t)14U)
#define SPI_CR1_BIDIOE_Msk      ((uint32_t)(1UL << SPI_CR1_BIDIOE_Pos))

#define SPI_CR1_CRCEN_Pos       ((uint32_t)13U)
#define SPI_CR1_CRCEN_Msk       ((uint32_t)(1UL << SPI_CR1_CRCEN_Pos))

#define SPI_CR1_CRCNEXT_Pos     ((uint32_t)12U)
#define SPI_CR1_CRCNEXT_Msk     ((uint32_t)(1UL << SPI_CR1_CRCNEXT_Pos))

#define SPI_CR1_DFF_Pos         ((uint32_t)11U)
#define SPI_CR1_DFF_Msk         ((uint32_t)(1UL << SPI_CR1_DFF_Pos))

#define SPI_CR1_RXONLY_Pos      ((uint32_t)10U)
#define SPI_CR1_RXONLY_Msk      ((uint32_t)(1UL << SPI_CR1_RXONLY_Pos))

#define SPI_CR1_SSM_Pos         ((uint32_t)9U)
#define SPI_CR1_SSM_Msk         ((uint32_t)(1UL << SPI_CR1_SSM_Pos))

#define SPI_CR1_SSI_Pos         ((uint32_t)8U)
#define SPI_CR1_SSI_Msk         ((uint32_t)(1UL << SPI_CR1_SSI_Pos))

#define SPI_CR1_LSBFIRST_Pos    ((uint32_t)7U)
#define SPI_CR1_LSBFIRST_Msk    ((uint32_t)(1UL << SPI_CR1_LSBFIRST_Pos))

#define SPI_CR1_SPE_Pos         ((uint32_t)6U)
#define SPI_CR1_SPE_Msk         ((uint32_t)(1UL << SPI_CR1_SPE_Pos))

#define SPI_CR1_BR_Pos          ((uint32_t)3U)
#define SPI_CR1_BR_Msk          ((uint32_t)(0x7UL << SPI_CR1_BR_Pos))

#define SPI_CR1_MSTR_Pos        ((uint32_t)2U)
#define SPI_CR1_MSTR_Msk        ((uint32_t)(1UL << SPI_CR1_MSTR_Pos))

#define SPI_CR1_CPOL_Pos        ((uint32_t)1U)
#define SPI_CR1_CPOL_Msk        ((uint32_t)(1UL << SPI_CR1_CPOL_Pos))

#define SPI_CR1_CPHA_Pos        ((uint32_t)0U)
#define SPI_CR1_CPHA_Msk        ((uint32_t)(1UL << SPI_CR1_CPHA_Pos))

/*===========================================================================*/
/* CR2 bitfield definitions                                                   */
/* Source: RM0008 §25.5.2 p.716                                              */
/*===========================================================================*/

#define SPI_CR2_TXEIE_Pos       ((uint32_t)7U)
#define SPI_CR2_TXEIE_Msk       ((uint32_t)(1UL << SPI_CR2_TXEIE_Pos))

#define SPI_CR2_RXNEIE_Pos      ((uint32_t)6U)
#define SPI_CR2_RXNEIE_Msk      ((uint32_t)(1UL << SPI_CR2_RXNEIE_Pos))

#define SPI_CR2_ERRIE_Pos       ((uint32_t)5U)
#define SPI_CR2_ERRIE_Msk       ((uint32_t)(1UL << SPI_CR2_ERRIE_Pos))

#define SPI_CR2_SSOE_Pos        ((uint32_t)2U)
#define SPI_CR2_SSOE_Msk        ((uint32_t)(1UL << SPI_CR2_SSOE_Pos))

#define SPI_CR2_TXDMAEN_Pos     ((uint32_t)1U)
#define SPI_CR2_TXDMAEN_Msk     ((uint32_t)(1UL << SPI_CR2_TXDMAEN_Pos))

#define SPI_CR2_RXDMAEN_Pos     ((uint32_t)0U)
#define SPI_CR2_RXDMAEN_Msk     ((uint32_t)(1UL << SPI_CR2_RXDMAEN_Pos))

/*===========================================================================*/
/* SR bitfield definitions                                                    */
/* Source: RM0008 §25.5.3 p.717                                              */
/* NOTE: SR has mixed access: BSY/TXE/RXNE/UDR/CHSIDE=RO, CRCERR=W0C,      */
/*       OVR/MODF=RC_SEQ. NEVER use |= on whole SR.                         */
/*===========================================================================*/

#define SPI_SR_BSY_Pos          ((uint32_t)7U)
#define SPI_SR_BSY_Msk          ((uint32_t)(1UL << SPI_SR_BSY_Pos))

#define SPI_SR_OVR_Pos          ((uint32_t)6U)
#define SPI_SR_OVR_Msk          ((uint32_t)(1UL << SPI_SR_OVR_Pos))

#define SPI_SR_MODF_Pos         ((uint32_t)5U)
#define SPI_SR_MODF_Msk         ((uint32_t)(1UL << SPI_SR_MODF_Pos))

#define SPI_SR_CRCERR_Pos       ((uint32_t)4U)
#define SPI_SR_CRCERR_Msk       ((uint32_t)(1UL << SPI_SR_CRCERR_Pos))

#define SPI_SR_UDR_Pos          ((uint32_t)3U)
#define SPI_SR_UDR_Msk          ((uint32_t)(1UL << SPI_SR_UDR_Pos))

#define SPI_SR_CHSIDE_Pos       ((uint32_t)2U)
#define SPI_SR_CHSIDE_Msk       ((uint32_t)(1UL << SPI_SR_CHSIDE_Pos))

#define SPI_SR_TXE_Pos          ((uint32_t)1U)
#define SPI_SR_TXE_Msk          ((uint32_t)(1UL << SPI_SR_TXE_Pos))

#define SPI_SR_RXNE_Pos         ((uint32_t)0U)
#define SPI_SR_RXNE_Msk         ((uint32_t)(1UL << SPI_SR_RXNE_Pos))

/*===========================================================================*/
/* DR bitfield definitions                                                    */
/* Source: RM0008 §25.5.4 p.718                                              */
/*===========================================================================*/

#define SPI_DR_DR_Pos           ((uint32_t)0U)
#define SPI_DR_DR_Msk           ((uint32_t)(0xFFFFUL << SPI_DR_DR_Pos))

/*===========================================================================*/
/* CRCPR bitfield definitions                                                 */
/* Source: RM0008 §25.5.5 p.718                                              */
/*===========================================================================*/

#define SPI_CRCPR_CRCPOLY_Pos   ((uint32_t)0U)
#define SPI_CRCPR_CRCPOLY_Msk   ((uint32_t)(0xFFFFUL << SPI_CRCPR_CRCPOLY_Pos))

/*===========================================================================*/
/* AFIO remap register for SPI1 pin remapping                               */
/* Source: RM0008 §9.3.10 p.181                                             */
/*===========================================================================*/

#define AFIO_BASE_ADDR          ((uint32_t)0x40010000UL)
#define AFIO_MAPR               (*((__IO uint32_t *)(AFIO_BASE_ADDR + 0x04UL)))

/* SPI1_REMAP: bit 0 of AFIO_MAPR */
/* 0: No remap (NSS/PA4, SCK/PA5, MISO/PA6, MOSI/PA7) */
/* 1: Remap   (NSS/PA15, SCK/PB3, MISO/PB4, MOSI/PB5) */
#define AFIO_MAPR_SPI1_REMAP_Pos  ((uint32_t)0U)
#define AFIO_MAPR_SPI1_REMAP_Msk  ((uint32_t)(1UL << AFIO_MAPR_SPI1_REMAP_Pos))

/*===========================================================================*/
/* RCC register addresses for SPI clock enable                               */
/* Source: RM0008 §7.3.7 p.109, §7.3.8 p.111                               */
/*===========================================================================*/

#define RCC_BASE_ADDR           ((uint32_t)0x40021000UL)

#define RCC_APB1ENR             (*((__IO uint32_t *)(RCC_BASE_ADDR + 0x1CUL)))
#define RCC_APB2ENR             (*((__IO uint32_t *)(RCC_BASE_ADDR + 0x18UL)))
#define RCC_APB1RSTR            (*((__IO uint32_t *)(RCC_BASE_ADDR + 0x10UL)))
#define RCC_APB2RSTR            (*((__IO uint32_t *)(RCC_BASE_ADDR + 0x0CUL)))

/* AFIO clock enable on APB2, bit 0 — RM0008 §7.3.7 p.111 */
#define RCC_APB2ENR_AFIOEN_Pos  ((uint32_t)0U)
#define RCC_APB2ENR_AFIOEN_Msk  ((uint32_t)(1UL << RCC_APB2ENR_AFIOEN_Pos))

/* SPI1 on APB2, bit 12 */
#define RCC_APB2ENR_SPI1EN_Pos  ((uint32_t)12U)
#define RCC_APB2ENR_SPI1EN_Msk  ((uint32_t)(1UL << RCC_APB2ENR_SPI1EN_Pos))

#define RCC_APB2RSTR_SPI1RST_Pos ((uint32_t)12U)
#define RCC_APB2RSTR_SPI1RST_Msk ((uint32_t)(1UL << RCC_APB2RSTR_SPI1RST_Pos))

/* SPI2 on APB1, bit 14 */
#define RCC_APB1ENR_SPI2EN_Pos  ((uint32_t)14U)
#define RCC_APB1ENR_SPI2EN_Msk  ((uint32_t)(1UL << RCC_APB1ENR_SPI2EN_Pos))

#define RCC_APB1RSTR_SPI2RST_Pos ((uint32_t)14U)
#define RCC_APB1RSTR_SPI2RST_Msk ((uint32_t)(1UL << RCC_APB1RSTR_SPI2RST_Pos))

#endif /* SPI_REG_H */
