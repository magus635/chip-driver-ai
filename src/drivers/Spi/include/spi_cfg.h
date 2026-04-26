/**
 * @file    spi_cfg.h
 * @brief   STM32F103C8T6 SPI 编译期配置层
 *
 * Source:  ir/spi_ir_summary.json v3.0
 */

#ifndef SPI_CFG_H
#define SPI_CFG_H

/* Instance enable */
#define SPI_CFG_SPI1_ENABLE         (1U)
#define SPI_CFG_SPI2_ENABLE         (1U)

/* Timeout loop counts — ir/spi_ir_summary.json timing_constraints */
#define SPI_CFG_TIMEOUT_LOOPS       ((uint32_t)100000U)

/* Default baud rate: DIV16 → SPI1=4.5MHz, SPI2=2.25MHz */
#define SPI_CFG_DEFAULT_BAUDRATE    (3U)    /* SPI_BAUDRATE_DIV16 */

/* IRQ enable (1=interrupt, 0=polling) */
#define SPI_CFG_IRQ_ENABLE          (1U)

/* IRQ numbers — ir/spi_ir_summary.json interrupts[] */
#define SPI_CFG_SPI1_IRQ_NUMBER     (35U)   /* RM0008 Table 63 p.194 */
#define SPI_CFG_SPI2_IRQ_NUMBER     (36U)   /* RM0008 Table 63 p.195 */

/* Default IRQ priority */
#define SPI_CFG_IRQ_PRIORITY        (5U)

/* Default CRC polynomial — ir/spi_ir_summary.json registers[4] */
#define SPI_CFG_DEFAULT_CRC_POLY    ((uint16_t)0x0007U)

/* DMA enable (future use) */
#define SPI_CFG_DMA_ENABLE          (0U)

#endif /* SPI_CFG_H */
