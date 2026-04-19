/**
 * @file    can_drv.c
 * @brief   STM32F103C8T6 CAN 驱动逻辑实现层
 *
 * @note    实现状态机控制、TX/RX 数据流、错误处理、中断分发。
 *          禁止直接访问寄存器（规则 R8-3）；所有硬件操作通过 can_ll.h 封装。
 *          禁止裸基础类型，必须用 uint32_t/uint8_t 等定长类型（规则 R8-4）。
 *          全局变量加 g_ 前缀，静态模块变量加 s_ 前缀。
 *
 * Source:  RM0008 Rev 14, Chapter 24 - bxCAN
 *          ir/can_ir_summary.json v2.0
 */

#include "can_api.h"
#include "can_ll.h"

#include <stddef.h>
#include <stdbool.h>

/*===========================================================================*/
/* Private storage (file-scope static)                                       */
/*===========================================================================*/

static bool            s_isInit   = false;
static Can_State_e     s_state    = CAN_STATE_UNINIT;

static Can_TxCallback_t    s_txCallback    = NULL;
static Can_RxCallback_t    s_rxCallback    = NULL;
static Can_ErrorCallback_t s_errorCallback = NULL;

/*===========================================================================*/
/* Can_Init                                                                   */
/* Source: ir/can_ir_summary.json configuration_strategies[0] INIT_BASIC     */
/*===========================================================================*/

Can_ReturnType Can_Init(const Can_ConfigType *pConfig)
{
    Can_ReturnType   result  = CAN_ERR_PARAM;
    CAN_TypeDef     *pCANx   = CAN1;
    uint32_t         loopCnt;
    Can_FilterConfigType defaultFilter;

    if (pConfig == NULL)
    {
        return CAN_ERR_PARAM;
    }

    /* Validate bit timing parameters */
    if ((pConfig->bitTiming.prescaler == 0U) ||
        (pConfig->bitTiming.prescaler > 1024U) ||
        (pConfig->bitTiming.timeSeg1 == 0U) ||
        (pConfig->bitTiming.timeSeg1 > 16U) ||
        (pConfig->bitTiming.timeSeg2 == 0U) ||
        (pConfig->bitTiming.timeSeg2 > 8U) ||
        (pConfig->bitTiming.syncJumpW == 0U) ||
        (pConfig->bitTiming.syncJumpW > 4U))
    {
        return CAN_ERR_PARAM;
    }

    /* Step 1: Configure GPIO pin remap if needed */
    /* Source: ir/can_ir_summary.json configuration_strategies[4] PIN_REMAP */
    if (pConfig->pinRemap != CAN_REMAP_NONE)
    {
        CAN_LL_SetPinRemap(pConfig->pinRemap);
    }

    /* Step 2: Enable CAN1 clock — ir/can_ir_summary.json clock[0] */
    CAN_LL_EnableClock(pCANx);

    /* Step 3: Request initialization mode */
    /* SEQ_CAN_INIT_MODE — ir/can_ir_summary.json atomic_sequences[0] */
    CAN_LL_RequestInitMode(pCANx);

    /* Step 4: Wait for initialization acknowledgment (INAK=1) */
    loopCnt = CAN_CFG_TIMEOUT_LOOPS;
    while ((!CAN_LL_IsInitAck(pCANx)) && (loopCnt > 0U))
    {
        loopCnt--;
    }
    if (loopCnt == 0U)
    {
        return CAN_ERR_TIMEOUT;
    }

    /* Step 5: Configure MCR operational bits */
    /* Guard: INV_CAN_001 — in init mode, BTR and MCR config bits writable */
    CAN_LL_ComposeMCR(pCANx, pConfig);

    /* Step 6: Configure BTR with baud rate and operation mode */
    /* Guard: INV_CAN_001 — BTR writable only when INAK=1 */
    CAN_LL_ComposeBTR(pCANx, pConfig);

    /* Step 7: Exit initialization mode */
    /* SEQ_CAN_EXIT_INIT_MODE — ir/can_ir_summary.json atomic_sequences[1] */
    CAN_LL_RequestNormalMode(pCANx);

    /* Step 8: Wait for normal operation (INAK=0) */
    /* INV_CAN_002: INAK=0 && SLAK=0 implies CAN_READY */
    loopCnt = CAN_CFG_TIMEOUT_LOOPS;
    while ((CAN_LL_IsInitAck(pCANx)) && (loopCnt > 0U))
    {
        loopCnt--;
    }
    if (loopCnt == 0U)
    {
        return CAN_ERR_TIMEOUT;
    }

    /* Step 9: Configure default accept-all filter (filter 0, 32-bit mask mode) */
    /* INV_CAN_004: at least one filter must be activated for reception */
    defaultFilter.filterNumber = 0U;
    defaultFilter.mode         = CAN_FILTER_MASK;
    defaultFilter.scale        = CAN_FILTER_SCALE_32BIT;
    defaultFilter.fifoAssign   = CAN_FILTER_FIFO0;
    defaultFilter.enable       = true;
    defaultFilter.idHigh       = 0U;
    defaultFilter.idLow        = 0U;
    defaultFilter.maskIdHigh   = 0U;  /* Mask=0 accepts all IDs */
    defaultFilter.maskIdLow    = 0U;

    CAN_LL_EnterFilterInit(pCANx);
    CAN_LL_ConfigureFilter(pCANx, &defaultFilter);
    CAN_LL_ExitFilterInit(pCANx);

    /* Step 10: Enable interrupts if configured */
#if (CAN_CFG_IRQ_ENABLE == 1U)
    CAN_LL_EnableTMEIE(pCANx);   /* TX mailbox empty — RM0008 §24.5 */
    CAN_LL_EnableFMPIE0(pCANx);  /* FIFO 0 message pending */
    CAN_LL_EnableFMPIE1(pCANx);  /* FIFO 1 message pending */
    CAN_LL_EnableERRIE(pCANx);   /* Error interrupt gate */
    CAN_LL_EnableBOFIE(pCANx);   /* Bus-off interrupt */
    CAN_LL_EnableEPVIE(pCANx);   /* Error passive interrupt */
    CAN_LL_EnableEWGIE(pCANx);   /* Error warning interrupt */
    CAN_LL_EnableLECIE(pCANx);   /* Last error code interrupt */
#endif

    /* Update driver state */
    s_isInit = true;
    s_state  = CAN_STATE_READY;
    result   = CAN_OK;

    return result;
}

