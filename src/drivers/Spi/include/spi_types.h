/**
 * @file    spi_types.h
 * @brief   STM32F103C8T6 SPI 类型定义 (V4.0)
 */

#ifndef SPI_TYPES_H
#define SPI_TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    SPI_OK              = 0,
    SPI_ERR_BUSY        = 1,
    SPI_ERR_PARAM       = 2,
    SPI_ERR_TIMEOUT     = 3,
    SPI_ERR_NOT_INIT    = 4
} Spi_ReturnType;

typedef enum
{
    SPI_STATE_UNINIT     = 0U,
    SPI_STATE_READY      = 1U,
    SPI_STATE_BUSY_TXRX  = 2U,
    SPI_STATE_ERROR      = 3U
} Spi_State_e;

typedef enum
{
    SPI_MODE_FULL_DUPLEX_MASTER = 0U,
    SPI_MODE_FULL_DUPLEX_SLAVE  = 1U,
    SPI_MODE_HALF_DUPLEX_TX     = 2U,
    SPI_MODE_HALF_DUPLEX_RX     = 3U,
    SPI_MODE_SIMPLEX_RX_MASTER  = 4U
} Spi_OperatingMode_e;

typedef enum { SPI_CPOL_LOW = 0U, SPI_CPOL_HIGH = 1U } Spi_Cpol_e;
typedef enum { SPI_CPHA_1EDGE = 0U, SPI_CPHA_2EDGE = 1U } Spi_Cpha_e;
typedef enum { SPI_BAUDRATE_DIV2 = 0U, SPI_BAUDRATE_DIV256 = 7U } Spi_BaudRate_e;
typedef enum { SPI_DFF_8BIT = 0U, SPI_DFF_16BIT = 1U } Spi_DataFrameFormat_e;
typedef enum { SPI_FIRSTBIT_MSB = 0U, SPI_FIRSTBIT_LSB = 1U } Spi_FirstBit_e;
typedef enum { SPI_NSS_SOFT = 0U, SPI_NSS_HARD_OUT = 1U, SPI_NSS_HARD_IN = 2U } Spi_NssMode_e;

typedef enum
{
    SPI_INSTANCE_1 = 0U,
    SPI_INSTANCE_2 = 1U,
    SPI_INSTANCE_COUNT = 2U
} Spi_Instance_e;

typedef struct
{
    Spi_Instance_e         instance;
    Spi_OperatingMode_e    mode;
    Spi_BaudRate_e         baudRate;
    Spi_Cpol_e             cpol;
    Spi_Cpha_e             cpha;
    Spi_DataFrameFormat_e  dataFrame;
    Spi_FirstBit_e         firstBit;
    Spi_NssMode_e          nssMode;
    bool                   crcEnable;
    uint16_t               crcPolynomial;
} Spi_ConfigType;

#endif /* SPI_TYPES_H */
