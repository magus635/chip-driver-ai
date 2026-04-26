/**
 * @file    i2c_types.h
 * @brief   STM32F103C8T6 I2C 数据类型定义
 */

#ifndef I2C_TYPES_H
#define I2C_TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    I2C_OK          = 0,
    I2C_ERR         = 1,
    I2C_BUSY        = 2,
    I2C_ERR_PARAM   = 3,
    I2C_ERR_TIMEOUT = 4,
    I2C_AF          = 5  /* Ack Failure */
} I2c_ReturnType;

typedef struct
{
    uint32_t pclk1;     /* APB1 Clock Frequency in Hz */
    uint32_t speed;     /* I2C Bus Speed in Hz (up to 400000) */
    uint16_t ownAddr;   /* Local Slave Address (for slave mode) */
} I2c_ConfigType;

#endif /* I2C_TYPES_H */
