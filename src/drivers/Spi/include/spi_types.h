/**
 * @file    spi_types.h
 * @brief   STM32F103C8T6 SPI 类型定义层
 *
 * @note    集中定义跨层共用的数据结构、枚举、错误码。
 *          禁止包含任何函数声明/实现或寄存器引用。
 *          只允许 include C 标准库头文件。
 *
 * Source:  RM0008 Rev 14, §25.3 SPI functional description
 */

#ifndef SPI_TYPES_H
#define SPI_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/*===========================================================================*/
/* Return / error code enumeration                                            */
/*===========================================================================*/

typedef enum
{
    SPI_OK              = 0,  /* Operation successful                        */
    SPI_ERR_BUSY        = 1,  /* Peripheral or transfer in progress          */
    SPI_ERR_PARAM       = 2,  /* Invalid parameter                           */
    SPI_ERR_TIMEOUT     = 3,  /* Wait loop exceeded timeout limit            */
    SPI_ERR_OVERRUN     = 4,  /* SR.OVR — overrun detected (RC_SEQ)          */
    SPI_ERR_MODE_FAULT  = 5,  /* SR.MODF — mode fault detected (RC_SEQ)      */
    SPI_ERR_CRC         = 6,  /* SR.CRCERR — CRC mismatch (W0C)             */
    SPI_ERR_NOT_INIT    = 7   /* Module not initialised                      */
} Spi_ReturnType;

/*===========================================================================*/
/* Baud rate prescaler                                                        */
/* Source: RM0008 §25.5.1 p.715 CR1.BR[2:0]                                 */
/*===========================================================================*/

typedef enum
{
    SPI_BAUD_DIV2   = 0U,  /* fPCLK / 2   */
    SPI_BAUD_DIV4   = 1U,  /* fPCLK / 4   */
    SPI_BAUD_DIV8   = 2U,  /* fPCLK / 8   */
    SPI_BAUD_DIV16  = 3U,  /* fPCLK / 16  */
    SPI_BAUD_DIV32  = 4U,  /* fPCLK / 32  */
    SPI_BAUD_DIV64  = 5U,  /* fPCLK / 64  */
    SPI_BAUD_DIV128 = 6U,  /* fPCLK / 128 */
    SPI_BAUD_DIV256 = 7U   /* fPCLK / 256 */
} Spi_BaudDiv_e;

/*===========================================================================*/
/* Clock mode (CPOL / CPHA)                                                  */
/* Source: RM0008 §25.3.3 p.680                                              */
/*===========================================================================*/

typedef enum
{
    SPI_MODE_0 = 0U,  /* CPOL=0, CPHA=0 — idle low,  capture on rising edge  */
    SPI_MODE_1 = 1U,  /* CPOL=0, CPHA=1 — idle low,  capture on falling edge */
    SPI_MODE_2 = 2U,  /* CPOL=1, CPHA=0 — idle high, capture on falling edge */
    SPI_MODE_3 = 3U   /* CPOL=1, CPHA=1 — idle high, capture on rising edge  */
} Spi_ClockMode_e;

/*===========================================================================*/
/* Data frame format                                                          */
/* Source: RM0008 §25.5.1 p.714 CR1.DFF                                     */
/*===========================================================================*/

typedef enum
{
    SPI_DFF_8BIT  = 0U,  /* 8-bit data frame  */
    SPI_DFF_16BIT = 1U   /* 16-bit data frame */
} Spi_DataFrameFormat_e;

/*===========================================================================*/
/* Bit order                                                                  */
/* Source: RM0008 §25.5.1 p.715 CR1.LSBFIRST                                */
/*===========================================================================*/

typedef enum
{
    SPI_FIRSTBIT_MSB = 0U,  /* MSB transmitted first */
    SPI_FIRSTBIT_LSB = 1U   /* LSB transmitted first */
} Spi_FirstBit_e;

/*===========================================================================*/
/* Communication mode (BIDIMODE / RXONLY)                                     */
/* Source: RM0008 §25.3.5 p.682, Table 177; IR operation_modes[]            */
/*===========================================================================*/

