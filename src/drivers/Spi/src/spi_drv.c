/**
 * @file    spi_drv.c
 * @brief   STM32F103C8T6 SPI 驱动逻辑实现层
 *
 * @note    实现状态机控制、数据流处理、等待超时机制。
 *          禁止直接访问寄存器（规则 R8-3）；所有硬件操作通过 spi_ll.h 封装。
 *          禁止裸基础类型，必须用 uint32_t/uint16_t 等定长类型（规则 R8-4）。
 *          禁止空循环延时（规则 R8-5）。
 *          全局变量加 g_ 前缀，静态模块变量加 s_ 前缀。
 *
 * Source:  RM0008 Rev 14, §25.3 SPI functional description
 */

#include "spi_api.h"
#include "spi_ll.h"

#include <stddef.h>
#include <stdbool.h>

/*===========================================================================*/
/* Private types                                                              */
/*===========================================================================*/

/** Internal transfer control block for interrupt-driven transfers. */
typedef struct
{
    const uint8_t      *pTxBuf;     /* Transmit buffer pointer (NULL = dummy 0xFF) */
    uint8_t            *pRxBuf;     /* Receive buffer pointer (NULL = discard)     */
    uint32_t            txCount;    /* Bytes already transmitted                   */
    uint32_t            rxCount;    /* Bytes already received                      */
    uint32_t            length;     /* Total transfer length                       */
    Spi_IsrCallback_t   callback;   /* Completion/error callback (may be NULL)     */
} Spi_TransferCtrl_t;

/** Per-instance runtime state. */
typedef struct
{
    bool                 isInit;    /* Module initialised flag                     */
    Spi_State_e          state;     /* Current driver state                        */
    Spi_CommMode_e       commMode;  /* Active communication mode (for disable)     */
    Spi_TransferCtrl_t   xfer;      /* Active transfer control block               */
} Spi_InstanceCtrl_t;

/*===========================================================================*/
/* Private storage (file-scope static — never exposed via extern)            */
/*===========================================================================*/

static Spi_InstanceCtrl_t s_ctrl[SPI_INSTANCE_COUNT];

/*===========================================================================*/
/* Private helper: check for SR error flags and update state                 */
/*===========================================================================*/

/**
 * @brief  Inspect SR error flags and return the corresponding error code.
 *         Clears flags using correct sequences (no side-effect leaks to caller).
 *         Source: IR errors[], RM0008 §25.3.10 p.693-694
 *
 * @param[in]  SPIx  SPI peripheral pointer.
 * @param[in]  sr    Current SR snapshot.
 * @return     SPI_OK if no error flags set, error code otherwise.
 */
static Spi_ReturnType Spi_Drv_CheckErrors(SPI_TypeDef *SPIx, uint32_t sr)
{
    Spi_ReturnType result = SPI_OK;

    if ((sr & SPI_SR_MODF_Msk) != 0U)
    {
        /* MODF: RC_SEQ clear — read SR then write CR1 — RM0008 §25.3.10 p.693 */
        uint32_t cr1 = SPI_LL_ReadCR1(SPIx);
        SPI_LL_ClearMODF(SPIx, cr1);
        result = SPI_ERR_MODE_FAULT;
    }
    else if ((sr & SPI_SR_OVR_Msk) != 0U)
    {
        /* OVR: RC_SEQ clear — read DR then read SR — RM0008 §25.3.10 p.694 */
        SPI_LL_ClearOVR(SPIx);
        result = SPI_ERR_OVERRUN;
    }
    else if ((sr & SPI_SR_CRCERR_Msk) != 0U)
    {
        /* CRCERR: W0C — RM0008 §25.5.3 p.717 */
        SPI_LL_ClearCRCERR(SPIx);
        result = SPI_ERR_CRC;
    }
    else
    {
        /* No error */
    }

    return result;
}

/*===========================================================================*/
/* Spi_Init                                                                   */
/*===========================================================================*/

