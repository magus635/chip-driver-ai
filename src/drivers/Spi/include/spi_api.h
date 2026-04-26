/**
 * @file    spi_api.h
 * @brief   STM32F103C8T6 SPI 公共接口定义 (V4.0)
 */

#ifndef SPI_API_H
#define SPI_API_H

#include "spi_types.h"
#include "dma_types.h"

/**
 * @brief  Initialize SPI peripheral.
 * @param  pConfig Pointer to configuration structure.
 * @return SPI_OK on success.
 */
Spi_ReturnType Spi_Init(const Spi_ConfigType *pConfig);

/**
 * @brief  De-initialize SPI peripheral.
 * @param  instance SPI instance index.
 */
void Spi_DeInit(Spi_Instance_e instance);

/**
 * @brief  Transmit and receive data synchronously (Polling).
 * @param  pConfig Pointer to configuration structure.
 * @param  pTxBuf  Pointer to transmit buffer (can be NULL).
 * @param  pRxBuf  Pointer to receive buffer (can be NULL).
 * @param  length  Number of data units.
 * @return SPI_OK on success.
 */
Spi_ReturnType Spi_TransmitReceive(const Spi_ConfigType *pConfig, const void *pTxBuf, void *pRxBuf, uint16_t length);

/**
 * @brief  Transmit and receive data asynchronously (DMA).
 * @param  pConfig Pointer to configuration structure.
 * @param  pTxBuf  Pointer to transmit buffer (can be NULL).
 * @param  pRxBuf  Pointer to receive buffer (can be NULL).
 * @param  length  Number of data units.
 * @return DMA_OK on success.
 */
Dma_ReturnType Spi_TransmitReceiveDma(const Spi_ConfigType *pConfig, const void *pTxBuf, void *pRxBuf, uint16_t length);

#endif /* SPI_API_H */