typedef enum
{
    SPI_COMM_FULL_DUPLEX   = 0U,  /* BIDIMODE=0, RXONLY=0: 2-line, TX+RX      */
    SPI_COMM_RECEIVE_ONLY  = 1U,  /* BIDIMODE=0, RXONLY=1: 2-line, RX only    */
    SPI_COMM_BIDI_TX       = 2U,  /* BIDIMODE=1, BIDIOE=1: 1-line, TX         */
    SPI_COMM_BIDI_RX       = 3U   /* BIDIMODE=1, BIDIOE=0: 1-line, RX         */
} Spi_CommMode_e;

/*===========================================================================*/
/* NSS management mode                                                        */
/* Source: RM0008 §25.3.1 p.676, §25.5.1 p.715 CR1.SSM, §25.5.2 p.716 CR2.SSOE */
/* IR: configuration_strategies[]                                            */
/*===========================================================================*/

typedef enum
{
    SPI_NSS_SOFT        = 0U,  /* Software NSS: SSM=1, SSI per role         */
    SPI_NSS_HARD_INPUT  = 1U,  /* Hardware NSS input: SSM=0, SSOE=0         */
    SPI_NSS_HARD_OUTPUT = 2U   /* Hardware NSS output: SSM=0, SSOE=1        */
} Spi_NssMode_e;

/*===========================================================================*/
/* Master / slave selection                                                   */
/* Source: RM0008 §25.5.1 p.715 CR1.MSTR                                    */
/*===========================================================================*/

typedef enum
{
    SPI_SLAVE  = 0U,  /* Slave configuration  */
    SPI_MASTER = 1U   /* Master configuration */
} Spi_MasterSlave_e;

/*===========================================================================*/
/* SPI instance index                                                         */
/*===========================================================================*/

typedef enum
{
    SPI_INSTANCE_1 = 0U,  /* SPI1: base 0x40013000, APB2 */
    SPI_INSTANCE_2 = 1U,  /* SPI2: base 0x40003800, APB1 */
    SPI_INSTANCE_COUNT    /* Sentinel — number of instances */
} Spi_InstanceIndex_e;

/*===========================================================================*/
/* Runtime module state                                                       */
/*===========================================================================*/

typedef enum
{
    SPI_STATE_UNINIT   = 0U,  /* Module not initialised     */
    SPI_STATE_IDLE     = 1U,  /* Initialised, no transfer   */
    SPI_STATE_BUSY_TX  = 2U,  /* Transmit in progress       */
    SPI_STATE_BUSY_RX  = 3U,  /* Receive in progress        */
    SPI_STATE_BUSY_TRX = 4U,  /* Full-duplex in progress    */
    SPI_STATE_ERROR    = 5U   /* Error state, needs recovery */
} Spi_State_e;

/*===========================================================================*/
/* Configuration structure (passed to Spi_Init)                              */
/*===========================================================================*/

typedef struct
{
    Spi_InstanceIndex_e    instanceIndex;  /* Which SPI peripheral to configure */
    Spi_MasterSlave_e      masterSlave;    /* Master or slave mode               */
    Spi_CommMode_e         commMode;       /* Communication mode (duplex/rx/bidi) */
    Spi_BaudDiv_e          baudDiv;        /* Clock prescaler                    */
    Spi_ClockMode_e        clockMode;      /* CPOL/CPHA combination              */
    Spi_DataFrameFormat_e  dataFrame;      /* 8-bit or 16-bit frame              */
    Spi_FirstBit_e         firstBit;       /* MSB or LSB first                   */
    Spi_NssMode_e          nssMode;        /* NSS management strategy            */
    bool                   crcEnable;      /* Hardware CRC enable                */
    uint16_t               crcPolynomial;  /* CRC polynomial (default 0x0007)    */
} Spi_ConfigType;

/*===========================================================================*/
/* Status snapshot (returned by Spi_GetStatus)                               */
/*===========================================================================*/

typedef struct
{
    Spi_State_e  state;      /* Current driver state       */
    bool         isBusy;     /* SR.BSY flag                */
    bool         txEmpty;    /* SR.TXE flag                */
    bool         rxNotEmpty; /* SR.RXNE flag               */
    bool         overrun;    /* SR.OVR flag                */
    bool         modeFault;  /* SR.MODF flag               */
    bool         crcError;   /* SR.CRCERR flag             */
} Spi_StatusType;

#endif /* SPI_TYPES_H */