Spi_ReturnType Spi_Init(const Spi_ConfigType *pConfig)
{
    Spi_ReturnType    result = SPI_ERR_PARAM;
    SPI_TypeDef      *pSPIx  = NULL;
    Spi_InstanceCtrl_t *pCtrl = NULL;

    /* Parameter validation */
    if ((pConfig != NULL) &&
        (pConfig->instanceIndex < SPI_INSTANCE_COUNT))
    {
        pSPIx  = SPI_LL_GetInstance(pConfig->instanceIndex);
        pCtrl  = &s_ctrl[pConfig->instanceIndex];

        if (pSPIx != NULL)
        {
            /* Step 1: Enable peripheral clock — RM0008 §7.3.7/§7.3.8 */
            SPI_LL_EnableClock(pSPIx);

            /* Step 2: Guard SPE=0 before configuration (INV_SPI_002) */
            /* Guard: INV_SPI_002 */
            SPI_LL_Disable(pSPIx);

            /* Steps 3-9: Compose and write CR1 (SPE intentionally 0) */
            SPI_LL_ComposeCR1(pSPIx, pConfig);

            /* Step 10: Compose and write CR2 (ISR/DMA enables off by default) */
            SPI_LL_ComposeCR2(pSPIx, pConfig);

            /* CRC polynomial (must be written while SPE=0) */
            /* Guard: INV_SPI_002 */
            SPI_LL_SetCRCPolynomial(pSPIx, pConfig->crcPolynomial);

            /* Step 11: Enable SPI — BR/CPOL/CPHA/DFF/MSTR now locked */
            SPI_LL_Enable(pSPIx); /* RM0008 §25.3.3 p.680 */

            /* Update driver state */
            pCtrl->isInit         = true;
            pCtrl->state          = SPI_STATE_IDLE;
            pCtrl->commMode       = pConfig->commMode;
            pCtrl->xfer.pTxBuf    = NULL;
            pCtrl->xfer.pRxBuf    = NULL;
            pCtrl->xfer.txCount   = 0U;
            pCtrl->xfer.rxCount   = 0U;
            pCtrl->xfer.length    = 0U;
            pCtrl->xfer.callback  = NULL;

            result = SPI_OK;
        }
    }

    return result;
}

/*===========================================================================*/
/* Spi_DeInit                                                                 */
/*===========================================================================*/

Spi_ReturnType Spi_DeInit(Spi_InstanceIndex_e instanceIndex)
{
    Spi_ReturnType      result = SPI_ERR_NOT_INIT;
    SPI_TypeDef        *pSPIx  = NULL;
    Spi_InstanceCtrl_t *pCtrl  = NULL;
    uint32_t            loopCnt;

    if (instanceIndex < SPI_INSTANCE_COUNT)
    {
        pCtrl = &s_ctrl[instanceIndex];

        if (pCtrl->isInit)
        {
            pSPIx  = SPI_LL_GetInstance(instanceIndex);
            result = SPI_OK;

            /* Disable interrupts first */
            SPI_LL_DisableTXEIE(pSPIx);
            SPI_LL_DisableRXNEIE(pSPIx);
            SPI_LL_DisableERRIE(pSPIx);

            /*
             * Mode-specific disable procedures from IR disable_procedures
             * Source: RM0008 §25.3.8 p.691
             */
            if (pCtrl->commMode == SPI_COMM_BIDI_RX)
            {
                /* bidirectional_rx: clear BIDIOE first to stop clock/data */
                /* Step 1: Clear BIDIOE — RM0008 §25.3.8 p.691 */
                SPI_LL_SetCommMode(pSPIx, SPI_COMM_BIDI_RX);

                /* Step 2: Wait BSY=0 */
                loopCnt = SPI_CFG_TIMEOUT_LOOPS;
                while (SPI_LL_IsBusy(pSPIx) && (loopCnt > 0U))
                {
                    loopCnt--;
                }
                if (loopCnt == 0U)
                {
                    result = SPI_ERR_TIMEOUT;
                }

                /* Step 3: Clear SPE */
                SPI_LL_Disable(pSPIx);

                /* Step 4: Drain RX */
                if (SPI_LL_IsRXNE(pSPIx))
                {
                    (void)SPI_LL_ReadDR8(pSPIx);
                }
            }
            else if (pCtrl->commMode == SPI_COMM_RECEIVE_ONLY)
            {
                /* master_receive_only: clear SPE first to stop clock */
                /* Step 1: Clear SPE immediately — RM0008 §25.3.8 p.691 */
                SPI_LL_Disable(pSPIx);

                /* Step 2: Wait BSY=0 */
                loopCnt = SPI_CFG_TIMEOUT_LOOPS;
                while (SPI_LL_IsBusy(pSPIx) && (loopCnt > 0U))
                {
                    loopCnt--;
                }
                if (loopCnt == 0U)
                {
                    result = SPI_ERR_TIMEOUT;
                }

                /* Step 3: Read remaining data */
                if (SPI_LL_IsRXNE(pSPIx))
                {
                    (void)SPI_LL_ReadDR8(pSPIx);
                }
            }
            else
            {
                /* full_duplex / bidi_tx: standard disable — RM0008 §25.3.8 p.691 */

                /* Step 1: Wait RXNE=1, read last received frame — RM0008 §25.3.8 p.691 */
                loopCnt = SPI_CFG_TIMEOUT_LOOPS;
                while ((!SPI_LL_IsRXNE(pSPIx)) && (loopCnt > 0U))
                {
                    loopCnt--;
                }
                if (SPI_LL_IsRXNE(pSPIx))
                {
                    (void)SPI_LL_ReadDR8(pSPIx); /* drain last RX frame */
                }

                /* Step 2: Wait TXE=1 — RM0008 §25.3.8 p.691 */
                loopCnt = SPI_CFG_TIMEOUT_LOOPS;
                while ((!SPI_LL_IsTXE(pSPIx)) && (loopCnt > 0U))
                {
                    loopCnt--;
                }
                if (loopCnt == 0U)
                {
                    result = SPI_ERR_TIMEOUT;
                }

                /* Step 3: Wait BSY=0 (Guard: INV_SPI_003) — RM0008 §25.3.8 p.691 */
                if (result == SPI_OK)
                {
                    /* Guard: INV_SPI_003 */
                    loopCnt = SPI_CFG_TIMEOUT_LOOPS;
                    while (SPI_LL_IsBusy(pSPIx) && (loopCnt > 0U))
                    {
                        loopCnt--;
                    }
                    if (loopCnt == 0U)
                    {
                        result = SPI_ERR_TIMEOUT;
                    }
                }

                /* Step 4: Disable SPI peripheral — RM0008 §25.3.8 p.691 */
                SPI_LL_Disable(pSPIx);
            }

            /* Step 5: Disable peripheral clock — RM0008 §7.3.7/§7.3.8 */
            SPI_LL_DisableClock(pSPIx);

            /* Reset driver state regardless of timeout */
            pCtrl->isInit  = false;
            pCtrl->state   = SPI_STATE_UNINIT;
        }
    }

    return result;
}

