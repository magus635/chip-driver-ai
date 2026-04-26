/**
 * @file    spi_api.h
 * @brief   STM32F103C8T6 SPI 公共接口层 (API)
 *
 * @note    对上层唯一暴露的稳定接口。
 *          禁止 include spi_ll.h 或 spi_reg.h（规则 R8-7）。
 *
 * Source:  RM0008 Rev 21, §25
 *          ir/spi_ir_summary.json v3.0
 */

#ifndef SPI_API_H
#define SPI_API_H

#include "spi_types.h"
#include "spi_cfg.h"

/*===========================================================================*/
/* Initialisation / De-initialisation (Init/DeInit 配对 — R8#9)              */
/*===========================================================================*/

/**
 * @brief  Initialise SPI according to the provided configuration.
 *
 *         Init sequence (ir/spi_ir_summary.json functional_model.init_sequence):
 *           1. Enable SPI peripheral clock via RCC.
 *           2. Configure GPIO pins (SCK/MOSI: AF_PP; MISO: Input_Float).
 *           3. Write CR1: CPOL, CPHA, BR, MSTR, SSM/SSI, DFF, LSBFIRST (SPE=0).
 *              Guard: INV_SPI_001 — MSTR=1 && SSM=1 => SSI=1.
 *           4. Write CR2: SSOE, interrupt enables.
 *           5. Set CR1.SPE=1 (INV_SPI_002 — BR/CPOL/CPHA locked after this).
 *
 * @param[in]  pConfig  Configuration. Must not be NULL.
 * @return     SPI_OK, SPI_ERR_PARAM, SPI_ERR_BUSY.
 */
Spi_ReturnType Spi_Init(const Spi_ConfigType *pConfig);

/**
 * @brief  De-initialise SPI, release clock, restore default state.
 *         Follows SEQ_DISABLE_FULLDUPLEX if in full-duplex mode.
 *         Guard: INV_SPI_003 — waits BSY=0 before clearing SPE.
 *
 * @param[in]  instance  SPI_INSTANCE_1 or SPI_INSTANCE_2.
 * @return     SPI_OK, SPI_ERR_NOT_INIT, SPI_ERR_TIMEOUT.
 */
Spi_ReturnType Spi_DeInit(Spi_Instance_e instance);

/*===========================================================================*/
/* Polling transfer                                                           */
/*===========================================================================*/

/**
 * @brief  Transmit data in polling mode (full-duplex, discard RX).
 *
 * @param[in]  instance   SPI instance.
 * @param[in]  pTxData    TX buffer. Must not be NULL.
 * @param[in]  length     Number of bytes (8-bit) or halfwords (16-bit).
 * @return     SPI_OK, SPI_ERR_PARAM, SPI_ERR_TIMEOUT, SPI_ERR_BUSY.
 */
Spi_ReturnType Spi_Transmit(Spi_Instance_e instance,
                            const uint8_t *pTxData,
                            uint16_t length);

/**
 * @brief  Receive data in polling mode (full-duplex, send dummy 0xFF).
 *
 * @param[in]  instance   SPI instance.
 * @param[out] pRxData    RX buffer. Must not be NULL.
 * @param[in]  length     Number of bytes (8-bit) or halfwords (16-bit).
 * @return     SPI_OK, SPI_ERR_PARAM, SPI_ERR_TIMEOUT, SPI_ERR_BUSY.
 */
Spi_ReturnType Spi_Receive(Spi_Instance_e instance,
                           uint8_t *pRxData,
                           uint16_t length);

/**
 * @brief  Full-duplex transmit and receive in polling mode.
 *
 * @param[in]  instance   SPI instance.
 * @param[in]  pTxData    TX buffer. Must not be NULL.
 * @param[out] pRxData    RX buffer. Must not be NULL.
 * @param[in]  length     Number of bytes/halfwords.
 * @return     SPI_OK, SPI_ERR_PARAM, SPI_ERR_TIMEOUT, SPI_ERR_BUSY.
 */
Spi_ReturnType Spi_TransmitReceive(Spi_Instance_e instance,
                                   const uint8_t *pTxData,
                                   uint8_t *pRxData,
                                   uint16_t length);

/*===========================================================================*/
/* Status                                                                     */
/*===========================================================================*/

/**
 * @brief  Get the current driver state.
 */
Spi_State_e Spi_GetState(Spi_Instance_e instance);

/**
 * @brief  Get error status snapshot.
 *
 * @param[in]  instance   SPI instance.
 * @param[out] pStatus    Output. Must not be NULL.
 * @return     SPI_OK or SPI_ERR_PARAM.
 */
Spi_ReturnType Spi_GetErrorStatus(Spi_Instance_e instance,
                                  Spi_ErrorStatusType *pStatus);

/*===========================================================================*/
/* Callback registration                                                      */
/*===========================================================================*/

Spi_ReturnType Spi_RegisterTxRxCallback(Spi_TxRxCallback_t callback);
Spi_ReturnType Spi_RegisterErrorCallback(Spi_ErrorCallback_t callback);

/*===========================================================================*/
/* ISR entry points (called from spi_isr.c)                                  */
/* Source: ir/spi_ir_summary.json interrupts[]                               */
/*===========================================================================*/

void Spi_IRQHandler(Spi_Instance_e instance);

Dma_ReturnType Spi_TransmitReceiveDma(const Spi_ConfigType *pConfig, const void *pTxBuf, void *pRxBuf, uint16_t length);

#endif /* SPI_API_H */
