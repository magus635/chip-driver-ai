#ifndef SPI_REG_H
#define SPI_REG_H

#include <stdint.h>

/* STM32F103 SPI Register Mapping (DS §23.5)
 * Layer 1: Register Mapping Layer
 * STRICT CONFORMITY: No logic, only register structures and bitfields.
 */

/* CMSIS-style Access Macros (Normally from cmsis_compiler.h) */
#ifndef __IO
#define __IO volatile
#endif
#ifndef __I
#define __I  volatile const
#endif
#ifndef __O
#define __O  volatile
#endif

/* SPI Register Structure */
typedef struct {
    __IO uint32_t CR1;      /* SPI control register 1           Offset: 0x00 */
    __IO uint32_t CR2;      /* SPI control register 2           Offset: 0x04 */
    __IO uint32_t SR;       /* SPI status register             Offset: 0x08 */
    __IO uint32_t DR;       /* SPI data register               Offset: 0x0C */
    __IO uint32_t CRCPR;    /* SPI CRC polynomial register      Offset: 0x10 */
    __I  uint32_t RXCRCR;   /* SPI RX CRC register             Offset: 0x14 */
    __I  uint32_t TXCRCR;   /* SPI TX CRC register             Offset: 0x18 */
    __IO uint32_t I2SCFGR;  /* SPI_I2S configuration register   Offset: 0x1C */
    __IO uint32_t I2SPR;    /* SPI_I2S prescaler register       Offset: 0x20 */
} SPI_TypeDef;

/* SPI Register Bit Definitions */

/* CR1 */
#define SPI_CR1_BIDIMODE_Pos   (15U)
#define SPI_CR1_BIDIMODE_Msk   (0x1UL << SPI_CR1_BIDIMODE_Pos)
#define SPI_CR1_BIDIOE_Pos     (14U)
#define SPI_CR1_BIDIOE_Msk     (0x1UL << SPI_CR1_BIDIOE_Pos)
#define SPI_CR1_CRCEN_Pos      (13U)
#define SPI_CR1_CRCEN_Msk      (0x1UL << SPI_CR1_CRCEN_Pos)
#define SPI_CR1_CRCNEXT_Pos    (12U)
#define SPI_CR1_CRCNEXT_Msk    (0x1UL << SPI_CR1_CRCNEXT_Pos)
#define SPI_CR1_DFF_Pos        (11U)
#define SPI_CR1_DFF_Msk        (0x1UL << SPI_CR1_DFF_Pos)
#define SPI_CR1_RXONLY_Pos     (10U)
#define SPI_CR1_RXONLY_Msk     (0x1UL << SPI_CR1_RXONLY_Pos)
#define SPI_CR1_SSM_Pos        (9U)
#define SPI_CR1_SSM_Msk        (0x1UL << SPI_CR1_SSM_Pos)
#define SPI_CR1_SSI_Pos        (8U)
#define SPI_CR1_SSI_Msk        (0x1UL << SPI_CR1_SSI_Pos)
#define SPI_CR1_LSBFIRST_Pos   (7U)
#define SPI_CR1_LSBFIRST_Msk   (0x1UL << SPI_CR1_LSBFIRST_Pos)
#define SPI_CR1_SPE_Pos        (6U)
#define SPI_CR1_SPE_Msk        (0x1UL << SPI_CR1_SPE_Pos)
#define SPI_CR1_BR_Pos         (3U)
#define SPI_CR1_BR_Msk         (0x7UL << SPI_CR1_BR_Pos)
#define SPI_CR1_MSTR_Pos       (2U)
#define SPI_CR1_MSTR_Msk       (0x1UL << SPI_CR1_MSTR_Pos)
#define SPI_CR1_CPOL_Pos       (1U)
#define SPI_CR1_CPOL_Msk       (0x1UL << SPI_CR1_CPOL_Pos)
#define SPI_CR1_CPHA_Pos       (0U)
#define SPI_CR1_CPHA_Msk       (0x1UL << SPI_CR1_CPHA_Pos)

/* CR2 */
#define SPI_CR2_TXEIE_Pos      (7U)
#define SPI_CR2_TXEIE_Msk      (0x1UL << SPI_CR2_TXEIE_Pos)
#define SPI_CR2_RXNEIE_Pos     (6U)
#define SPI_CR2_RXNEIE_Msk     (0x1UL << SPI_CR2_RXNEIE_Pos)
#define SPI_CR2_ERRIE_Pos      (5U)
#define SPI_CR2_ERRIE_Msk      (0x1UL << SPI_CR2_ERRIE_Pos)
#define SPI_CR2_SSOE_Pos       (2U)
#define SPI_CR2_SSOE_Msk       (0x1UL << SPI_CR2_SSOE_Pos)
#define SPI_CR2_TXDMAEN_Pos    (1U)
#define SPI_CR2_TXDMAEN_Msk    (0x1UL << SPI_CR2_TXDMAEN_Pos)
#define SPI_CR2_RXDMAEN_Pos    (0U)
#define SPI_CR2_RXDMAEN_Msk    (0x1UL << SPI_CR2_RXDMAEN_Pos)

/* SR */
#define SPI_SR_BSY_Pos         (7U)
#define SPI_SR_BSY_Msk         (0x1UL << SPI_SR_BSY_Pos)
#define SPI_SR_OVR_Pos         (6U)
#define SPI_SR_OVR_Msk         (0x1UL << SPI_SR_OVR_Pos)
#define SPI_SR_MODF_Pos        (5U)
#define SPI_SR_MODF_Msk        (0x1UL << SPI_SR_MODF_Pos)
#define SPI_SR_CRCERR_Pos      (4U)
#define SPI_SR_CRCERR_Msk      (0x1UL << SPI_SR_CRCERR_Pos)
#define SPI_SR_UDR_Pos         (3U)
#define SPI_SR_UDR_Msk         (0x1UL << SPI_SR_UDR_Pos)
#define SPI_SR_CHSIDE_Pos      (2U)
#define SPI_SR_CHSIDE_Msk      (0x1UL << SPI_SR_CHSIDE_Pos)
#define SPI_SR_TXE_Pos         (1U)
#define SPI_SR_TXE_Msk         (0x1UL << SPI_SR_TXE_Pos)
#define SPI_SR_RXNE_Pos        (0U)
#define SPI_SR_RXNE_Msk        (0x1UL << SPI_SR_RXNE_Pos)

/* SPI Base Addresses */
#define SPI1_BASE              (0x40013000UL)
#define SPI2_BASE              (0x40003800UL)
#define SPI3_BASE              (0x40003C00UL)

#define SPI1                   ((SPI_TypeDef *)SPI1_BASE)
#define SPI2                   ((SPI_TypeDef *)SPI2_BASE)
#define SPI3                   ((SPI_TypeDef *)SPI3_BASE)

#endif