/*===========================================================================*/
/* Spi_TransferBlocking                                                       */
/*===========================================================================*/

Spi_ReturnType Spi_TransferBlocking(Spi_InstanceIndex_e  instanceIndex,
                                    const uint8_t       *pTxBuf,
                                    uint8_t             *pRxBuf,
                                    uint32_t             length)
{
    Spi_ReturnType      result  = SPI_ERR_PARAM;
    SPI_TypeDef        *pSPIx   = NULL;
    Spi_InstanceCtrl_t *pCtrl   = NULL;
    uint32_t            txIdx   = 0U;
    uint32_t            rxIdx   = 0U;
    uint32_t            loopCnt;
    uint32_t            sr;

    if ((instanceIndex >= SPI_INSTANCE_COUNT) || (length == 0U))
    {
        return SPI_ERR_PARAM;
    }

    pCtrl = &s_ctrl[instanceIndex];

    if (!pCtrl->isInit)
    {
        return SPI_ERR_NOT_INIT;
    }

    if (pCtrl->state != SPI_STATE_IDLE)
    {
        return SPI_ERR_BUSY;
    }

    pSPIx        = SPI_LL_GetInstance(instanceIndex);
    pCtrl->state = SPI_STATE_BUSY_TRX;
    result       = SPI_OK;

    while ((txIdx < length) || (rxIdx < length))
    {
        /* Transmit path: wait TXE, write TX data */
        if (txIdx < length)
        {
            loopCnt = SPI_CFG_TIMEOUT_LOOPS;
            while ((!SPI_LL_IsTXE(pSPIx)) && (loopCnt > 0U))
            {
                loopCnt--;
            }
            if (loopCnt == 0U)
            {
                result = SPI_ERR_TIMEOUT;
                break;
            }

            /* Check for errors before writing */
            sr = SPI_LL_ReadSR(pSPIx);
            result = Spi_Drv_CheckErrors(pSPIx, sr);
            if (result != SPI_OK)
            {
                break;
            }

            if (pTxBuf != NULL)
            {
                SPI_LL_WriteDR8(pSPIx, pTxBuf[txIdx]); /* RM0008 §25.5.4 p.718 */
            }
            else
            {
                SPI_LL_WriteDR8(pSPIx, (uint8_t)0xFFU); /* dummy byte */
            }
            txIdx++;
        }

        /* Receive path: wait RXNE, read RX data */
        if (rxIdx < length)
        {
            loopCnt = SPI_CFG_TIMEOUT_LOOPS;
            while ((!SPI_LL_IsRXNE(pSPIx)) && (loopCnt > 0U))
            {
                loopCnt--;
            }
            if (loopCnt == 0U)
            {
                result = SPI_ERR_TIMEOUT;
                break;
            }

            if (pRxBuf != NULL)
            {
                pRxBuf[rxIdx] = SPI_LL_ReadDR8(pSPIx); /* RM0008 §25.5.4 p.718 */
            }
            else
            {
                (void)SPI_LL_ReadDR8(pSPIx); /* discard */
            }
            rxIdx++;
        }
    }

    pCtrl->state = SPI_STATE_IDLE;

    return result;
}

