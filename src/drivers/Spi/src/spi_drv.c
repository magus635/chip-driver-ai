/**
 * @file    spi_drv.c
 * @brief   STM32F103C8T6 SPI 驱动逻辑实现层
 *
 * Source:  ir/spi_ir_summary.json v3.0
 *          RM0008 Rev 21, §25
 */

#include "spi_api.h"
#include "spi_ll.h"
#include "dma_api.h"
#include "spi_types.h"
#include "spi_cfg.h"
#include "rcc_config.h"

#include <stddef.h>

/*===========================================================================*/
/* Module-scope static state (data encapsulation — no extern)                */
/*===========================================================================*/

static Spi_State_e g_spi_state[SPI_INSTANCE_COUNT] = {
    SPI_STATE_UNINIT, SPI_STATE_UNINIT
};

static Spi_ConfigType g_spi_config[SPI_INSTANCE_COUNT];

static Spi_TxRxCallback_t  g_spi_txrx_cb  = NULL;
static Spi_ErrorCallback_t g_spi_error_cb  = NULL;

/*===========================================================================*/
/* Internal helpers                                                           */
/*===========================================================================*/

/**
 * @brief  Wait for TXE=1 with timeout.
 */
static Spi_ReturnType Spi_WaitTxEmpty(const SPI_TypeDef *SPIx)
{
    uint32_t count = SPI_CFG_TIMEOUT_LOOPS;
    while (!SPI_LL_IsTxEmpty(SPIx))
    {
        if (--count == 0U)
        {
            return SPI_ERR_TIMEOUT;
        }
    }
    return SPI_OK;
}

/**
 * @brief  Wait for RXNE=1 with timeout.
 */
static Spi_ReturnType Spi_WaitRxNotEmpty(const SPI_TypeDef *SPIx)
{
    uint32_t count = SPI_CFG_TIMEOUT_LOOPS;
    while (!SPI_LL_IsRxNotEmpty(SPIx))
    {
        if (--count == 0U)
        {
            return SPI_ERR_TIMEOUT;
        }
    }
    return SPI_OK;
}

/**
 * @brief  Wait for BSY=0 with timeout.
 *         Guard: INV_SPI_003 — required before clearing SPE.
 */
static Spi_ReturnType Spi_WaitNotBusy(const SPI_TypeDef *SPIx)
{
    uint32_t count = SPI_CFG_TIMEOUT_LOOPS;
    while (SPI_LL_IsBusy(SPIx))
    {
        if (--count == 0U)
        {
            return SPI_ERR_TIMEOUT;   /* Guard: INV_SPI_003 — RM0008 §25.3.8 */
        }
    }
    return SPI_OK;
}

/**
 * @brief  Disable SPI following SEQ_DISABLE_FULLDUPLEX.
 *         ir/spi_ir_summary.json atomic_sequences[2]
 *         RM0008 §25.3.8 p.692
 */
static Spi_ReturnType Spi_DisableFullDuplex(SPI_TypeDef *SPIx)
{
    Spi_ReturnType ret;

    /* Step 1-2: Wait RXNE=1 then read DR — SEQ_DISABLE_FULLDUPLEX */
    ret = Spi_WaitRxNotEmpty(SPIx);
    if (ret != SPI_OK) { return ret; }
    (void)SPI_LL_ReadData(SPIx);

    /* Step 3: Wait TXE=1 — SEQ_DISABLE_FULLDUPLEX */
    ret = Spi_WaitTxEmpty(SPIx);
    if (ret != SPI_OK) { return ret; }

    /* Step 4: Wait BSY=0 — SEQ_DISABLE_FULLDUPLEX / INV_SPI_003 */
    ret = Spi_WaitNotBusy(SPIx);
    if (ret != SPI_OK) { return ret; }

    /* Step 5: Clear SPE — SEQ_DISABLE_FULLDUPLEX */
    SPI_LL_Disable(SPIx);

    return SPI_OK;
}

/**
 * @brief  Enable SPI clock via RCC.
 *         ir/spi_ir_summary.json clock[]
 */
static void Spi_EnableClock(Spi_Instance_e instance)
{
    if (instance == SPI_INSTANCE_1)
    {
        /* SPI1 on APB2 — RM0008 §7.3.7 p.112 */
        RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    }
    else
    {
        /* SPI2 on APB1 — RM0008 §7.3.8 p.114 */
        RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
    }
}

