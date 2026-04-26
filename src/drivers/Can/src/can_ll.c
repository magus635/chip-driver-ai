/**
 * @file    can_ll.c
 * @brief   STM32F103C8T6 CAN LL 层实现 (Complex Logic Composition)
 *
 * Source:  ir/can_ir_summary.json v3.0
 */

#include "can_ll.h"

/**
 * @brief  Compose BTR register value (Timing + Mode).
 * @param  pConfig Pointer to CAN configuration.
 * @return 32-bit BTR value.
 */
uint32_t CAN_LL_ComposeBTR(const Can_ConfigType *pConfig)
{
    uint32_t btr = 0U;

    /* Timing Parameters */
    btr |= ((pConfig->brp - 1U) << CAN_BTR_BRP_Pos) & CAN_BTR_BRP_Msk;
    btr |= ((pConfig->ts1 - 1U) << CAN_BTR_TS1_Pos) & CAN_BTR_TS1_Msk;
    btr |= ((pConfig->ts2 - 1U) << CAN_BTR_TS2_Pos) & CAN_BTR_TS2_Msk;
    btr |= (pConfig->sjw << CAN_BTR_SJW_Pos) & CAN_BTR_SJW_Msk;

    /* Test Modes (R14 Matrix Mapping) */
    if (pConfig->mode == CAN_MODE_LOOPBACK)
    {
        btr |= CAN_BTR_LBKM_Msk;
    }
    else if (pConfig->mode == CAN_MODE_SILENT)
    {
        btr |= CAN_BTR_SILM_Msk;
    }
    else if (pConfig->mode == CAN_MODE_LOOPBACK_SILENT)
    {
        btr |= (CAN_BTR_LBKM_Msk | CAN_BTR_SILM_Msk);
    }

    return btr;
}

/**
 * @brief  Compose MCR register value.
 * @param  pConfig Pointer to CAN configuration.
 * @return 32-bit MCR value.
 */
uint32_t CAN_LL_ComposeMCR(const Can_ConfigType *pConfig)
{
    uint32_t mcr = 0U;

    if (pConfig->autoBusOff) { mcr |= CAN_MCR_ABOM_Msk; }
    if (pConfig->autoWakeup) { mcr |= CAN_MCR_AWUM_Msk; }
    if (pConfig->txPriority) { mcr |= CAN_MCR_TXFP_Msk; } /* FIFO priority */

    /* Ensure INRQ/SLEEP are handled by LL State Machine, not here */

    return mcr;
}
