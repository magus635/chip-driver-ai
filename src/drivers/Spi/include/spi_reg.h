#ifndef SPI_REG_H
#define SPI_REG_H

#include <stdint.h>

/* Layer 1: CMSIS Register Mapping (STM32F407)
 * STRICT CONFORMITY: No business logic.
 */

#define __I  volatile const
#define __O  volatile
#define __IO volatile

/* SPI CR1 Bits */
#define SPI_CR1_BIDIMODE_Pos (15U)
#define SPI_CR1_BIDIMODE_Msk (1UL << SPI_CR1_BIDIMODE_Pos)
#define SPI_CR1_BIDIOE_Pos   (14U)
#define SPI_CR1_BIDIOE_Msk   (1UL << SPI_CR1_BIDIOE_Pos)
#define SPI_CR1_CRCEN_Pos    (13U)
#define SPI_CR1_CRCEN_Msk    (1UL << SPI_CR1_CRCEN_Pos)
#define SPI_CR1_CRCNEXT_Pos  (12U)
#define SPI_CR1_CRCNEXT_Msk  (1UL << SPI_CR1_CRCNEXT_Pos)
#define SPI_CR1_DFF_Pos      (11U)
#define SPI_CR1_DFF_Msk      (1UL << SPI_CR1_DFF_Pos)
#define SPI_CR1_RXONLY_Pos   (10U)
#define SPI_CR1_RXONLY_Msk   (1UL << SPI_CR1_RXONLY_Pos)
#define SPI_CR1_SSM_Pos      (9U)
#define SPI_CR1_SSM_Msk      (1UL << SPI_CR1_SSM_Pos)
#define SPI_CR1_SSI_Pos      (8U)
#define SPI_CR1_SSI_Msk      (1UL << SPI_CR1_SSI_Pos)
#define SPI_CR1_LSBFIRST_Pos (7U)
#define SPI_CR1_LSBFIRST_Msk (1UL << SPI_CR1_LSBFIRST_Pos)
#define SPI_CR1_SPE_Pos      (6U)
#define SPI_CR1_SPE_Msk      (1UL << SPI_CR1_SPE_Pos)
#define SPI_CR1_BR_Pos       (3U)
#define SPI_CR1_BR_Msk       (7UL << SPI_CR1_BR_Pos)
#define SPI_CR1_MSTR_Pos     (2U)
#define SPI_CR1_MSTR_Msk     (1UL << SPI_CR1_MSTR_Pos)
#define SPI_CR1_CPOL_Pos     (1U)
#define SPI_CR1_CPOL_Msk     (1UL << SPI_CR1_CPOL_Pos)
#define SPI_CR1_CPHA_Pos     (0U)
#define SPI_CR1_CPHA_Msk     (1UL << SPI_CR1_CPHA_Pos)

/* SPI CR2 Bits */
#define SPI_CR2_TXEIE_Pos    (7U)
#define SPI_CR2_TXEIE_Msk    (1UL << SPI_CR2_TXEIE_Pos)
#define SPI_CR2_RXNEIE_Pos   (6U)
#define SPI_CR2_RXNEIE_Msk   (1UL << SPI_CR2_RXNEIE_Pos)
#define SPI_CR2_ERRIE_Pos    (5U)
#define SPI_CR2_ERRIE_Msk    (1UL << SPI_CR2_ERRIE_Pos)
#define SPI_CR2_SSOE_Pos     (2U)
#define SPI_CR2_SSOE_Msk     (1UL << SPI_CR2_SSOE_Pos)
#define SPI_CR2_TXDMAEN_Pos  (1U)
#define SPI_CR2_TXDMAEN_Msk  (1UL << SPI_CR2_TXDMAEN_Pos)
#define SPI_CR2_RXDMAEN_Pos  (0U)
#define SPI_CR2_RXDMAEN_Msk  (1UL << SPI_CR2_RXDMAEN_Pos)

/* SPI SR Bits */
#define SPI_SR_FRE_Pos       (8U)
#define SPI_SR_FRE_Msk       (1UL << SPI_SR_FRE_Pos)
#define SPI_SR_BSY_Pos       (7U)
#define SPI_SR_BSY_Msk       (1UL << SPI_SR_BSY_Pos)
#define SPI_SR_OVR_Pos       (6U)
#define SPI_SR_OVR_Msk       (1UL << SPI_SR_OVR_Pos)
#define SPI_SR_MODF_Pos      (5U)
#define SPI_SR_MODF_Msk      (1UL << SPI_SR_MODF_Pos)
#define SPI_SR_CRCERR_Pos    (4U)
#define SPI_SR_CRCERR_Msk    (1UL << SPI_SR_CRCERR_Pos)
#define SPI_SR_UDR_Pos       (3U)
#define SPI_SR_UDR_Msk       (1UL << SPI_SR_UDR_Pos)
#define SPI_SR_TXE_Pos       (1U)
#define SPI_SR_TXE_Msk       (1UL << SPI_SR_TXE_Pos)
#define SPI_SR_RXNE_Pos      (0U)
#define SPI_SR_RXNE_Msk      (1UL << SPI_SR_RXNE_Pos)

typedef struct {
    __IO uint32_t CR1;      
    __IO uint32_t CR2;      
    __IO uint32_t SR;       
    __IO uint32_t DR;       
    __IO uint32_t CRCPR;    
    __I  uint32_t RXCRCR;   
    __I  uint32_t TXCRCR;   
    __IO uint32_t I2SCFGR;  
    __IO uint32_t I2SPR;    
} SPI_TypeDef;

#define SPI1_BASE (0x40013000UL)
#define SPI1      ((SPI_TypeDef *)SPI1_BASE)

#endif 
