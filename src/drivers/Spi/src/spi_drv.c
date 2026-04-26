/**
 * @file    spi_drv.c
 * @brief   STM32F103C8T6 SPI 驱动层实现 (Layer 3)
 *
 * Source:  ir/spi_ir_summary.json v3.0
 */

#include "spi_api.h"
#include "spi_ll.h"
#include "dma_api.h"
#include "spi_cfg.h"
#include <stddef.h>

/*===========================================================================*/
/* Module Static Data (Encapsulation)                                        */
/*===========================================================================*/

static Spi_State_e g_spi_state[SPI_INSTANCE_COUNT] = {
    SPI_STATE_UNINIT, SPI_STATE_UNINIT
};

static uint16_t Spi_DummyData = 0xFFFFU;

/*===========================================================================*/
/* Internal Callbacks                                                        */
/*===========================================================================*/

static void Spi_DmaCallback(void *pContext)
{
    Spi_Instance_e instance = (Spi_Instance_e)(uintptr_t)pContext;
    if (instance < SPI_INSTANCE_COUNT)
    {
        g_spi_state[instance] = SPI_STATE_READY;
    }
}

/*===========================================================================*/
/* Internal Helpers                                                          */
/*===========================================================================*/

static inline SPI_TypeDef* Spi_GetInstance(Spi_Instance_e instance)
{
    return (instance == SPI_INSTANCE_1) ? SPI1 : SPI2;
}

/*===========================================================================*/
/* Public API Implementation                                                 */
/*===========================================================================*/

Spi_ReturnType Spi_Init(const Spi_ConfigType *pConfig)
{
    if (pConfig == NULL || pConfig->instance >= SPI_INSTANCE_COUNT)
    {
        return SPI_ERR_PARAM;
    }

    SPI_TypeDef *SPIx = Spi_GetInstance(pConfig->instance);

    /* 1. Enable Clock (In real project, via RCC_API) */
    /* ... RCC Enable Logic ... */

    /* 2. Guard: INV_SPI_002/004 — Disable before config */
    SPI_LL_Disable(SPIx);

    /* 3. Compose and Write Registers (R8 Composition) */
    uint16_t cr1 = SPI_LL_ComposeCR1(pConfig);
    uint16_t cr2 = SPI_LL_ComposeCR2(pConfig);

    SPI_LL_WriteCR1(SPIx, cr1);
    SPI_LL_WriteCR2(SPIx, cr2);

    /* 4. Configure CRC Polynomial if enabled */
    if (pConfig->crcEnable)
    {
        SPI_LL_WriteCRCPR(SPIx, pConfig->crcPolynomial);
    }

    /* 5. Enable SPI */
    SPI_LL_Enable(SPIx);

    g_spi_state[pConfig->instance] = SPI_STATE_READY;
    return SPI_OK;
}

void Spi_DeInit(Spi_Instance_e instance)
{
    if (instance >= SPI_INSTANCE_COUNT) { return; }

    SPI_TypeDef *SPIx = Spi_GetInstance(instance);

    /* 1. R12 Rule: Follow disable_procedures from IR/RM0008 */
    /* Wait for BSY=0 (INV_SPI_003) */
    while ((SPI_LL_GetSR(SPIx) & SPI_SR_BSY_Msk) != 0U);

    /* 2. Disable SPI */
    SPI_LL_Disable(SPIx);

    /* 3. Reset registers to defaults (R3 compliance) */
    SPI_LL_WriteCR1(SPIx, 0U);
    SPI_LL_WriteCR2(SPIx, 0U);

    g_spi_state[instance] = SPI_STATE_UNINIT;
}

