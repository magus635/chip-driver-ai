#ifndef SPI_CFG_H
#define SPI_CFG_H

/* Layer 0b: Compile-Time Configuration Layer
 * STRICT CONFORMITY: Macros ONLY, no include other project headers.
 */

/* Default SPI Instance to Use if Multiple exist */
#define SPI_CFG_DEFAULT_INSTANCE     (1U)

/* Timeout for Wait Loops (Blocking Transactions) */
#define SPI_CFG_TX_TIMEOUT_CT        (100000U)
#define SPI_CFG_RX_TIMEOUT_CT        (100000U)

/* Software Slave Management Configuration */
#define SPI_CFG_USE_SSM              (1U)

/* Minimum clock frequency in MHz */
#define SPI_CFG_MIN_CLK_MHZ          (1U)

#endif 
