/**
 * @file    i2c_reg.h
 * @brief   STM32F103C8T6 I2C 寄存器映射 (Layer 1)
 */

#ifndef I2C_REG_H
#define I2C_REG_H

#include <stdint.h>

#define I2C1_BASE_ADDR    (0x40005400UL)
#define I2C2_BASE_ADDR    (0x40005800UL)

typedef struct
{
    volatile uint32_t CR1;    /* Control register 1,      Offset: 0x00 */
    volatile uint32_t CR2;    /* Control register 2,      Offset: 0x04 */
    volatile uint32_t OAR1;   /* Own address register 1,  Offset: 0x08 */
    volatile uint32_t OAR2;   /* Own address register 2,  Offset: 0x0C */
    volatile uint32_t DR;     /* Data register,           Offset: 0x10 */
    volatile uint32_t SR1;    /* Status register 1,       Offset: 0x14 */
    volatile uint32_t SR2;    /* Status register 2,       Offset: 0x18 */
    volatile uint32_t CCR;    /* Clock control register,  Offset: 0x1C */
    volatile uint32_t TRISE;  /* TRISE register,          Offset: 0x20 */
} I2C_TypeDef;

#define I2C1    ((I2C_TypeDef *)I2C1_BASE_ADDR)
#define I2C2    ((I2C_TypeDef *)I2C2_BASE_ADDR)

/* Bitfield Masks (Derived from IR) */
#define I2C_CR1_PE_Msk        (0x0001U)
#define I2C_CR1_START_Msk     (0x0100U)
#define I2C_CR1_STOP_Msk      (0x0200U)
#define I2C_CR1_ACK_Msk       (0x0400U)

#define I2C_CR2_FREQ_Msk      (0x003FU)
#define I2C_CR2_ITEVTEN_Msk   (0x0200U)
#define I2C_CR2_DMAEN_Msk     (0x0800U)
#define I2C_CR2_LAST_Msk      (0x1000U)

#define I2C_SR1_SB_Msk        (0x0001U)
#define I2C_SR1_ADDR_Msk      (0x0002U)
#define I2C_SR1_BTF_Msk       (0x0004U)
#define I2C_SR1_RxNE_Msk      (0x0040U)
#define I2C_SR1_TxE_Msk       (0x0080U)
#define I2C_SR1_AF_Msk        (0x0400U)

#endif /* I2C_REG_H */