/*===========================================================================*/
/* Can_DeInit                                                                 */
/*===========================================================================*/

Can_ReturnType Can_DeInit(void)
{
    CAN_TypeDef *pCANx = CAN1;

    if (!s_isInit)
    {
        return CAN_ERR_NOT_INIT;
    }

    /* Disable all interrupts */
    CAN_LL_DisableAllInterrupts(pCANx);

    /* Reset peripheral via RCC */
    CAN_LL_ResetPeripheral(pCANx);

    /* Disable clock */
    CAN_LL_DisableClock(pCANx);

    /* Reset driver state */
    s_isInit       = false;
    s_state        = CAN_STATE_UNINIT;
    s_txCallback   = NULL;
    s_rxCallback   = NULL;
    s_errorCallback = NULL;

    return CAN_OK;
}

/*===========================================================================*/
/* Can_ConfigFilter                                                           */
/*===========================================================================*/

Can_ReturnType Can_ConfigFilter(const Can_FilterConfigType *pFilter)
{
    CAN_TypeDef *pCANx = CAN1;

    if (pFilter == NULL)
    {
        return CAN_ERR_PARAM;
    }

    if (pFilter->filterNumber >= CAN_FILTER_BANK_COUNT)
    {
        return CAN_ERR_PARAM;
    }

    if (!s_isInit)
    {
        return CAN_ERR_NOT_INIT;
    }

    /* Guard: INV_CAN_003 — enter filter init mode */
    CAN_LL_EnterFilterInit(pCANx);
    CAN_LL_ConfigureFilter(pCANx, pFilter);
    CAN_LL_ExitFilterInit(pCANx);

    return CAN_OK;
}

/*===========================================================================*/
/* Can_Transmit                                                               */
/*===========================================================================*/

Can_TxResultType Can_Transmit(const Can_MessageType *pMsg)
{
    Can_TxResultType txResult;
    CAN_TypeDef     *pCANx = CAN1;
    uint8_t          mailbox;

    txResult.status  = CAN_ERR_PARAM;
    txResult.mailbox = CAN_TX_MAILBOX_NONE;

    if (pMsg == NULL)
    {
        return txResult;
    }

    if (pMsg->dlc > 8U)
    {
        return txResult;
    }

    if (!s_isInit)
    {
        txResult.status = CAN_ERR_NOT_INIT;
        return txResult;
    }

    /* INV_CAN_006: check TME to find an empty mailbox */
    mailbox = CAN_LL_GetFreeMailbox(pCANx);

    if (mailbox == CAN_TX_MAILBOX_NONE)
    {
        txResult.status = CAN_ERR_NO_MAILBOX;
        return txResult;
    }

    /* Write message to mailbox and request transmission */
    CAN_LL_WriteMailbox(pCANx, mailbox, pMsg);

    txResult.status  = CAN_OK;
    txResult.mailbox = mailbox;

    return txResult;
}

