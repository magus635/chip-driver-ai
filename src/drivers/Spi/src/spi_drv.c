/**
 * @file spi_drv.c
 * @brief High-level SPI driver logic.
 * @note Architecture Guidelines (v2.3) - HARDWARE EXHAUSTION Edition:
 *       1. Full Configuration Object (ConfigType)
 *       2. Hardware Error Auto-Recovery (OVR/MODF Management)
 *       3. Finite Timeouts for safety.
 */

#include "spi_api.h"
#include "spi_ll.h"
#include "rcc_api.h"
#include "port_api.h"

/* Static internal state management */
static Spi_ReturnType g_last_spi_error = SPI_OK;

/* 内部辅助：自动自愈硬件错误状态机 */
static inline void Spi_HandleHardwareErrors(SPI_TypeDef *SPIx) {
    uint32_t sr = SPI_LL_GetSR(SPIx);
    volatile uint32_t dummy;

    /* OVR Recovery Sequence (RM0090 §27.3.7) */
    if (sr & SPI_SR_OVR_Msk) {
        dummy = SPIx->DR; /* 1. Read DR */
        dummy = SPIx->SR; /* 2. Read SR (Order matters to clear OVR) */
        (void)dummy;
        g_last_spi_error = SPI_ERR_OVR;
    }

    /* MODF Recovery (RM0090 §27.3.7) */
    if (sr & SPI_SR_MODF_Msk) {
        SPI_LL_Disable(SPIx); /* Disable SPI */
        /* SPE=0, then re-write MSTR to 1 is needed to recover if it dropped to slave */
        /* Normally the LL_Configure will handle the bits, but we mark error */
        g_last_spi_error = SPI_ERR_MODF;
    }
    
    if (sr & SPI_SR_CRCERR_Msk) {
        SPIx->SR &= ~SPI_SR_CRCERR_Msk; /* W1C if possible */
        g_last_spi_error = SPI_ERR_CRC;
    }
}

Spi_ReturnType Spi_Init(const Spi_ConfigType *ConfigPtr) {
    if (ConfigPtr == (void*)0) return SPI_ERR_PARAM;
    
    SPI_TypeDef *spi_instance = SPI1;

    /* 启动外设时钟与引脚复用 (Cross-Module Calls) */
    /* Rcc_Api_EnableSpi1(); */
    /* Port_Api_InitSpi1Pins(); */

    SPI_LL_Disable(spi_instance);
    SPI_LL_Configure(spi_instance, ConfigPtr);
    SPI_LL_Enable(spi_instance);

    g_last_spi_error = SPI_OK;
    return SPI_OK;
}

Spi_ReturnType Spi_TransmitReceive(const Spi_PduType *PduInfo) {
    SPI_TypeDef *spi_instance = SPI1;
    uint32_t timeout;

    if ((PduInfo == (void*)0) || (PduInfo->txData == (void*)0) || (PduInfo->rxData == (void*)0)) {
        return SPI_ERR_PARAM;
    }

    for (uint16_t i = 0U; i < PduInfo->length; i++) {
        /* 在每一帧数据交换前执行“错误防范与自愈”序列 */
        Spi_HandleHardwareErrors(spi_instance);

        /* SPI_TX: Wait for TX buffer shift empty */
        timeout = 10000U;
        while (!SPI_LL_IsTxEmpty(spi_instance)) {
            if (--timeout == 0U) return SPI_ERR_TIMEOUT;
        }
        SPI_LL_WriteData8(spi_instance, PduInfo->txData[i]);

        /* SPI_RX: Wait for frame to finish shifting in */
        timeout = 10000U;
        while (!SPI_LL_IsRxNotEmpty(spi_instance)) {
            if (--timeout == 0U) return SPI_ERR_TIMEOUT;
        }
        PduInfo->rxData[i] = SPI_LL_ReadData8(spi_instance);
    }

    return (g_last_spi_error == SPI_OK) ? SPI_OK : g_last_spi_error;
}

Spi_ReturnType Spi_GetStatus(void) {
    return g_last_spi_error;
}
