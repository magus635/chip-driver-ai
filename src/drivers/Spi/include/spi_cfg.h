/**
 * @file    spi_cfg.h
 * @brief   STM32F103C8T6 SPI 编译期配置层
 *
 * @note    只允许使用 #define，禁止包含类型定义或函数声明。
 *          禁止 include 任何项目头文件。
 *          所有宏命名带 SPI_CFG_ 前缀。
 *
 * Source:  RM0008 Rev 14
 */

#ifndef SPI_CFG_H
#define SPI_CFG_H

/*===========================================================================*/
/* Instance enable flags                                                      */
/* Set to 1 to include the instance in the driver, 0 to exclude it.         */
/*===========================================================================*/

#define SPI_CFG_SPI1_ENABLE         (1U)  /* SPI1: APB2, base 0x40013000 */
#define SPI_CFG_SPI2_ENABLE         (1U)  /* SPI2: APB1, base 0x40003800 */

/*===========================================================================*/
/* Transfer timeout (loop iteration counts)                                   */
/* All polling loops must exit after this many iterations.                   */
/*===========================================================================*/

#define SPI_CFG_TIMEOUT_LOOPS       ((uint32_t)100000U)

/*===========================================================================*/
/* Default configuration parameters                                           */
/*===========================================================================*/

/* Default baud rate prescaler: fPCLK / 8 */
#define SPI_CFG_DEFAULT_BAUD_DIV    (2U)

/* Default clock mode: Mode 0 (CPOL=0, CPHA=0) */
#define SPI_CFG_DEFAULT_CLOCK_MODE  (0U)

/* Default data frame: 8-bit */
#define SPI_CFG_DEFAULT_DFF         (0U)

/* Default bit order: MSB first */
#define SPI_CFG_DEFAULT_FIRSTBIT    (0U)

/* Default CRC polynomial (reset value) */
/* Source: RM0008 §25.5.5 p.718 */
#define SPI_CFG_DEFAULT_CRC_POLY    ((uint16_t)0x0007U)

/*===========================================================================*/
/* Interrupt / DMA feature selection                                          */
/*===========================================================================*/

/* Enable interrupt-driven transfer support (1=enabled, 0=polling only) */
#define SPI_CFG_IRQ_ENABLE          (1U)

/* SPI1 IRQ number (NVIC position 35) */
/* Source: RM0008 §10.1.2 Table 63 p.196 */
#define SPI_CFG_SPI1_IRQ_NUMBER     (35U)

/* SPI2 IRQ number (NVIC position 36) */
/* Source: RM0008 §10.1.2 Table 63 p.197 */
#define SPI_CFG_SPI2_IRQ_NUMBER     (36U)

/* Default IRQ preemption priority */
#define SPI_CFG_IRQ_PRIORITY        (5U)

/*===========================================================================*/
/* RX ring buffer size (must be power of 2)                                  */
/*===========================================================================*/

#define SPI_CFG_RX_BUF_SIZE         ((uint32_t)64U)

#endif /* SPI_CFG_H */