/**
 * @brief  Disable SPI clock via RCC.
 */
static void Spi_DisableClock(Spi_Instance_e instance)
{
    if (instance == SPI_INSTANCE_1)
    {
        RCC->APB2ENR &= ~RCC_APB2ENR_SPI1EN;
    }
    else
    {
        RCC->APB1ENR &= ~RCC_APB1ENR_SPI2EN;
    }
}

/*===========================================================================*/
/* Public API implementation                                                  */
/*===========================================================================*/

Spi_ReturnType Spi_Init(const Spi_ConfigType *pConfig)
{
    SPI_TypeDef *SPIx;

    /* 1. Basic validation */
    if (pConfig == NULL)
    {
        return SPI_ERR_PARAM;
    }

    if (pConfig->instance >= SPI_INSTANCE_COUNT)
    {
        return SPI_ERR_PARAM;
    }

    if (pConfig->baudRate > SPI_BAUDRATE_DIV256)
    {
        return SPI_ERR_PARAM;
    }

    Spi_Instance_e inst = pConfig->instance;

    if (g_spi_state[inst] != SPI_STATE_UNINIT)
    {
        return SPI_ERR_BUSY;
    }

    SPIx = SPI_LL_GetInstance(inst);
    if (SPIx == NULL)
    {
        return SPI_ERR_PARAM;
    }

    /* Step 1: Enable peripheral clock — init_sequence[0] */
    Spi_EnableClock(inst);

    /* Step 2: GPIO pin config should be done via Port_Api (cross-module) */
    /* Caller responsibility: Port_Api_InitSpiPins() before Spi_Init()    */

    /* Ensure SPI mode (not I2S) — RM0008 §25.5.8 */
    SPI_LL_ForceSpiMode(SPIx);

    /* Step 3: Write CR1 (SPE=0) — init_sequence[2] */
    /* Rule: Composition moved to LL per Guidelines §2 */
    uint16_t cr1 = SPI_LL_ComposeCR1(pConfig);
    SPI_LL_WriteCR1(SPIx, cr1);   /* IR: init_sequence step 3 — RM0008 §25.5.1 */

    /* Set CRC polynomial if enabled — RM0008 §25.5.5 */
    if (pConfig->crcEnable)
    {
        uint16_t poly = (pConfig->crcPolynomial != 0U)
                        ? pConfig->crcPolynomial
                        : SPI_CFG_DEFAULT_CRC_POLY;
        SPI_LL_SetCrcPolynomial(SPIx, poly);
    }

    /* Step 4: Write CR2 — init_sequence[3] */
    /* Rule: Composition moved to LL per Guidelines §2 */
    uint16_t cr2 = SPI_LL_ComposeCR2(pConfig);
#if (SPI_CFG_IRQ_ENABLE == 1U)
    cr2 |= SPI_CR2_RXNEIE_Msk | SPI_CR2_ERRIE_Msk;
#endif
    SPI_LL_WriteCR2(SPIx, cr2);   /* IR: init_sequence step 4 — RM0008 §25.5.2 */

    /* Step 5: Enable SPI (SPE=1) — init_sequence[4] */
    SPI_LL_Enable(SPIx);   /* IR: init_sequence step 5 — RM0008 §25.5.1 */

    /* Save config and update state */
    g_spi_config[inst] = *pConfig;
    g_spi_state[inst] = SPI_STATE_READY;

    return SPI_OK;
}

