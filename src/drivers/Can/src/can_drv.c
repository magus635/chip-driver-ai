/**
 * @file    can_drv.c
 * @brief   STM32F103C8T6 CAN 驱动层实现 (Layer 3)
 *
 * Source:  ir/can_ir_summary.json v3.0
 */

#include "can_api.h"
#include "can_ll.h"
#include <stddef.h>

/*===========================================================================*/
/* Module Static Data (Encapsulation)                                        */
/*===========================================================================*/

#define CAN_REG         CAN1
#define CAN_TIMEOUT     (100000UL)

static Can_State_e g_can_state = CAN_STATE_UNINIT;

/*===========================================================================*/
/* Public API Implementation                                                 */
/*===========================================================================*/

Can_ReturnType Can_Init(const Can_ConfigType *pConfig)
{
    if (pConfig == NULL) { return CAN_ERR_PARAM; }

    /* 1. Enter Initialization Mode */
    CAN_LL_EnterInitMode(CAN_REG);
    uint32_t timeout = CAN_TIMEOUT;
    while (!CAN_LL_IsInInitMode(CAN_REG))
    {
        if (--timeout == 0U) { return CAN_ERR_TIMEOUT; }
    }

    /* 2. Configure Basic Control (MCR) */
    uint32_t mcr = CAN_LL_ComposeMCR(pConfig);
    CAN_LL_WriteMCR(CAN_REG, mcr | CAN_MCR_INRQ_Msk);

    /* 3. Configure Bit Timing (BTR) */
    uint32_t btr = CAN_LL_ComposeBTR(pConfig);
    CAN_LL_WriteBTR(CAN_REG, btr);

    /* 4. Exit Init Mode */
    CAN_LL_ExitInitMode(CAN_REG);
    timeout = CAN_TIMEOUT;
    while (CAN_LL_IsInInitMode(CAN_REG))
    {
        if (--timeout == 0U) { return CAN_ERR_TIMEOUT; }
    }

    g_can_state = CAN_STATE_READY;
    return CAN_OK;
}

void Can_DeInit(void)
{
    /* 1. Enter Reset State (INRQ=1, SLEEP=1) */
    CAN_LL_WriteMCR(CAN_REG, 0x00010002U); 
    g_can_state = CAN_STATE_UNINIT;
}

Can_ReturnType Can_SetFilter(const Can_FilterConfigType *pFilter)
{
    if (pFilter == NULL || pFilter->bank >= 14U) { return CAN_ERR_PARAM; }

    CAN_LL_EnterFilterInitMode(CAN_REG);

    uint32_t fr1, fr2;
    if (pFilter->isExtended)
    {
        /* 32-bit Identifier Mask Mode */
        fr1 = (pFilter->id << 3U) | 0x04U;   /* IDE=1 */
        fr2 = (pFilter->mask << 3U) | 0x04U;
    }
    else
    {
        /* 16-bit mode simplified to use 32-bit layout for filter bank */
        fr1 = (pFilter->id << 21U);
        fr2 = (pFilter->mask << 21U);
    }

    /* scale=1 (32-bit), mode=0 (IdMask) */
    CAN_LL_ConfigFilter(CAN_REG, pFilter->bank, fr1, fr2, 0U, 1U, pFilter->fifo);

    CAN_LL_ExitFilterInitMode(CAN_REG);

    return CAN_OK;
}

Can_ReturnType Can_Write(const Can_PduType *pPdu)
{
    if (pPdu == NULL || g_can_state != CAN_STATE_READY) { return CAN_ERR_NOT_INIT; }

    /* 1. Find Empty Mailbox (TSR.TME0/1/2) */
    uint8_t mailbox;
    uint32_t tsr = CAN_LL_GetTSR(CAN_REG);

    if (tsr & CAN_TSR_TME0_Msk)      { mailbox = 0U; }
    else if (tsr & CAN_TSR_TME1_Msk) { mailbox = 1U; }
    else if (tsr & CAN_TSR_TME2_Msk) { mailbox = 2U; }
    else { return CAN_ERR_BUSY; }

    /* 2. Fill Identifer and Length */
    uint32_t tir = 0U;
    if (pPdu->idType == CAN_ID_EXT)
    {
        tir = (pPdu->id << 3U) | CAN_TIR_IDE_Msk;
    }
    else
    {
        tir = (pPdu->id << 21U);
    }
    uint32_t tdtr = (uint32_t)(pPdu->length & 0x0FU);

    /* 3. Fill Data Payload */
    uint32_t low = 0U, high = 0U;
    for (uint8_t i = 0U; i < 4U; i++) {
        if (i < pPdu->length) {
            low |= ((uint32_t)pPdu->data[i] << (i * 8U));
        }
        if ((uint8_t)(i + 4U) < pPdu->length) {
            high |= ((uint32_t)pPdu->data[i + 4U] << (i * 8U));
        }
    }

    /* 4. Request Transmission via LL */
    CAN_LL_WriteTxMailbox(CAN_REG, mailbox, tir, tdtr, low, high);

    return CAN_OK;
}

Can_ReturnType Can_Read(uint8_t fifo, Can_PduType *pPdu)
{
    if (pPdu == NULL || fifo > 1U) { return CAN_ERR_PARAM; }

    /* 1. Check if message pending (RFR.FMP) via LL */
    uint32_t rfr = CAN_LL_GetFifoStatus(CAN_REG, fifo);
    if (!(rfr & 0x03U)) { return CAN_ERR_EMPTY; }

    /* 2. Read ID, Length and Data payload via LL */
    uint32_t rir, rdtr, low, high;
    CAN_LL_ReadRxMailbox(CAN_REG, fifo, &rir, &rdtr, &low, &high);

    if (rir & CAN_RIR_IDE_Msk) {
        pPdu->id = rir >> 3U;
        pPdu->idType = CAN_ID_EXT;
    } else {
        pPdu->id = rir >> 21U;
        pPdu->idType = CAN_ID_STD;
    }
    pPdu->length = (uint8_t)(rdtr & 0x0FU);

    for (uint8_t i = 0U; i < 4U; i++) {
        pPdu->data[i] = (uint8_t)(low >> (i * 8U));
        pPdu->data[i + 4U] = (uint8_t)(high >> (i * 8U));
    }

    /* 3. Release FIFO via LL */
    CAN_LL_ReleaseFifo(CAN_REG, fifo);

    return CAN_OK;
}
