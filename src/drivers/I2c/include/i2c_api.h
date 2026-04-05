#ifndef I2C_API_H
#define I2C_API_H

#include "i2c_types.h"
#include "i2c_reg.h"

/**
 * @file i2c_api.h
 * @brief I2C High-Level Driver API (V2.0 Extended)
 * @note Supports polling, interrupt modes, memory access, and bus recovery
 */

/*============================================================================
 * Initialization
 *===========================================================================*/

/**
 * @brief Initialize the I2C peripheral with given configuration.
 */
I2C_StatusType I2C_Init(I2C_TypeDef *I2Cx, const I2C_ConfigType *Config);

/**
 * @brief Initialize I2C handle for interrupt/DMA modes.
 */
I2C_StatusType I2C_Handle_Init(I2C_HandleType *hi2c);

/**
 * @brief De-initialize the I2C peripheral.
 */
I2C_StatusType I2C_DeInit(I2C_TypeDef *I2Cx);

/*============================================================================
 * Polling Mode (Blocking)
 *===========================================================================*/

/**
 * @brief Master Transmit data to slave (blocking).
 */
I2C_StatusType I2C_MasterTransmit(I2C_TypeDef *I2Cx, uint16_t SlaveAddr,
                                   const uint8_t *Data, uint16_t Size, uint32_t Timeout);

/**
 * @brief Master Receive data from slave (blocking).
 */
I2C_StatusType I2C_MasterReceive(I2C_TypeDef *I2Cx, uint16_t SlaveAddr,
                                  uint8_t *Data, uint16_t Size, uint32_t Timeout);

/*============================================================================
 * Memory Access (Polling)
 *===========================================================================*/

/**
 * @brief Write data to device memory/register (blocking).
 * @param I2Cx       I2C peripheral instance
 * @param DevAddr    7-bit device address (without R/W bit)
 * @param MemAddr    Memory/register address
 * @param MemAddrSize Address size: I2C_MEMADDR_SIZE_8BIT or I2C_MEMADDR_SIZE_16BIT
 * @param Data       Pointer to data buffer
 * @param Size       Number of bytes to write
 * @param Timeout    Timeout in ticks
 */
I2C_StatusType I2C_MemWrite(I2C_TypeDef *I2Cx, uint16_t DevAddr, uint16_t MemAddr,
                             I2C_MemAddrSizeType MemAddrSize, const uint8_t *Data,
                             uint16_t Size, uint32_t Timeout);

/**
 * @brief Read data from device memory/register (blocking).
 */
I2C_StatusType I2C_MemRead(I2C_TypeDef *I2Cx, uint16_t DevAddr, uint16_t MemAddr,
                            I2C_MemAddrSizeType MemAddrSize, uint8_t *Data,
                            uint16_t Size, uint32_t Timeout);

/*============================================================================
 * Device Detection
 *===========================================================================*/

/**
 * @brief Check if a device is ready on the I2C bus.
 * @param I2Cx       I2C peripheral instance
 * @param DevAddr    7-bit device address
 * @param Trials     Number of trials
 * @param Timeout    Timeout per trial in ticks
 * @return I2C_STATUS_OK if device responds, I2C_STATUS_ERROR otherwise
 */
I2C_StatusType I2C_IsDeviceReady(I2C_TypeDef *I2Cx, uint16_t DevAddr,
                                  uint32_t Trials, uint32_t Timeout);

/*============================================================================
 * Interrupt Mode (Non-blocking)
 *===========================================================================*/

/**
 * @brief Master Transmit in interrupt mode (non-blocking).
 */
I2C_StatusType I2C_MasterTransmit_IT(I2C_HandleType *hi2c, uint16_t DevAddr,
                                      uint8_t *Data, uint16_t Size);

/**
 * @brief Master Receive in interrupt mode (non-blocking).
 */
I2C_StatusType I2C_MasterReceive_IT(I2C_HandleType *hi2c, uint16_t DevAddr,
                                     uint8_t *Data, uint16_t Size);

/**
 * @brief Memory Write in interrupt mode (non-blocking).
 */
I2C_StatusType I2C_MemWrite_IT(I2C_HandleType *hi2c, uint16_t DevAddr,
                                uint16_t MemAddr, I2C_MemAddrSizeType MemAddrSize,
                                uint8_t *Data, uint16_t Size);

/**
 * @brief Memory Read in interrupt mode (non-blocking).
 */
I2C_StatusType I2C_MemRead_IT(I2C_HandleType *hi2c, uint16_t DevAddr,
                               uint16_t MemAddr, I2C_MemAddrSizeType MemAddrSize,
                               uint8_t *Data, uint16_t Size);

/*============================================================================
 * IRQ Handlers (call from ISR)
 *===========================================================================*/

/**
 * @brief Event interrupt handler.
 */
void I2C_EV_IRQHandler(I2C_HandleType *hi2c);

/**
 * @brief Error interrupt handler.
 */
void I2C_ER_IRQHandler(I2C_HandleType *hi2c);

/*============================================================================
 * Bus Recovery
 *===========================================================================*/

/**
 * @brief Recover I2C bus from stuck state using GPIO bitbang.
 * @param I2Cx       I2C peripheral instance
 * @param GpioConfig GPIO configuration for SCL/SDA pins
 * @return I2C_STATUS_OK if recovery successful
 * @note This implements the 9-clock-pulse recovery per I2C specification
 */
I2C_StatusType I2C_BusRecovery(I2C_TypeDef *I2Cx, const I2C_GpioConfigType *GpioConfig);

/*============================================================================
 * Utility Functions
 *===========================================================================*/

/**
 * @brief Get current I2C error code.
 */
uint32_t I2C_GetError(const I2C_HandleType *hi2c);

/**
 * @brief Abort ongoing I2C transfer.
 */
I2C_StatusType I2C_Abort(I2C_HandleType *hi2c);

#endif /* I2C_API_H */