/*===========================================================================*/
/* Can_AbortTransmit                                                          */
/*===========================================================================*/

Can_ReturnType Can_AbortTransmit(uint8_t mailbox)
{
    CAN_TypeDef *pCANx = CAN1;

    if (mailbox >= CAN_TX_MAILBOX_COUNT)
    {
        return CAN_ERR_PARAM;
    }

    CAN_LL_AbortMailbox(pCANx, mailbox);

    return CAN_OK;
}

/*===========================================================================*/
/* Can_IsTxComplete                                                           */
/*===========================================================================*/

bool Can_IsTxComplete(uint8_t mailbox)
{
    if (mailbox >= CAN_TX_MAILBOX_COUNT)
    {
        return false;
    }

    return CAN_LL_IsRequestComplete(CAN1, mailbox);
}

/*===========================================================================*/
/* Can_Receive                                                                */
/*===========================================================================*/

Can_ReturnType Can_Receive(uint8_t fifoIndex, Can_MessageType *pMsg)
{
    CAN_TypeDef *pCANx = CAN1;

    if ((pMsg == NULL) || (fifoIndex >= CAN_RX_FIFO_COUNT))
    {
        return CAN_ERR_PARAM;
    }

    if (!s_isInit)
    {
        return CAN_ERR_NOT_INIT;
    }

    /* INV_CAN_007: check FMP>0 before reading */
    if (CAN_LL_GetFifoPending(pCANx, fifoIndex) == 0U)
    {
        return CAN_ERR_NO_MESSAGE;
    }

    /* Read message from FIFO mailbox */
    CAN_LL_ReadFifoMailbox(pCANx, fifoIndex, pMsg);

    /* Release FIFO output mailbox */
    CAN_LL_ReleaseFifo(pCANx, fifoIndex);

    return CAN_OK;
}

/*===========================================================================*/
/* Can_GetRxPending                                                           */
/*===========================================================================*/

uint8_t Can_GetRxPending(uint8_t fifoIndex)
{
    if (fifoIndex >= CAN_RX_FIFO_COUNT)
    {
        return 0U;
    }

    return CAN_LL_GetFifoPending(CAN1, fifoIndex);
}

/*===========================================================================*/
/* Can_GetErrorStatus                                                         */
/*===========================================================================*/

Can_ReturnType Can_GetErrorStatus(Can_ErrorStatusType *pStatus)
{
    CAN_TypeDef *pCANx = CAN1;

    if (pStatus == NULL)
    {
        return CAN_ERR_PARAM;
    }

    pStatus->state        = s_state;
    pStatus->errorWarning = CAN_LL_IsErrorWarning(pCANx);
    pStatus->errorPassive = CAN_LL_IsErrorPassive(pCANx);
    pStatus->busOff       = CAN_LL_IsBusOff(pCANx);
    pStatus->lastErrorCode = CAN_LL_GetLastErrorCode(pCANx);
    pStatus->txErrorCount = CAN_LL_GetTEC(pCANx);
    pStatus->rxErrorCount = CAN_LL_GetREC(pCANx);

    return CAN_OK;
}

/*===========================================================================*/
/* Can_GetState                                                               */
/*===========================================================================*/

Can_State_e Can_GetState(void)
{
    return s_state;
}

/*===========================================================================*/
/* Can_Sleep                                                                  */
/*===========================================================================*/

Can_ReturnType Can_Sleep(void)
{
    CAN_TypeDef *pCANx   = CAN1;
    uint32_t     loopCnt;

    if (!s_isInit)
    {
        return CAN_ERR_NOT_INIT;
    }

    CAN_LL_RequestSleepMode(pCANx);

    loopCnt = CAN_CFG_TIMEOUT_LOOPS;
    while ((!CAN_LL_IsSleepAck(pCANx)) && (loopCnt > 0U))
    {
        loopCnt--;
    }
    if (loopCnt == 0U)
    {
        return CAN_ERR_TIMEOUT;
    }

    s_state = CAN_STATE_SLEEP;
    return CAN_OK;
}

/*===========================================================================*/
/* Can_Wakeup                                                                 */
/*===========================================================================*/