Spi_ReturnType Spi_TransmitReceive(const Spi_ConfigType *pConfig, const void *pTxBuf, void *pRxBuf, uint16_t length)
{
    if (pConfig == NULL || length == 0U) { return SPI_ERR_PARAM; }
    if (g_spi_state[pConfig->instance] != SPI_STATE_READY) { return SPI_ERR_NOT_INIT; }

    SPI_TypeDef *SPIx = Spi_GetInstance(pConfig->instance);
    g_spi_state[pConfig->instance] = SPI_STATE_BUSY_TXRX;

    for (uint16_t i = 0; i < length; i++)
    {
        /* Wait for TXE */
        uint32_t timeout = SPI_TIMEOUT_TICKS;
        while (!(SPI_LL_GetSR(SPIx) & SPI_SR_TXE_Msk))
        {
            if (--timeout == 0U) { goto handle_timeout; }
        }

        /* Write Data */
        if (pConfig->dataFrame == SPI_DFF_16BIT)
        {
            uint16_t val = (pTxBuf != NULL) ? ((const uint16_t*)pTxBuf)[i] : 0xFFFFU;
            SPI_LL_WriteDR(SPIx, val);
        }
        else
        {
            uint8_t val = (pTxBuf != NULL) ? ((const uint8_t*)pTxBuf)[i] : 0xFFU;
            SPI_LL_WriteDR(SPIx, (uint16_t)val);
        }

        /* Wait for RXNE */
        timeout = SPI_TIMEOUT_TICKS;
        while (!(SPI_LL_GetSR(SPIx) & SPI_SR_RXNE_Msk))
        {
            if (--timeout == 0U) { goto handle_timeout; }
        }

        /* Read Data */
        uint16_t rxVal = SPI_LL_ReadDR(SPIx);
        if (pRxBuf != NULL)
        {
            if (pConfig->dataFrame == SPI_DFF_16BIT) { ((uint16_t*)pRxBuf)[i] = rxVal; }
            else { ((uint8_t*)pRxBuf)[i] = (uint8_t)rxVal; }
        }
    }

    /* Wait for BSY=0 (INV_SPI_003) */
    while ((SPI_LL_GetSR(SPIx) & SPI_SR_BSY_Msk) != 0U);

    g_spi_state[pConfig->instance] = SPI_STATE_READY;
    return SPI_OK;

handle_timeout:
    g_spi_state[pConfig->instance] = SPI_STATE_READY;
    return SPI_ERR_TIMEOUT;
}

Dma_ReturnType Spi_TransmitReceiveDma(const Spi_ConfigType *pConfig, const void *pTxBuf, void *pRxBuf, uint16_t length)
{
    if (pConfig == NULL || length == 0U) { return DMA_PARAM; }
    if (g_spi_state[pConfig->instance] != SPI_STATE_READY) { return (Dma_ReturnType)DMA_BUSY; }

    SPI_TypeDef *SPIx = Spi_GetInstance(pConfig->instance);
    Dma_ConfigType dmaCfg;
    Dma_Channel_e txCh, rxCh;

    /* 1. Map SPI instance to DMA channels (IR Metadata) */
    if (pConfig->instance == SPI_INSTANCE_1) {
        rxCh = DMA_CH2; txCh = DMA_CH3;
    } else {
        rxCh = DMA_CH4; txCh = DMA_CH5;
    }

    g_spi_state[pConfig->instance] = SPI_STATE_BUSY_TXRX;

    /* 2. Configure DMA RX Channel */
    dmaCfg.channel = rxCh;
    dmaCfg.direction = DMA_DIR_P2M;
    dmaCfg.srcSize = (pConfig->dataFrame == SPI_DFF_16BIT) ? DMA_SIZE_16BIT : DMA_SIZE_8BIT;
    dmaCfg.dstSize = dmaCfg.srcSize;
    dmaCfg.srcInc = false;
    dmaCfg.dstInc = (pRxBuf != NULL);
    dmaCfg.circular = false;
    dmaCfg.priority = DMA_PRIO_HIGH;
    dmaCfg.mem2mem = false;
    dmaCfg.pCallback = Spi_DmaCallback;
    dmaCfg.pContext = (void*)(uintptr_t)pConfig->instance;
    Dma_InitChannel(&dmaCfg);

    /* 3. Configure DMA TX Channel */
    dmaCfg.channel = txCh;
    dmaCfg.direction = DMA_DIR_M2P;
    dmaCfg.dstInc = false;
    dmaCfg.srcInc = (pTxBuf != NULL);
    Dma_InitChannel(&dmaCfg);

    /* 4. Start RX before TX (Timing Constraint) */
    Dma_StartTransfer(rxCh, (uint32_t)&SPIx->DR, (uint32_t)(pRxBuf ? pRxBuf : &Spi_DummyData), length);
    Dma_StartTransfer(txCh, (uint32_t)(pTxBuf ? pTxBuf : &Spi_DummyData), (uint32_t)&SPIx->DR, length);

    /* 5. Enable SPI DMA requests (Layer 2 Call) */
    SPI_LL_EnableDmaRequests(SPIx);

    return DMA_OK;
}
