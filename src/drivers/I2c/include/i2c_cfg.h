#ifndef I2C_CFG_H
#define I2C_CFG_H

#include "i2c_types.h"

/**
 * @file i2c_cfg.h
 * @brief Default I2C Configuration for STM32F4
 */

/**
 * @brief Default I2C1 Configuration
 * @note PCLK1 = 42MHz (APB1 at 168MHz/4, RM0090 §6.2)
 */
static const I2C_ConfigType g_I2C1_DefaultConfig = {
    .pclk1_freq     = 42000000U,
    .clock_speed    = I2C_SPEED_STANDARD,
    .duty_cycle     = I2C_DUTYCYCLE_2,
    .own_address1   = 0x00U,
    .address_10bit  = false,
    .ack_enable     = true
};

#endif /* I2C_CFG_H */