/*===========================================================================*/
/* Spi_TransmitBlocking                                                       */
/*===========================================================================*/

Spi_ReturnType Spi_TransmitBlocking(Spi_InstanceIndex_e  instanceIndex,
                                    const uint8_t       *pTxBuf,
                                    uint32_t             length)
{
    Spi_ReturnType result = SPI_ERR_PARAM;

    if (pTxBuf != NULL)
    {
        /* Full duplex: transmit pTxBuf, discard RX */
        result = Spi_TransferBlocking(instanceIndex, pTxBuf, NULL, length);
    }

    return result;
}

/*===========================================================================*/
/* Spi_ReceiveBlocking                                                        */
/*===========================================================================*/

Spi_ReturnType Spi_ReceiveBlocking(Spi_InstanceIndex_e  instanceIndex,
                                   uint8_t             *pRxBuf,
                                   uint32_t             length)
{
    Spi_ReturnType result = SPI_ERR_PARAM;

    if (pRxBuf != NULL)
    {
        /* Full duplex: transmit dummy 0xFF, capture RX */
        result = Spi_TransferBlocking(instanceIndex, NULL, pRxBuf, length);
    }

    return result;
}

/*===========================================================================*/
/* Spi_TransferIT                                                              */
/*===========================================================================*/

Spi_ReturnType Spi_TransferIT(Spi_InstanceIndex_e  instanceIndex,
                               const uint8_t       *pTxBuf,
                               uint8_t             *pRxBuf,
                               uint32_t             length)
{
    Spi_ReturnType      result = SPI_ERR_PARAM;
    SPI_TypeDef        *pSPIx  = NULL;
    Spi_InstanceCtrl_t *pCtrl  = NULL;

    if ((instanceIndex >= SPI_INSTANCE_COUNT) || (length == 0U))
    {
        return SPI_ERR_PARAM;
    }

    pCtrl = &s_ctrl[instanceIndex];

    if (!pCtrl->isInit)
    {
        return SPI_ERR_NOT_INIT;
    }

    if (pCtrl->state != SPI_STATE_IDLE)
    {
        return SPI_ERR_BUSY;
    }

    pSPIx = SPI_LL_GetInstance(instanceIndex);

    /* Set up transfer control block */
    pCtrl->xfer.pTxBuf  = pTxBuf;
    pCtrl->xfer.pRxBuf  = pRxBuf;
    pCtrl->xfer.txCount = 0U;
    pCtrl->xfer.rxCount = 0U;
    pCtrl->xfer.length  = length;
    pCtrl->state        = SPI_STATE_BUSY_TRX;

    /* Enable RXNE and error interrupts; TXEIE enables TX path */
    SPI_LL_EnableERRIE(pSPIx);   /* RM0008 §25.5.2 p.716 */
    SPI_LL_EnableRXNEIE(pSPIx);  /* RM0008 §25.5.2 p.716 */
    SPI_LL_EnableTXEIE(pSPIx);   /* RM0008 §25.5.2 p.716 — kicks off first TX */

    result = SPI_OK;

    return result;
}

/*===========================================================================*/
/* Spi_RegisterCallback                                                       */
/*===========================================================================*/

Spi_ReturnType Spi_RegisterCallback(Spi_InstanceIndex_e instanceIndex,
                                    Spi_IsrCallback_t   callback)
{
    Spi_ReturnType result = SPI_ERR_PARAM;

    if (instanceIndex < SPI_INSTANCE_COUNT)
    {
        /* NULL is a valid value (deregister) */
        s_ctrl[instanceIndex].xfer.callback = callback;
        result = SPI_OK;
    }

    return result;
}

/*===========================================================================*/
/* Spi_GetStatus                                                              */
/*===========================================================================*/

