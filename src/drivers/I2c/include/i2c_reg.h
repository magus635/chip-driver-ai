#ifndef I2C_REG_H
#define I2C_REG_H

#include <stdint.h>

/**
 * @file i2c_reg.h
 * @brief STM32F103 I2C Register Mapping (RM0008 §26.6)
 * @note V2.1 Strict Machine Contract Compliance
 * 
 * Layer 1: Register Mapping Layer
 * STRICT CONFORMITY: Fixed offsets, width-aware access, no logic.
 */

/* CMSIS-style Access Macros */
#ifndef __IO
#define __IO volatile
#endif
#ifndef __I
#define __I  volatile const
#endif

/* I2C Register Structure */
typedef struct {
    __IO uint32_t CR1;      /**< I2C Control register 1           Offset: 0x00 */
    __IO uint32_t CR2;      /**< I2C Control register 2           Offset: 0x04 */
    __IO uint32_t OAR1;     /**< I2C Own address register 1      Offset: 0x08 */
    __IO uint32_t OAR2;     /**< I2C Own address register 2      Offset: 0x0C */
    __IO uint32_t DR;       /**< I2C Data register               Offset: 0x10 */
    __IO uint32_t SR1;      /**< I2C Status register 1            Offset: 0x14 */
    __IO uint32_t SR2;      /**< I2C Status register 2            Offset: 0x18 */
    __IO uint32_t CCR;      /**< I2C Clock control register      Offset: 0x1C */
    __IO uint32_t TRISE;    /**< I2C TRISE register              Offset: 0x20 */
} I2C_TypeDef;

/* I2C Register Bit Definitions */

/* CR1 - Control Register 1 */
#define I2C_CR1_SWRST_Pos      (15U)
#define I2C_CR1_SWRST_Msk      (0x1UL << I2C_CR1_SWRST_Pos)
#define I2C_CR1_SMBALERT_Pos   (13U)
#define I2C_CR1_SMBALERT_Msk   (0x1UL << I2C_CR1_SMBALERT_Pos)
#define I2C_CR1_PEC_Pos        (12U)
#define I2C_CR1_PEC_Msk        (0x1UL << I2C_CR1_PEC_Pos)
#define I2C_CR1_POS_Pos        (11U)
#define I2C_CR1_POS_Msk        (0x1UL << I2C_CR1_POS_Pos)
#define I2C_CR1_ACK_Pos        (10U)
#define I2C_CR1_ACK_Msk        (0x1UL << I2C_CR1_ACK_Pos)
#define I2C_CR1_STOP_Pos       (9U)
#define I2C_CR1_STOP_Msk       (0x1UL << I2C_CR1_STOP_Pos)
#define I2C_CR1_START_Pos      (8U)
#define I2C_CR1_START_Msk      (0x1UL << I2C_CR1_START_Pos)
#define I2C_CR1_NOSTRETCH_Pos  (7U)
#define I2C_CR1_NOSTRETCH_Msk  (0x1UL << I2C_CR1_NOSTRETCH_Pos)
#define I2C_CR1_ENGC_Pos       (6U)
#define I2C_CR1_ENGC_Msk       (0x1UL << I2C_CR1_ENGC_Pos)
#define I2C_CR1_ENPEC_Pos      (5U)
#define I2C_CR1_ENPEC_Msk      (0x1UL << I2C_CR1_ENPEC_Pos)
#define I2C_CR1_ENARP_Pos      (4U)
#define I2C_CR1_ENARP_Msk      (0x1UL << I2C_CR1_ENARP_Pos)
#define I2C_CR1_SMBTYPE_Pos    (3U)
#define I2C_CR1_SMBTYPE_Msk    (0x1UL << I2C_CR1_SMBTYPE_Pos)
#define I2C_CR1_SMBUS_Pos      (1U)
#define I2C_CR1_SMBUS_Msk      (0x1UL << I2C_CR1_SMBUS_Pos)
#define I2C_CR1_PE_Pos         (0U)
#define I2C_CR1_PE_Msk         (0x1UL << I2C_CR1_PE_Pos)

/* CR2 - Control Register 2 */
#define I2C_CR2_LAST_Pos       (12U)
#define I2C_CR2_LAST_Msk       (0x1UL << I2C_CR2_LAST_Pos)
#define I2C_CR2_DMAEN_Pos      (11U)
#define I2C_CR2_DMAEN_Msk      (0x1UL << I2C_CR2_DMAEN_Pos)
#define I2C_CR2_ITBUFEN_Pos    (10U)
#define I2C_CR2_ITBUFEN_Msk    (0x1UL << I2C_CR2_ITBUFEN_Pos)
#define I2C_CR2_ITEVTEN_Pos    (9U)
#define I2C_CR2_ITEVTEN_Msk    (0x1UL << I2C_CR2_ITEVTEN_Pos)
#define I2C_CR2_ITERREN_Pos    (8U)
#define I2C_CR2_ITERREN_Msk    (0x1UL << I2C_CR2_ITERREN_Pos)
#define I2C_CR2_FREQ_Pos       (0U)
#define I2C_CR2_FREQ_Msk       (0x3FUL << I2C_CR2_FREQ_Pos)