Spi_ReturnType Spi_DeInit(Spi_Instance_e instance)
{
    if (instance >= SPI_INSTANCE_COUNT)
    {
        return SPI_ERR_PARAM;
    }

    if (g_spi_state[instance] == SPI_STATE_UNINIT)
    {
        return SPI_ERR_NOT_INIT;
    }

    SPI_TypeDef *SPIx = SPI_LL_GetInstance(instance);
    if (SPIx == NULL)
    {
        return SPI_ERR_PARAM;
    }

    Spi_ReturnType ret;

    /* Disable following IR disable_procedures — R8#12 */
    if (SPI_LL_IsEnabled(SPIx))
    {
        /* SEQ_DISABLE_FULLDUPLEX — ir/spi_ir_summary.json atomic_sequences[2] */
        ret = Spi_DisableFullDuplex(SPIx);
        if (ret != SPI_OK)
        {
            /* Fallback: force wait BSY then disable — INV_SPI_003 */
            ret = Spi_WaitNotBusy(SPIx);
            if (ret != SPI_OK)
            {
                g_spi_state[instance] = SPI_STATE_ERROR;
                return ret;
            }
            SPI_LL_Disable(SPIx);
        }
    }

    /* Disable all interrupts */
    SPI_LL_DisableIT_TXE(SPIx);
    SPI_LL_DisableIT_RXNE(SPIx);
    SPI_LL_DisableIT_ERR(SPIx);

    /* Clear pending error flags */
    SPI_LL_ClearOVR(SPIx);
    SPI_LL_ClearCRCERR(SPIx);

    /* Reset registers to default */
    SPI_LL_WriteCR1(SPIx, 0x0000U);
    SPI_LL_WriteCR2(SPIx, 0x0000U);

    /* Disable clock */
    Spi_DisableClock(instance);

    g_spi_state[instance] = SPI_STATE_UNINIT;
    return SPI_OK;
}

Spi_ReturnType Spi_Transmit(Spi_Instance_e instance,
                            const uint8_t *pTxData,
                            uint16_t length)
{
    if ((pTxData == NULL) || (length == 0U))
    {
        return SPI_ERR_PARAM;
    }
    if (instance >= SPI_INSTANCE_COUNT)
    {
        return SPI_ERR_PARAM;
    }
    if (g_spi_state[instance] != SPI_STATE_READY)
    {
        return (g_spi_state[instance] == SPI_STATE_UNINIT)
               ? SPI_ERR_NOT_INIT : SPI_ERR_BUSY;
    }

    SPI_TypeDef *SPIx = SPI_LL_GetInstance(instance);
    if (SPIx == NULL) { return SPI_ERR_PARAM; }

    g_spi_state[instance] = SPI_STATE_BUSY_TX;

    bool is16bit = (g_spi_config[instance].dataFrame == SPI_DFF_16BIT);
    Spi_ReturnType ret = SPI_OK;

    for (uint16_t i = 0U; i < length; i++)
    {
        /* Wait TXE=1 — RM0008 §25.3.7 p.688 */
        ret = Spi_WaitTxEmpty(SPIx);
        if (ret != SPI_OK) { break; }

        if (is16bit)
        {
            uint16_t val = (uint16_t)pTxData[i * 2U]
                         | ((uint16_t)pTxData[i * 2U + 1U] << 8U);
            SPI_LL_WriteData(SPIx, val);   /* IR: registers[3] DR — RM0008 §25.5.4 */
        }
        else
        {
            SPI_LL_WriteData(SPIx, (uint16_t)pTxData[i]);
        }

        /* Wait RXNE and discard (full-duplex: each TX generates RX) */
        ret = Spi_WaitRxNotEmpty(SPIx);
        if (ret != SPI_OK) { break; }
        (void)SPI_LL_ReadData(SPIx);
    }

    /* Wait BSY=0 — RM0008 §25.3.8 */
    if (ret == SPI_OK)
    {
        ret = Spi_WaitNotBusy(SPIx);
    }

    /* Check overrun */
    if (SPI_LL_IsOverrun(SPIx))
    {
        SPI_LL_ClearOVR(SPIx);   /* SEQ_OVR_CLEAR — RM0008 §25.3.10 */
        if (ret == SPI_OK) { ret = SPI_ERR_OVERRUN; }
    }

    g_spi_state[instance] = SPI_STATE_READY;
    return ret;
}

