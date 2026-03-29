#ifndef SPI_TYPES_H
#define SPI_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/* Layer 0a: Base types for SPI Driver */

typedef enum {
    SPI_OK          = 0,
    SPI_ERR_BUSY    = 1,
    SPI_ERR_PARAM   = 2,
    SPI_ERR_TIMEOUT = 3,
    SPI_ERR_OVR     = 4,  /* Overrun Error Detected */
    SPI_ERR_MODF    = 5,  /* Mode Fault Detected */
    SPI_ERR_CRC     = 6,  /* CRC Mismatch Detected */
} Spi_ReturnType;

typedef enum {
    SPI_DATA_8BIT  = 0,
    SPI_DATA_16BIT = 1
} Spi_DataSizeType;

/* 榨干硬件特性：完整配置结构体 */
typedef struct {
    uint32_t baudrate_prescaler; /* 2, 4, 8, 16, 32, 64, 128, 256. Value: 0..7 as per BR[2:0] */
    uint16_t clk_polarity;       /* 0:CPOL=0, 1:CPOL=1 */
    uint16_t clk_phase;          /* 0:CPHA=0, 1:CPHA=1 */
    Spi_DataSizeType data_size;  /* 8bit or 16bit */
    bool is_master;              /* true:Master, false:Slave */
    bool use_ssm;                /* Software Slave Management */
} Spi_ConfigType;

typedef struct {
    uint8_t *txData;       
    uint8_t *rxData;       
    uint16_t length;       
} Spi_PduType;

#endif 
