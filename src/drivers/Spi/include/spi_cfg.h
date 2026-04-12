/**
 * @file spi_cfg.h
 * @brief SPI driver compile-time configuration for STM32F103C8T6.
 */

#ifndef SPI_CFG_H
#define SPI_CFG_H

/** Number of SPI peripheral instances available (SPI1 + SPI2 on C8T6) */
#define SPI_CFG_INSTANCE_COUNT    (2U)

/** Transmit timeout in milliseconds */
#define SPI_CFG_TX_TIMEOUT_MS     (1000U)

/** Receive timeout in milliseconds */
#define SPI_CFG_RX_TIMEOUT_MS     (1000U)

/** Default baud-rate prescaler (3 = fPCLK/16) */
#define SPI_CFG_DEFAULT_BAUD      (3U)

/** SPI interrupt priority level */
#define SPI_CFG_ISR_PRIORITY      (5U)

/** CRC calculation: 0 = disabled, 1 = enabled */
#define SPI_CFG_USE_CRC           (0U)

/** DMA mode: 0 = polling, 1 = DMA */
#define SPI_CFG_DMA_ENABLE        (0U)

#endif /* SPI_CFG_H */