Spi_ReturnType Spi_Receive(Spi_Instance_e instance,
                           uint8_t *pRxData,
                           uint16_t length)
{
    if ((pRxData == NULL) || (length == 0U))
    {
        return SPI_ERR_PARAM;
    }
    if (instance >= SPI_INSTANCE_COUNT)
    {
        return SPI_ERR_PARAM;
    }
    if (g_spi_state[instance] != SPI_STATE_READY)
    {
        return (g_spi_state[instance] == SPI_STATE_UNINIT)
               ? SPI_ERR_NOT_INIT : SPI_ERR_BUSY;
    }

    SPI_TypeDef *SPIx = SPI_LL_GetInstance(instance);
    if (SPIx == NULL) { return SPI_ERR_PARAM; }

    g_spi_state[instance] = SPI_STATE_BUSY_RX;

    bool is16bit = (g_spi_config[instance].dataFrame == SPI_DFF_16BIT);
    Spi_ReturnType ret = SPI_OK;

    for (uint16_t i = 0U; i < length; i++)
    {
        /* Send dummy byte to generate clock — RM0008 §25.3.7 */
        ret = Spi_WaitTxEmpty(SPIx);
        if (ret != SPI_OK) { break; }

        SPI_LL_WriteData(SPIx, 0xFFFFU);   /* dummy — RM0008 §25.5.4 */

        ret = Spi_WaitRxNotEmpty(SPIx);
        if (ret != SPI_OK) { break; }

        uint16_t rxval = SPI_LL_ReadData(SPIx);

        if (is16bit)
        {
            pRxData[i * 2U]      = (uint8_t)(rxval & 0xFFU);
            pRxData[i * 2U + 1U] = (uint8_t)((rxval >> 8U) & 0xFFU);
        }
        else
        {
            pRxData[i] = (uint8_t)(rxval & 0xFFU);
        }
    }

    if (ret == SPI_OK)
    {
        ret = Spi_WaitNotBusy(SPIx);
    }

    if (SPI_LL_IsOverrun(SPIx))
    {
        SPI_LL_ClearOVR(SPIx);
        if (ret == SPI_OK) { ret = SPI_ERR_OVERRUN; }
    }

    g_spi_state[instance] = SPI_STATE_READY;
    return ret;
}

Spi_ReturnType Spi_TransmitReceive(Spi_Instance_e instance,
                                   const uint8_t *pTxData,
                                   uint8_t *pRxData,
                                   uint16_t length)
{
    if ((pTxData == NULL) || (pRxData == NULL) || (length == 0U))
    {
        return SPI_ERR_PARAM;
    }
    if (instance >= SPI_INSTANCE_COUNT)
    {
        return SPI_ERR_PARAM;
    }
    if (g_spi_state[instance] != SPI_STATE_READY)
    {
        return (g_spi_state[instance] == SPI_STATE_UNINIT)
               ? SPI_ERR_NOT_INIT : SPI_ERR_BUSY;
    }

    SPI_TypeDef *SPIx = SPI_LL_GetInstance(instance);
    if (SPIx == NULL) { return SPI_ERR_PARAM; }

    g_spi_state[instance] = SPI_STATE_BUSY_TXRX;

    bool is16bit = (g_spi_config[instance].dataFrame == SPI_DFF_16BIT);
    Spi_ReturnType ret = SPI_OK;

    for (uint16_t i = 0U; i < length; i++)
    {
        ret = Spi_WaitTxEmpty(SPIx);
        if (ret != SPI_OK) { break; }

        if (is16bit)
        {
            uint16_t val = (uint16_t)pTxData[i * 2U]
                         | ((uint16_t)pTxData[i * 2U + 1U] << 8U);
            SPI_LL_WriteData(SPIx, val);
        }
        else
        {
            SPI_LL_WriteData(SPIx, (uint16_t)pTxData[i]);
        }

        ret = Spi_WaitRxNotEmpty(SPIx);
        if (ret != SPI_OK) { break; }

        uint16_t rxval = SPI_LL_ReadData(SPIx);

        if (is16bit)
        {
            pRxData[i * 2U]      = (uint8_t)(rxval & 0xFFU);
            pRxData[i * 2U + 1U] = (uint8_t)((rxval >> 8U) & 0xFFU);
        }
        else
        {
            pRxData[i] = (uint8_t)(rxval & 0xFFU);
        }
    }

    if (ret == SPI_OK)
    {
        ret = Spi_WaitNotBusy(SPIx);
    }

    if (SPI_LL_IsOverrun(SPIx))
    {
        SPI_LL_ClearOVR(SPIx);
        if (ret == SPI_OK) { ret = SPI_ERR_OVERRUN; }
    }

    g_spi_state[instance] = SPI_STATE_READY;
    return ret;
}