/* SR1 - Status Register 1 */
#define I2C_SR1_SMBALERT_Pos   (15U)
#define I2C_SR1_SMBALERT_Msk   (0x1UL << I2C_SR1_SMBALERT_Pos)
#define I2C_SR1_TIMEOUT_Pos    (14U)
#define I2C_SR1_TIMEOUT_Msk    (0x1UL << I2C_SR1_TIMEOUT_Pos)
#define I2C_SR1_PECERR_Pos     (12U)
#define I2C_SR1_PECERR_Msk     (0x1UL << I2C_SR1_PECERR_Pos)
#define I2C_SR1_OVR_Pos        (11U)
#define I2C_SR1_OVR_Msk        (0x1UL << I2C_SR1_OVR_Pos)
#define I2C_SR1_AF_Pos         (10U)
#define I2C_SR1_AF_Msk         (0x1UL << I2C_SR1_AF_Pos)
#define I2C_SR1_ARLO_Pos       (9U)
#define I2C_SR1_ARLO_Msk       (0x1UL << I2C_SR1_ARLO_Pos)
#define I2C_SR1_BERR_Pos       (8U)
#define I2C_SR1_BERR_Msk       (0x1UL << I2C_SR1_BERR_Pos)
#define I2C_SR1_TXE_Pos        (7U)
#define I2C_SR1_TXE_Msk        (0x1UL << I2C_SR1_TXE_Pos)
#define I2C_SR1_RXNE_Pos       (6U)
#define I2C_SR1_RXNE_Msk       (0x1UL << I2C_SR1_RXNE_Pos)
#define I2C_SR1_STOPF_Pos      (4U)
#define I2C_SR1_STOPF_Msk      (0x1UL << I2C_SR1_STOPF_Pos)
#define I2C_SR1_ADD10_Pos      (3U)
#define I2C_SR1_ADD10_Msk      (0x1UL << I2C_SR1_ADD10_Pos)
#define I2C_SR1_BTF_Pos        (2U)
#define I2C_SR1_BTF_Msk        (0x1UL << I2C_SR1_BTF_Pos)
#define I2C_SR1_ADDR_Pos       (1U)
#define I2C_SR1_ADDR_Msk       (0x1UL << I2C_SR1_ADDR_Pos)
#define I2C_SR1_SB_Pos         (0U)
#define I2C_SR1_SB_Msk         (0x1UL << I2C_SR1_SB_Pos)

/* SR2 - Status Register 2 */
#define I2C_SR2_PEC_Pos        (8U)
#define I2C_SR2_PEC_Msk        (0xFFUL << I2C_SR2_PEC_Pos)
#define I2C_SR2_DUALF_Pos      (7U)
#define I2C_SR2_DUALF_Msk      (0x1UL << I2C_SR2_DUALF_Pos)
#define I2C_SR2_SMBHOST_Pos    (6U)
#define I2C_SR2_SMBHOST_Msk    (0x1UL << I2C_SR2_SMBHOST_Pos)
#define I2C_SR2_SMBDEFAULT_Pos (5U)
#define I2C_SR2_SMBDEFAULT_Msk (0x1UL << I2C_SR2_SMBDEFAULT_Pos)
#define I2C_SR2_GENCALL_Pos    (4U)
#define I2C_SR2_GENCALL_Msk    (0x1UL << I2C_SR2_GENCALL_Pos)
#define I2C_SR2_TRA_Pos        (2U)
#define I2C_SR2_TRA_Msk        (0x1UL << I2C_SR2_TRA_Pos)
#define I2C_SR2_BUSY_Pos       (1U)
#define I2C_SR2_BUSY_Msk       (0x1UL << I2C_SR2_BUSY_Pos)
#define I2C_SR2_MSL_Pos        (0U)
#define I2C_SR2_MSL_Msk        (0x1UL << I2C_SR2_MSL_Pos)

/* CCR - Clock Control Register */
#define I2C_CCR_FS_Pos         (15U)
#define I2C_CCR_FS_Msk         (0x1UL << I2C_CCR_FS_Pos)
#define I2C_CCR_DUTY_Pos       (14U)
#define I2C_CCR_DUTY_Msk       (0x1UL << I2C_CCR_DUTY_Pos)
#define I2C_CCR_CCR_Pos        (0U)
#define I2C_CCR_CCR_Msk        (0xFFFUL << I2C_CCR_CCR_Pos)

/* I2C Base Addresses */
#define I2C1_BASE              (0x40005400UL)
#define I2C2_BASE              (0x40005800UL)

#define I2C1                   ((I2C_TypeDef *)I2C1_BASE)
#define I2C2                   ((I2C_TypeDef *)I2C2_BASE)

#endif /* I2C_REG_H */
