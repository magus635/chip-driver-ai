/**
 * @file    spi_api.h
 * @brief   STM32F103C8T6 SPI 公共接口层 (API)
 *
 * @note    对上层唯一暴露的稳定接口。
 *          禁止 include spi_ll.h 或 spi_reg.h（规则 R8-7）。
 *          只 include spi_types.h 和 spi_cfg.h。
 *
 * Source:  RM0008 Rev 14, §25.3 SPI functional description
 */

#ifndef SPI_API_H
#define SPI_API_H

#include "spi_types.h"
#include "spi_cfg.h"

/*===========================================================================*/
/* Initialisation / De-initialisation                                         */
/*===========================================================================*/

/**
 * @brief  Initialise an SPI instance according to the provided configuration.
 *
 *         Full init sequence (RM0008 §25.3.3 p.680):
 *           1. Enable peripheral clock (RCC).
 *           2. Guard SPE=0 (INV_SPI_002).
 *           3. Configure CR1: BR, CPOL/CPHA, DFF, LSBFIRST, NSS, MSTR.
 *           4. Configure CR2: interrupt/DMA enables.
 *           5. Enable SPI (SPE=1).
 *
 * @param[in]  pConfig  Pointer to configuration structure. Must not be NULL.
 * @return     SPI_OK on success, SPI_ERR_PARAM if pConfig is NULL or invalid.
 */
Spi_ReturnType Spi_Init(const Spi_ConfigType *pConfig);

/**
 * @brief  De-initialise an SPI instance and release the peripheral clock.
 *
 *         Full disable sequence (RM0008 §25.3.8 p.691):
 *           1. Wait RXNE=1 and read last data.
 *           2. Wait TXE=1.
 *           3. Wait BSY=0 (INV_SPI_003).
 *           4. Clear SPE=0.
 *           5. Disable peripheral clock (RCC).
 *
 * @param[in]  instanceIndex  Instance to de-initialise.
 * @return     SPI_OK on success, SPI_ERR_TIMEOUT if BSY/TXE wait expired,
 *             SPI_ERR_NOT_INIT if not previously initialised.
 */
Spi_ReturnType Spi_DeInit(Spi_InstanceIndex_e instanceIndex);

/*===========================================================================*/
/* Blocking (polling) data transfer                                           */
/*===========================================================================*/

/**
 * @brief  Full-duplex blocking transmit/receive (polling, 8-bit frame).
 *
 *         Transmits each byte in pTxBuf and simultaneously captures received
 *         bytes into pRxBuf. If pRxBuf is NULL, received data is discarded.
 *         If pTxBuf is NULL, 0xFF dummy bytes are transmitted.
 *         All waits are guarded by SPI_CFG_TIMEOUT_LOOPS.
 *
 * @param[in]  instanceIndex  Instance to use.
 * @param[in]  pTxBuf         Transmit buffer (may be NULL for receive-only).
 * @param[out] pRxBuf         Receive buffer (may be NULL for transmit-only).
 * @param[in]  length         Number of bytes to transfer. Must be > 0.
 * @return     SPI_OK, SPI_ERR_BUSY, SPI_ERR_TIMEOUT, SPI_ERR_PARAM,
 *             SPI_ERR_OVERRUN, SPI_ERR_MODE_FAULT, SPI_ERR_NOT_INIT.
 */
Spi_ReturnType Spi_TransferBlocking(Spi_InstanceIndex_e  instanceIndex,
                                    const uint8_t       *pTxBuf,
                                    uint8_t             *pRxBuf,
                                    uint32_t             length);

/**
 * @brief  Blocking transmit only (polling, 8-bit frame).
 *
 * @param[in]  instanceIndex  Instance to use.
 * @param[in]  pTxBuf         Transmit buffer. Must not be NULL.
 * @param[in]  length         Number of bytes. Must be > 0.
 * @return     SPI_OK or error code.
 */
Spi_ReturnType Spi_TransmitBlocking(Spi_InstanceIndex_e  instanceIndex,
                                    const uint8_t       *pTxBuf,
                                    uint32_t             length);

/**
 * @brief  Blocking receive only (polling, 8-bit frame).
 *         Transmits 0xFF dummy bytes to generate clock.
 *
 * @param[in]  instanceIndex  Instance to use.
 * @param[out] pRxBuf         Receive buffer. Must not be NULL.
 * @param[in]  length         Number of bytes. Must be > 0.
 * @return     SPI_OK or error code.
 */
Spi_ReturnType Spi_ReceiveBlocking(Spi_InstanceIndex_e  instanceIndex,
                                   uint8_t             *pRxBuf,
                                   uint32_t             length);

/*===========================================================================*/
/* Interrupt-driven (non-blocking) data transfer                              */
/*===========================================================================*/

/**
 * @brief  Start a non-blocking full-duplex transfer (interrupt-driven).
 *         Returns immediately; completion is signalled via Spi_IsrCallback_t
 *         registered through Spi_RegisterCallback().
 *
 * @param[in]  instanceIndex  Instance to use.
 * @param[in]  pTxBuf         Transmit buffer (may be NULL).
 * @param[out] pRxBuf         Receive buffer (may be NULL).
 * @param[in]  length         Number of bytes. Must be > 0.
 * @return     SPI_OK if transfer started, SPI_ERR_BUSY if already running,
 *             SPI_ERR_PARAM, SPI_ERR_NOT_INIT.
 */
Spi_ReturnType Spi_TransferIT(Spi_InstanceIndex_e  instanceIndex,
                               const uint8_t       *pTxBuf,
                               uint8_t             *pRxBuf,
                               uint32_t             length);

/*===========================================================================*/
/* Callback registration                                                      */
/*===========================================================================*/

/**
 * @brief  Callback type invoked from ISR context on transfer completion
 *         or error.
 *
 * @param[in]  instanceIndex  Which instance completed.
 * @param[in]  result         SPI_OK on success, error code otherwise.
 */
typedef void (*Spi_IsrCallback_t)(Spi_InstanceIndex_e instanceIndex,
                                  Spi_ReturnType      result);

/**
 * @brief  Register a transfer-complete callback for an instance.
 *         Pass NULL to deregister.
 *
 * @param[in]  instanceIndex  Instance to register for.
 * @param[in]  callback       Function pointer (may be NULL).
 * @return     SPI_OK or SPI_ERR_PARAM.
 */
Spi_ReturnType Spi_RegisterCallback(Spi_InstanceIndex_e instanceIndex,
                                    Spi_IsrCallback_t   callback);

/*===========================================================================*/
/* Status query                                                               */
/*===========================================================================*/

/**
 * @brief  Query the current driver and hardware status of an instance.
 *
 * @param[in]  instanceIndex  Instance to query.
 * @param[out] pStatus        Output status structure. Must not be NULL.
 * @return     SPI_OK or SPI_ERR_PARAM.
 */
Spi_ReturnType Spi_GetStatus(Spi_InstanceIndex_e  instanceIndex,
                              Spi_StatusType      *pStatus);

/*===========================================================================*/
/* ISR entry points (called from spi_isr.c vector handlers)                  */
/*===========================================================================*/

/**
 * @brief  Common SPI interrupt handler — must be called from the vector ISR.
 *         Handles TXE, RXNE, MODF, OVR, CRCERR flags.
 *
 * @param[in]  instanceIndex  Instance whose IRQ fired.
 */
void Spi_IRQHandler(Spi_InstanceIndex_e instanceIndex);

#endif /* SPI_API_H */