Spi_ReturnType Spi_GetStatus(Spi_InstanceIndex_e  instanceIndex,
                              Spi_StatusType      *pStatus)
{
    Spi_ReturnType      result = SPI_ERR_PARAM;
    SPI_TypeDef        *pSPIx  = NULL;
    Spi_InstanceCtrl_t *pCtrl  = NULL;
    uint32_t            sr;

    if ((instanceIndex < SPI_INSTANCE_COUNT) && (pStatus != NULL))
    {
        pCtrl  = &s_ctrl[instanceIndex];
        pSPIx  = SPI_LL_GetInstance(instanceIndex);

        pStatus->state = pCtrl->state;

        sr = SPI_LL_ReadSR(pSPIx);
        pStatus->isBusy     = ((sr & SPI_SR_BSY_Msk)    != 0U);
        pStatus->txEmpty    = ((sr & SPI_SR_TXE_Msk)    != 0U);
        pStatus->rxNotEmpty = ((sr & SPI_SR_RXNE_Msk)   != 0U);
        pStatus->overrun    = ((sr & SPI_SR_OVR_Msk)    != 0U);
        pStatus->modeFault  = ((sr & SPI_SR_MODF_Msk)   != 0U);
        pStatus->crcError   = ((sr & SPI_SR_CRCERR_Msk) != 0U);

        result = SPI_OK;
    }

    return result;
}

/*===========================================================================*/
/* Spi_IRQHandler — common interrupt handler (called from spi_isr.c)         */
/*===========================================================================*/

void Spi_IRQHandler(Spi_InstanceIndex_e instanceIndex)
{
    SPI_TypeDef        *pSPIx  = NULL;
    Spi_InstanceCtrl_t *pCtrl  = NULL;
    Spi_TransferCtrl_t *pXfer  = NULL;
    uint32_t            sr;
    Spi_ReturnType      xferResult = SPI_OK;

    if (instanceIndex >= SPI_INSTANCE_COUNT)
    {
        return;
    }

    pCtrl = &s_ctrl[instanceIndex];
    pXfer = &pCtrl->xfer;
    pSPIx = SPI_LL_GetInstance(instanceIndex);

    /* Step 1: Read SR snapshot (do not call helpers that consume flags yet) */
    sr = SPI_LL_ReadSR(pSPIx); /* RM0008 §25.5.3 p.717 */

    /* Step 2: Handle error flags first — clears them via correct sequences */
    if ((sr & (SPI_SR_MODF_Msk | SPI_SR_OVR_Msk | SPI_SR_CRCERR_Msk)) != 0U)
    {
        xferResult = Spi_Drv_CheckErrors(pSPIx, sr);
        pCtrl->state = SPI_STATE_ERROR;

        /* Disable all interrupts for this instance */
        SPI_LL_DisableTXEIE(pSPIx);
        SPI_LL_DisableRXNEIE(pSPIx);
        SPI_LL_DisableERRIE(pSPIx);

        /* Invoke callback with error */
        if (pXfer->callback != NULL)
        {
            pXfer->callback(instanceIndex, xferResult);
        }
        return;
    }

    /* Step 3: RXNE — read received byte */
    if ((sr & SPI_SR_RXNE_Msk) != 0U)
    {
        if (pXfer->pRxBuf != NULL)
        {
            pXfer->pRxBuf[pXfer->rxCount] = SPI_LL_ReadDR8(pSPIx); /* RM0008 §25.5.4 p.718 */
        }
        else
        {
            (void)SPI_LL_ReadDR8(pSPIx); /* discard */
        }
        pXfer->rxCount++;
    }

    /* Step 4: TXE — write next transmit byte */
    if ((sr & SPI_SR_TXE_Msk) != 0U)
    {
        if (pXfer->txCount < pXfer->length)
        {
            if (pXfer->pTxBuf != NULL)
            {
                SPI_LL_WriteDR8(pSPIx, pXfer->pTxBuf[pXfer->txCount]); /* RM0008 §25.5.4 p.718 */
            }
            else
            {
                SPI_LL_WriteDR8(pSPIx, (uint8_t)0xFFU); /* dummy */
            }
            pXfer->txCount++;
        }
        else
        {
            /* All bytes sent — disable TXEIE to prevent re-entry */
            SPI_LL_DisableTXEIE(pSPIx); /* RM0008 §25.5.2 p.716 */
        }
    }

    /* Step 5: Check for transfer completion (all bytes received) */
    if (pXfer->rxCount >= pXfer->length)
    {
        /* Disable remaining interrupts */
        SPI_LL_DisableTXEIE(pSPIx);
        SPI_LL_DisableRXNEIE(pSPIx);
        SPI_LL_DisableERRIE(pSPIx);

        pCtrl->state = SPI_STATE_IDLE;

        /* Notify caller */
        if (pXfer->callback != NULL)
        {
            pXfer->callback(instanceIndex, SPI_OK);
        }
    }
}
