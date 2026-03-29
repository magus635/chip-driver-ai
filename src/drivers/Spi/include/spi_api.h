#ifndef SPI_API_H
#define SPI_API_H

#include "spi_types.h"
#include "spi_cfg.h"

/* Layer 4: Public API Interface (Hardware Exhaustion V2.3)
 * STRICT CONFORMITY: Does not include _ll.h or _reg.h
 */

Spi_ReturnType Spi_Init(const Spi_ConfigType *ConfigPtr);
Spi_ReturnType Spi_TransmitReceive(const Spi_PduType *PduInfo);
Spi_ReturnType Spi_GetStatus(void);

#endif 