Can_ReturnType Can_Wakeup(void)
{
    CAN_TypeDef *pCANx   = CAN1;
    uint32_t     loopCnt;

    if (!s_isInit)
    {
        return CAN_ERR_NOT_INIT;
    }

    CAN_LL_RequestWakeup(pCANx);

    loopCnt = CAN_CFG_TIMEOUT_LOOPS;
    while ((CAN_LL_IsSleepAck(pCANx)) && (loopCnt > 0U))
    {
        loopCnt--;
    }
    if (loopCnt == 0U)
    {
        return CAN_ERR_TIMEOUT;
    }

    /* Clear WKUI flag */
    CAN_LL_ClearWKUI(pCANx);

    s_state = CAN_STATE_READY;
    return CAN_OK;
}

/*===========================================================================*/
/* Callback registration                                                      */
/*===========================================================================*/

Can_ReturnType Can_RegisterTxCallback(Can_TxCallback_t callback)
{
    s_txCallback = callback;
    return CAN_OK;
}

Can_ReturnType Can_RegisterRxCallback(Can_RxCallback_t callback)
{
    s_rxCallback = callback;
    return CAN_OK;
}

Can_ReturnType Can_RegisterErrorCallback(Can_ErrorCallback_t callback)
{
    s_errorCallback = callback;
    return CAN_OK;
}

/*===========================================================================*/
/* Can_TxIRQHandler — TX interrupt handler                                   */
/* Source: ir/can_ir_summary.json interrupts[0] CAN1_TX                      */
/*===========================================================================*/

void Can_TxIRQHandler(void)
{
    CAN_TypeDef    *pCANx = CAN1;
    uint32_t        tsr;
    uint8_t         mbIdx;
    Can_ReturnType  txResult;

    tsr = CAN_LL_ReadTSR(pCANx); /* RM0008 §24.4.2 */

    for (mbIdx = 0U; mbIdx < CAN_TX_MAILBOX_COUNT; mbIdx++)
    {
        uint32_t rqcpMsk = (CAN_TSR_RQCP0_Msk << ((uint32_t)mbIdx * 8U));

        if ((tsr & rqcpMsk) != 0U)
        {
            /* Determine TX result for this mailbox */
            if (CAN_LL_IsTxOk(pCANx, mbIdx))
            {
                txResult = CAN_OK;
            }
            else
            {
                /* Check arbitration lost or transmission error */
                uint32_t alstMsk = (CAN_TSR_ALST0_Msk << ((uint32_t)mbIdx * 8U));
                uint32_t terrMsk = (CAN_TSR_TERR0_Msk << ((uint32_t)mbIdx * 8U));

                if ((tsr & terrMsk) != 0U)
                {
                    txResult = CAN_ERR_ACK; /* Generic TX error */
                }
                else if ((tsr & alstMsk) != 0U)
                {
                    txResult = CAN_ERR_BUSY; /* Arbitration lost */
                }
                else
                {
                    txResult = CAN_ERR_ACK;
                }
            }

            /* Clear RQCP flag — W1C, direct write */
            CAN_LL_ClearRQCP(pCANx, mbIdx);

            /* Invoke callback */
            if (s_txCallback != NULL)
            {
                s_txCallback(mbIdx, txResult);
            }
        }
    }
}

/*===========================================================================*/
/* Can_Rx0IRQHandler — RX FIFO 0 interrupt handler                           */
/* Source: ir/can_ir_summary.json interrupts[1] CAN1_RX0                     */
/*===========================================================================*/

void Can_Rx0IRQHandler(void)
{
    CAN_TypeDef    *pCANx = CAN1;
    Can_MessageType rxMsg;

    /* Handle FIFO overrun first — W1C clear */
    if (CAN_LL_IsFifoOverrun(pCANx, CAN_RX_FIFO_0))
    {
        CAN_LL_ClearFifoOverrun(pCANx, CAN_RX_FIFO_0);
    }

    /* Handle FIFO full — W1C clear */
    if (CAN_LL_IsFifoFull(pCANx, CAN_RX_FIFO_0))
    {
        CAN_LL_ClearFifoFull(pCANx, CAN_RX_FIFO_0);
    }

    /* Read all pending messages */
    while (CAN_LL_GetFifoPending(pCANx, CAN_RX_FIFO_0) > 0U)
    {
        CAN_LL_ReadFifoMailbox(pCANx, CAN_RX_FIFO_0, &rxMsg);
        CAN_LL_ReleaseFifo(pCANx, CAN_RX_FIFO_0);

        if (s_rxCallback != NULL)
        {
            s_rxCallback(CAN_RX_FIFO_0, &rxMsg);
        }
    }
}

