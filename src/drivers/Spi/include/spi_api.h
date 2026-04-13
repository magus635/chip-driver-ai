/**
 * @file    spi_api.h
 * @brief   SPI driver public API for STM32F103C8T6
 * @target  STM32F103C8T6
 * @ref     RM0008 Rev 21, Chapter 25 — Serial peripheral interface (SPI)
 *
 * External interface for the SPI driver. Only spi_types.h and spi_cfg.h are
 * included. spi_ll.h and spi_reg.h must NOT be included here — implementation
 * details must not leak into the API layer (architecture guideline R8.7).
 *
 * Supported instances: SPI_INSTANCE_1 (SPI1, APB2) and SPI_INSTANCE_2 (SPI2, APB1).
 * SPI3 does not exist on STM32F103C8T6 (Medium-Density). — RM0008 §25
 */

#ifndef SPI_API_H
#define SPI_API_H

#include <stdint.h>
#include "spi_types.h"
#include "spi_cfg.h"

/* ── Instance selector (opaque index, not a register pointer) ─────────────── */
typedef uint8_t Spi_InstanceType;

/** SPI1 — APB2 bus, max 36 MHz (fPCLK2/2 at 72 MHz). — RM0008 §25, §3.3 */
#define SPI_INSTANCE_1  (0U)

/** SPI2 — APB1 bus, max 18 MHz (fPCLK1/2 at 36 MHz). — RM0008 §25, §3.3 */
#define SPI_INSTANCE_2  (1U)

/* ═══════════════════════════════════════════════════════════════════════════
 *  Lifecycle — Init / DeInit must always be used as a pair
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Initialize an SPI peripheral instance.
 *
 * Sequence (RM0008 §25.3.3):
 *   1. Enable APB clock via RCC.
 *   2. Configure GPIO (caller's responsibility — depends on application pinout).
 *   3. Write CR1 with SPE=0 (Guard: INV_SPI_002).
 *   4. Write CR2 (interrupt / DMA enables).
 *   5. Set CR1.SPE=1.
 *
 * Invariants enforced:
 *   INV_SPI_001: if SSM=1 and MSTR=1, SSI is forced to 1.
 *   INV_SPI_002: BR/CPOL/CPHA written only while SPE=0.
 *   INV_SPI_004: DFF/CRCEN written only while SPE=0.
 *
 * @param  instance  SPI_INSTANCE_1 or SPI_INSTANCE_2
 * @param  cfg       Pointer to configuration structure (must not be NULL)
 * @return SPI_OK on success; SPI_ERR_PARAM if instance or cfg is invalid
 */
Spi_ReturnType Spi_Init(Spi_InstanceType instance, const Spi_ConfigType *cfg);

/**
 * @brief  De-initialize an SPI instance and disable the peripheral clock.
 *
 * Waits for BSY=0 before clearing SPE (Guard: INV_SPI_003).
 * Disables all interrupts in CR2 before disabling.
 *
 * @param  instance  SPI_INSTANCE_1 or SPI_INSTANCE_2
 * @return SPI_OK on success; SPI_ERR_TIMEOUT if BSY never cleared
 */
Spi_ReturnType Spi_DeInit(Spi_InstanceType instance);

/* ═══════════════════════════════════════════════════════════════════════════
 *  Polling-mode data transfer
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Transmit a buffer in polling mode (received data discarded).
 *
 * For 8-bit frames, p_data is treated as uint8_t array.
 * For 16-bit frames, p_data is reinterpreted as uint16_t array; size is
 * the number of 16-bit frames, and p_data must be 2-byte aligned.
 *
 * @param  instance    SPI_INSTANCE_1 or SPI_INSTANCE_2
 * @param  p_data      Pointer to transmit buffer (must not be NULL)
 * @param  size        Number of data frames to send
 * @param  timeout_ms  Per-frame timeout in milliseconds
 * @return SPI_OK, SPI_ERR_TIMEOUT, SPI_ERR_BUSY, SPI_ERR_NOT_INIT, SPI_ERR_PARAM
 */
Spi_ReturnType Spi_Transmit(Spi_InstanceType  instance,
                             const uint8_t    *p_data,
                             uint16_t          size,
                             uint32_t          timeout_ms);

/**
 * @brief  Receive a buffer in polling mode (0xFF transmitted as dummy bytes).
 *
 * @param  instance    SPI_INSTANCE_1 or SPI_INSTANCE_2
 * @param  p_buf       Pointer to receive buffer (must not be NULL)
 * @param  size        Number of data frames to receive
 * @param  timeout_ms  Per-frame timeout in milliseconds
 * @return SPI_OK, SPI_ERR_TIMEOUT, SPI_ERR_OVERRUN, SPI_ERR_NOT_INIT, SPI_ERR_PARAM
 */
Spi_ReturnType Spi_Receive(Spi_InstanceType  instance,
                            uint8_t          *p_buf,
                            uint16_t          size,
                            uint32_t          timeout_ms);

/**
 * @brief  Full-duplex polling transmit and receive.
 *
 * @param  instance    SPI_INSTANCE_1 or SPI_INSTANCE_2
 * @param  p_tx_data   Pointer to transmit buffer (must not be NULL)
 * @param  p_rx_buf    Pointer to receive buffer (must not be NULL)
 * @param  size        Number of data frames
 * @param  timeout_ms  Per-frame timeout in milliseconds
 * @return SPI_OK, SPI_ERR_TIMEOUT, SPI_ERR_OVERRUN, SPI_ERR_NOT_INIT, SPI_ERR_PARAM
 */
Spi_ReturnType Spi_TransmitReceive(Spi_InstanceType  instance,
                                    const uint8_t    *p_tx_data,
                                    uint8_t          *p_rx_buf,
                                    uint16_t          size,
                                    uint32_t          timeout_ms);

/* ═══════════════════════════════════════════════════════════════════════════
 *  Status and control
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Retrieve the current driver status for an instance.
 * @param  instance  SPI_INSTANCE_1 or SPI_INSTANCE_2
 * @param  p_status  Output pointer — filled with state, error count, tx/rx counts
 * @return SPI_OK or SPI_ERR_PARAM (if p_status is NULL or instance invalid)
 */
Spi_ReturnType Spi_GetStatus(Spi_InstanceType instance, Spi_StatusType *p_status);

/**
 * @brief  Abort an ongoing transfer and return driver to IDLE state.
 *         Clears pending error flags (OVR, MODF, CRCERR) after abort.
 * @param  instance  SPI_INSTANCE_1 or SPI_INSTANCE_2
 * @return SPI_OK or SPI_ERR_PARAM
 */
Spi_ReturnType Spi_AbortTransfer(Spi_InstanceType instance);

/* ═══════════════════════════════════════════════════════════════════════════
 *  ISR callback registration
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Callback type invoked from the SPI ISR on transfer-complete or error.
 * @param  instance  Which SPI instance triggered the event
 * @param  event     SPI_OK (transfer done) or an error code
 */
typedef void (*Spi_CallbackType)(Spi_InstanceType instance, Spi_ReturnType event);

/**
 * @brief  Register (or deregister) an event callback for the given instance.
 * @param  instance   SPI_INSTANCE_1 or SPI_INSTANCE_2
 * @param  callback   Function pointer; NULL to disable callbacks
 * @return SPI_OK or SPI_ERR_PARAM
 */
Spi_ReturnType Spi_RegisterCallback(Spi_InstanceType instance, Spi_CallbackType callback);

#endif /* SPI_API_H */
