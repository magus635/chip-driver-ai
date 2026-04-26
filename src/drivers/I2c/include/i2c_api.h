/**
 * @file    i2c_api.h
 * @brief   STM32F103C8T6 I2C 公共 API (V4.0)
 */

#ifndef I2C_API_H
#define I2C_API_H

#include "i2c_types.h"

I2c_ReturnType I2c_Init(const I2c_ConfigType *pConfig);
void I2c_DeInit(void);

/* Polling APIs */
I2c_ReturnType I2c_MasterTransmit(uint16_t slaveAddr, const uint8_t *pData, uint16_t size);
I2c_ReturnType I2c_MasterReceive(uint16_t slaveAddr, uint8_t *pData, uint16_t size);

/* Asynchronous DMA APIs (Mandated by R8#15) */
I2c_ReturnType I2c_MasterTransmitDMA(uint16_t slaveAddr, const uint8_t *pData, uint16_t size);
I2c_ReturnType I2c_MasterReceiveDMA(uint16_t slaveAddr, uint8_t *pData, uint16_t size);

#endif /* I2C_API_H */