/*===========================================================================*/
/* Can_Rx1IRQHandler — RX FIFO 1 interrupt handler                           */
/* Source: ir/can_ir_summary.json interrupts[2] CAN1_RX1                     */
/*===========================================================================*/

void Can_Rx1IRQHandler(void)
{
    CAN_TypeDef    *pCANx = CAN1;
    Can_MessageType rxMsg;

    /* Handle FIFO overrun first — W1C clear */
    if (CAN_LL_IsFifoOverrun(pCANx, CAN_RX_FIFO_1))
    {
        CAN_LL_ClearFifoOverrun(pCANx, CAN_RX_FIFO_1);
    }

    /* Handle FIFO full — W1C clear */
    if (CAN_LL_IsFifoFull(pCANx, CAN_RX_FIFO_1))
    {
        CAN_LL_ClearFifoFull(pCANx, CAN_RX_FIFO_1);
    }

    /* Read all pending messages */
    while (CAN_LL_GetFifoPending(pCANx, CAN_RX_FIFO_1) > 0U)
    {
        CAN_LL_ReadFifoMailbox(pCANx, CAN_RX_FIFO_1, &rxMsg);
        CAN_LL_ReleaseFifo(pCANx, CAN_RX_FIFO_1);

        if (s_rxCallback != NULL)
        {
            s_rxCallback(CAN_RX_FIFO_1, &rxMsg);
        }
    }
}

/*===========================================================================*/
/* Can_SceIRQHandler — Status Change / Error interrupt handler               */
/* Source: ir/can_ir_summary.json interrupts[3] CAN1_SCE                     */
/*===========================================================================*/

void Can_SceIRQHandler(void)
{
    CAN_TypeDef        *pCANx = CAN1;
    uint32_t            msr;
    Can_ErrorStatusType errStatus;
    Can_ReturnType      errCode = CAN_OK;

    msr = CAN_LL_ReadMSR(pCANx);

    /* Wakeup event */
    if ((msr & CAN_MSR_WKUI_Msk) != 0U)
    {
        CAN_LL_ClearWKUI(pCANx); /* W1C — RM0008 §24.4.1 */
        s_state = CAN_STATE_READY;
    }

    /* Sleep acknowledge */
    if ((msr & CAN_MSR_SLAKI_Msk) != 0U)
    {
        CAN_LL_ClearSLAKI(pCANx); /* W1C — RM0008 §24.4.1 */
        s_state = CAN_STATE_SLEEP;
    }

    /* Error interrupt */
    if ((msr & CAN_MSR_ERRI_Msk) != 0U)
    {
        /* Collect error status */
        errStatus.errorWarning  = CAN_LL_IsErrorWarning(pCANx);
        errStatus.errorPassive  = CAN_LL_IsErrorPassive(pCANx);
        errStatus.busOff        = CAN_LL_IsBusOff(pCANx);
        errStatus.lastErrorCode = CAN_LL_GetLastErrorCode(pCANx);
        errStatus.txErrorCount  = CAN_LL_GetTEC(pCANx);
        errStatus.rxErrorCount  = CAN_LL_GetREC(pCANx);
        errStatus.state         = s_state;

        /* Determine error severity */
        if (errStatus.busOff)
        {
            errCode = CAN_ERR_BUS_OFF;
            s_state = CAN_STATE_ERROR;
        }
        else if (errStatus.errorPassive)
        {
            errCode = CAN_ERR_PASSIVE;
            s_state = CAN_STATE_ERROR;
        }
        else if (errStatus.errorWarning)
        {
            errCode = CAN_ERR_WARNING;
        }
        else
        {
            /* Map LEC to error code — ir/can_ir_summary.json errors[] */
            switch (errStatus.lastErrorCode)
            {
                case 1U: errCode = CAN_ERR_STUFF;      break;
                case 2U: errCode = CAN_ERR_FORM;       break;
                case 3U: errCode = CAN_ERR_ACK;        break;
                case 4U: errCode = CAN_ERR_BIT_RECESS; break;
                case 5U: errCode = CAN_ERR_BIT_DOMIN;  break;
                case 6U: errCode = CAN_ERR_CRC;        break;
                default: errCode = CAN_OK;              break;
            }
        }

        /* Clear LEC for next detection */
        CAN_LL_ClearLEC(pCANx);

        /* Clear ERRI flag — W1C */
        CAN_LL_ClearERRI(pCANx);

        /* Invoke error callback */
        if ((s_errorCallback != NULL) && (errCode != CAN_OK))
        {
            errStatus.state = s_state;
            s_errorCallback(errCode, &errStatus);
        }
    }
}