Spi_State_e Spi_GetState(Spi_Instance_e instance)
{
    if (instance >= SPI_INSTANCE_COUNT)
    {
        return SPI_STATE_UNINIT;
    }
    return g_spi_state[instance];
}

Spi_ReturnType Spi_GetErrorStatus(Spi_Instance_e instance,
                                  Spi_ErrorStatusType *pStatus)
{
    if ((pStatus == NULL) || (instance >= SPI_INSTANCE_COUNT))
    {
        return SPI_ERR_PARAM;
    }

    SPI_TypeDef *SPIx = SPI_LL_GetInstance(instance);
    if (SPIx == NULL) { return SPI_ERR_PARAM; }

    pStatus->state     = g_spi_state[instance];
    pStatus->overrun   = SPI_LL_IsOverrun(SPIx);
    pStatus->modeFault = SPI_LL_IsModeFault(SPIx);
    pStatus->crcError  = SPI_LL_IsCrcError(SPIx);

    return SPI_OK;
}

Spi_ReturnType Spi_RegisterTxRxCallback(Spi_TxRxCallback_t callback)
{
    g_spi_txrx_cb = callback;
    return SPI_OK;
}

Spi_ReturnType Spi_RegisterErrorCallback(Spi_ErrorCallback_t callback)
{
    g_spi_error_cb = callback;
    return SPI_OK;
}

/*===========================================================================*/
/* ISR dispatch (called from spi_isr.c)                                      */
/* Source: ir/spi_ir_summary.json interrupts[]                               */
/*===========================================================================*/

void Spi_IRQHandler(Spi_Instance_e instance)
{
    if (instance >= SPI_INSTANCE_COUNT)
    {
        return;
    }

    SPI_TypeDef *SPIx = SPI_LL_GetInstance(instance);
    if (SPIx == NULL) { return; }

    uint16_t sr  = SPI_LL_ReadSR(SPIx);
    uint16_t cr2 = SPI_LL_ReadCR2(SPIx);

    /* Error handling — ir/spi_ir_summary.json errors[] */
    if ((sr & SPI_SR_OVR_Msk) && (cr2 & SPI_CR2_ERRIE_Msk))
    {
        SPI_LL_ClearOVR(SPIx);   /* SEQ_OVR_CLEAR — RM0008 §25.3.10 */
        g_spi_state[instance] = SPI_STATE_ERROR;
        if (g_spi_error_cb != NULL)
        {
            g_spi_error_cb(instance, SPI_ERR_OVERRUN);
        }
    }

    if ((sr & SPI_SR_MODF_Msk) && (cr2 & SPI_CR2_ERRIE_Msk))
    {
        SPI_LL_ClearMODF(SPIx);   /* SEQ_MODF_CLEAR — RM0008 §25.3.10 */
        g_spi_state[instance] = SPI_STATE_ERROR;
        if (g_spi_error_cb != NULL)
        {
            g_spi_error_cb(instance, SPI_ERR_MODF);
        }
    }

    if ((sr & SPI_SR_CRCERR_Msk) && (cr2 & SPI_CR2_ERRIE_Msk))
    {
        SPI_LL_ClearCRCERR(SPIx);   /* W0C — RM0008 §25.5.3 */
        if (g_spi_error_cb != NULL)
        {
            g_spi_error_cb(instance, SPI_ERR_CRCERR);
        }
    }

    /* RXNE — ir/spi_ir_summary.json interrupts[].sources[1] */
    if ((sr & SPI_SR_RXNE_Msk) && (cr2 & SPI_CR2_RXNEIE_Msk))
    {
        /* ISR context: read data, notify via callback */
        (void)SPI_LL_ReadData(SPIx);   /* clears RXNE */
        if (g_spi_txrx_cb != NULL)
        {
            g_spi_txrx_cb(instance, SPI_OK);
        }
    }

    /* TXE — ir/spi_ir_summary.json interrupts[].sources[0] */
    if ((sr & SPI_SR_TXE_Msk) && (cr2 & SPI_CR2_TXEIE_Msk))
    {
        /* Disable TXE interrupt to prevent re-entry */
        SPI_LL_DisableIT_TXE(SPIx);
        if (g_spi_txrx_cb != NULL)
        {
            g_spi_txrx_cb(instance, SPI_OK);
        }
    }
}
