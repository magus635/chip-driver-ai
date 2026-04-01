#include "spi_api.h"
#include "spi_ll.h"
#include "spi_cfg.h"
#include <stdint.h>
#include <stddef.h>

/* Layer 3: Driver Logic Implementation
 * STRICT CONFORMITY: State machine, Data flow, Timeout management.
 * NO DIRECT REGISTER ACCESS allowed - only via _ll.h
 */

/* Current Instance determined by configuration (for now static) */
#if (SPI_CFG_DEFAULT_INSTANCE == 1U)
#define CURRENT_SPI   SPI1
#elif (SPI_CFG_DEFAULT_INSTANCE == 2U)
#define CURRENT_SPI   SPI2
#elif (SPI_CFG_DEFAULT_INSTANCE == 3U)
#define CURRENT_SPI   SPI3
#endif

/* Module Internal State Tracking */
typedef enum {
    SPI_STATUS_UNINIT = 0,
    SPI_STATUS_IDLE,
    SPI_STATUS_BUSY,
} Spi_InternalStatusType;

static Spi_InternalStatusType g_spi_status = SPI_STATUS_UNINIT;

/* [ARCHITECTURE V2.3] Public API Implementation */

Spi_ReturnType Spi_Init(const Spi_ConfigType *ConfigPtr)
{
    /* Defense: NULL check */
    if (ConfigPtr == NULL) {
        return SPI_ERR_PARAM;
    }

    /* 1. Ensure SPI is disabled before configuration */
    SPI_LL_Disable(CURRENT_SPI);

    /* 2. Configure Hardware registers */
    SPI_LL_Configure(CURRENT_SPI, ConfigPtr);

    /* 3. Pre-clear any stale error flags */
    SPI_LL_ClearOverrun(CURRENT_SPI);

    /* 4. Enable SPI peripheral */
    SPI_LL_Enable(CURRENT_SPI);

    g_spi_status = SPI_STATUS_IDLE;
    return SPI_OK;
}

Spi_ReturnType Spi_TransmitReceive(const Spi_PduType *PduInfo)
{
    uint32_t timeout = SPI_CFG_TX_TIMEOUT_CT;

    /* Defense Checks */
    if (PduInfo == NULL || PduInfo->txData == NULL || PduInfo->rxData == NULL || PduInfo->length == 0U) {
        return SPI_ERR_PARAM;
    }

    /* Wait if Busy */
    if (g_spi_status == SPI_STATUS_BUSY) {
        return SPI_ERR_BUSY;
    }

    g_spi_status = SPI_STATUS_BUSY;

    for (uint16_t i = 0U; i < PduInfo->length; i++) {
        /* Wait for TX Empty */
        timeout = SPI_CFG_TX_TIMEOUT_CT;
        while (!SPI_LL_IsTxEmpty(CURRENT_SPI) && timeout > 0) {
            timeout--;
        }
        if (timeout == 0) {
            g_spi_status = SPI_STATUS_IDLE;
            return SPI_ERR_TIMEOUT;
        }

        /* Write Byte */
        SPI_LL_WriteData(CURRENT_SPI, PduInfo->txData[i]);

        /* Wait for RX Not Empty */
        timeout = SPI_CFG_RX_TIMEOUT_CT;
        while (!SPI_LL_IsRxNotEmpty(CURRENT_SPI) && timeout > 0) {
            timeout--;
        }
        if (timeout == 0) {
            g_spi_status = SPI_STATUS_IDLE;
            return SPI_ERR_TIMEOUT;
        }

        /* Read Byte */
        PduInfo->rxData[i] = (uint8_t)SPI_LL_ReadData(CURRENT_SPI);
    }

    g_spi_status = SPI_STATUS_IDLE;
    return SPI_OK;
}

Spi_ReturnType Spi_GetStatus(void)
{
    if (g_spi_status == SPI_STATUS_UNINIT) {
        return SPI_ERR_PARAM;
    }
    
    if (g_spi_status == SPI_STATUS_BUSY) {
        return SPI_ERR_BUSY;
    }

    /* Hardware errors check (using bitmask from reg.h indirectly) */
    uint32_t sr = SPI_LL_GetStatusFlags(CURRENT_SPI);
    if ((sr & SPI_SR_OVR_Msk) != 0U) return SPI_ERR_OVR;
    if ((sr & SPI_SR_MODF_Msk) != 0U) return SPI_ERR_MODF;
    if ((sr & SPI_SR_CRCERR_Msk) != 0U) return SPI_ERR_CRC;

    return SPI_OK;
}
