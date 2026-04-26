/**
 * @file    spi_types.h
 * @brief   STM32F103C8T6 SPI 类型定义层
 *
 * Source:  RM0008 Rev 21, §25
 *          ir/spi_ir_summary.json v3.0
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
    SPI_ERR_NOT_INIT    = 4,
    SPI_ERR_OVERRUN     = 5,  /* SR.OVR — RM0008 §25.3.10  */
    SPI_ERR_MODF        = 6,  /* SR.MODF — RM0008 §25.3.10 */
    SPI_ERR_CRCERR      = 7   /* SR.CRCERR                  */
} Spi_ReturnType;

typedef enum
{
    SPI_MODE_FULL_DUPLEX_MASTER  = 0U,
    SPI_MODE_FULL_DUPLEX_SLAVE   = 1U,
    SPI_MODE_HALF_DUPLEX_TX      = 2U,
    SPI_MODE_HALF_DUPLEX_RX      = 3U,
    SPI_MODE_SIMPLEX_RX_MASTER   = 4U
} Spi_OperatingMode_e;

typedef enum { SPI_CPOL_LOW = 0U, SPI_CPOL_HIGH = 1U } Spi_Cpol_e;
typedef enum { SPI_CPHA_1EDGE = 0U, SPI_CPHA_2EDGE = 1U } Spi_Cpha_e;

typedef enum
{
    SPI_BAUDRATE_DIV2   = 0U,
    SPI_BAUDRATE_DIV4   = 1U,
    SPI_BAUDRATE_DIV8   = 2U,
    SPI_BAUDRATE_DIV16  = 3U,
    SPI_BAUDRATE_DIV32  = 4U,
    SPI_BAUDRATE_DIV64  = 5U,
    SPI_BAUDRATE_DIV128 = 6U,
    SPI_BAUDRATE_DIV256 = 7U
} Spi_BaudRate_e;

typedef enum { SPI_DFF_8BIT = 0U, SPI_DFF_16BIT = 1U } Spi_DataFrameFormat_e;
typedef enum { SPI_FIRSTBIT_MSB = 0U, SPI_FIRSTBIT_LSB = 1U } Spi_FirstBit_e;

typedef enum
{
    SPI_NSS_SOFT     = 0U,
    SPI_NSS_HARD_OUT = 1U,
    SPI_NSS_HARD_IN  = 2U
} Spi_NssMode_e;

typedef enum
{
    SPI_INSTANCE_1 = 0U,
    SPI_INSTANCE_2 = 1U,
    SPI_INSTANCE_COUNT = 2U
} Spi_Instance_e;

typedef enum
{
    SPI_STATE_UNINIT     = 0U,
    SPI_STATE_READY      = 1U,
    SPI_STATE_BUSY_TX    = 2U,
    SPI_STATE_BUSY_RX    = 3U,
    SPI_STATE_BUSY_TXRX  = 4U,
    SPI_STATE_ERROR      = 5U
} Spi_State_e;

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

typedef struct
{
    Spi_ReturnType  status;
    uint16_t        bytesTransferred;
} Spi_TransferResult;

typedef struct
{
    Spi_State_e  state;
    bool         overrun;
    bool         modeFault;
    bool         crcError;
} Spi_ErrorStatusType;

typedef void (*Spi_TxRxCallback_t)(Spi_Instance_e instance, Spi_ReturnType result);
typedef void (*Spi_ErrorCallback_t)(Spi_Instance_e instance, Spi_ReturnType error);

#endif /* SPI_TYPES_H */
