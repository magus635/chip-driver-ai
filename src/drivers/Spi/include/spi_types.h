/**
 * @file spi_types.h
 * @brief SPI driver type definitions for STM32F103C8T6.
 *
 * This file contains all type definitions, enumerations, and structures
 * used by the SPI driver. No function declarations or register references
 * are included. Compliant with MISRA-C:2012.
 */

#ifndef SPI_TYPES_H
#define SPI_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief SPI operation return codes.
 */
typedef enum
{
    SPI_OK            = 0,  /**< Operation completed successfully */
    SPI_ERR_BUSY      = 1,  /**< SPI peripheral is busy */
    SPI_ERR_PARAM     = 2,  /**< Invalid parameter supplied */
    SPI_ERR_TIMEOUT   = 3,  /**< Operation timed out */
    SPI_ERR_OVERRUN   = 4,  /**< Data overrun error */
    SPI_ERR_MODF      = 5,  /**< Mode fault error */
    SPI_ERR_CRCERR    = 6,  /**< CRC mismatch error */
    SPI_ERR_NOT_INIT  = 7   /**< Driver not initialized */
} Spi_ReturnType;

/**
 * @brief SPI driver state.
 */
typedef enum
{
    SPI_STATE_UNINIT    = 0,  /**< Driver not yet initialized */
    SPI_STATE_IDLE      = 1,  /**< Initialized and ready */
    SPI_STATE_BUSY_TX   = 2,  /**< Transmission in progress */
    SPI_STATE_BUSY_RX   = 3,  /**< Reception in progress */
    SPI_STATE_BUSY_TX_RX = 4, /**< Full-duplex transfer in progress */
    SPI_STATE_ERROR     = 5   /**< Error state */
} Spi_StateType;

/**
 * @brief SPI master/slave mode selection.
 */
typedef enum
{
    SPI_MODE_MASTER = 0,  /**< Master mode */
    SPI_MODE_SLAVE  = 1   /**< Slave mode */
} Spi_ModeType;

/**
 * @brief SPI baud rate prescaler values.
 *
 * The SPI clock is derived from APB clock divided by the prescaler.
 */
typedef enum
{
    SPI_BAUD_DIV2   = 0,  /**< fPCLK / 2 */
    SPI_BAUD_DIV4   = 1,  /**< fPCLK / 4 */
    SPI_BAUD_DIV8   = 2,  /**< fPCLK / 8 */
    SPI_BAUD_DIV16  = 3,  /**< fPCLK / 16 */
    SPI_BAUD_DIV32  = 4,  /**< fPCLK / 32 */
    SPI_BAUD_DIV64  = 5,  /**< fPCLK / 64 */
    SPI_BAUD_DIV128 = 6,  /**< fPCLK / 128 */
    SPI_BAUD_DIV256 = 7   /**< fPCLK / 256 */
} Spi_BaudPrescalerType;

/**
 * @brief SPI data frame format.
 */
typedef enum
{
    SPI_FRAME_8BIT  = 0,  /**< 8-bit data frame */
    SPI_FRAME_16BIT = 1   /**< 16-bit data frame */
} Spi_DataFrameType;

/**
 * @brief SPI clock polarity (CPOL).
 */
typedef enum
{
    SPI_CPOL_LOW  = 0,  /**< Clock idle state is low */
    SPI_CPOL_HIGH = 1   /**< Clock idle state is high */
} Spi_ClockPolarityType;

/**
 * @brief SPI clock phase (CPHA).
 */
typedef enum
{
    SPI_CPHA_1EDGE = 0,  /**< Data captured on first clock edge */
    SPI_CPHA_2EDGE = 1   /**< Data captured on second clock edge */
} Spi_ClockPhaseType;

/**
 * @brief SPI NSS (slave select) management mode.
 */
typedef enum
{
    SPI_NSS_HARD = 0,  /**< Hardware NSS management */
    SPI_NSS_SOFT = 1   /**< Software NSS management */
} Spi_NssManageType;

/**
 * @brief SPI bit transmission order.
 */
typedef enum
{
    SPI_MSB_FIRST = 0,  /**< Most significant bit transmitted first */
    SPI_LSB_FIRST = 1   /**< Least significant bit transmitted first */
} Spi_BitOrderType;

/**
 * @brief SPI peripheral configuration structure.
 */
typedef struct
{
    Spi_ModeType           mode;            /**< Master or slave mode */
    Spi_BaudPrescalerType  baud_prescaler;  /**< Baud rate prescaler */
    Spi_DataFrameType      data_frame;      /**< Data frame format (8/16 bit) */
    Spi_ClockPolarityType  cpol;            /**< Clock polarity */
    Spi_ClockPhaseType     cpha;            /**< Clock phase */
    Spi_NssManageType      nss_manage;      /**< NSS management mode */
    Spi_BitOrderType       bit_order;       /**< Bit transmission order */
    bool                   crc_enable;      /**< CRC calculation enable */
    uint16_t               crc_polynomial;  /**< CRC polynomial, default 0x0007 */
} Spi_ConfigType;

/**
 * @brief SPI runtime status information.
 */
typedef struct
{
    Spi_StateType state;        /**< Current driver state */
    uint32_t      error_count;  /**< Cumulative error count */
    uint32_t      tx_count;     /**< Total bytes/frames transmitted */
    uint32_t      rx_count;     /**< Total bytes/frames received */
} Spi_StatusType;

#endif /* SPI_TYPES_H */
