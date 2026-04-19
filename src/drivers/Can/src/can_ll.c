/**
 * @file    can_ll.c
 * @brief   STM32F103C8T6 CAN LL 层非 inline 实现
 *
 * @note    本文件仅包含不适合 inline 的 LL 实现（如初始化序列复合操作）。
 *          大多数 LL 操作以 static inline 形式定义在 can_ll.h 中。
 *          Driver 层（can_drv.c）禁止直接访问寄存器（规则 R8-3）。
 *
 * Source:  RM0008 Rev 14, Chapter 24 - bxCAN
 *          IR: ir/can_ir_summary.json — configuration_strategies[],
 *              cross_field_constraints[], registers[]
 */

#include "can_ll.h"

#include <stddef.h>

/*===========================================================================*/
/* CAN_LL_ComposeMCR                                                          */
/*===========================================================================*/

/**
 * @brief  Compose and write MCR operational bits from a Can_ConfigType.
 *
 *         Preserves INRQ and SLEEP state, configures TXFP, RFLM, NART,
 *         AWUM, ABOM, TTCM, DBF in a single write to avoid intermediate
 *         state violations.
 *
 *         Guard: INV_CAN_001 — caller must ensure INAK=1.
 *         IR: configuration_strategies[0] INIT_BASIC step 5
 *         IR: configuration_strategies[3] AUTO_RECOVERY — ABOM mapping
 *
 * @param[in]  CANx    Pointer to CAN peripheral.
 * @param[in]  pConfig Configuration structure. Caller must guarantee non-NULL.
 */
void CAN_LL_ComposeMCR(CAN_TypeDef *CANx, const Can_ConfigType *pConfig)
{
    uint32_t mcr = CANx->MCR;

    /* Preserve INRQ and SLEEP state, clear all configurable bits */
    mcr &= (CAN_MCR_INRQ_Msk | CAN_MCR_SLEEP_Msk);

    /* TX FIFO priority — IR: registers[0] MCR.TXFP */
    if (pConfig->txPriority == CAN_TXPRIO_FIFO)
    {
        mcr |= CAN_MCR_TXFP_Msk; /* RM0008 §24.4.1 */
    }

    /* Receive FIFO locked mode — IR: registers[0] MCR.RFLM */
    if (pConfig->rxFifoLocked)
    {
        mcr |= CAN_MCR_RFLM_Msk; /* RM0008 §24.4.1 */
    }

    /* No auto retransmission — IR: registers[0] MCR.NART */
    if (!pConfig->autoRetransmit)
    {
        mcr |= CAN_MCR_NART_Msk; /* RM0008 §24.4.1 */
    }

    /* Auto wakeup — IR: registers[0] MCR.AWUM */
    if (pConfig->autoWakeup)
    {
        mcr |= CAN_MCR_AWUM_Msk; /* RM0008 §24.4.1 */
    }

    /* Auto bus-off recovery — IR: configuration_strategies[3] AUTO_RECOVERY */
    if (pConfig->autoBusOff)
    {
        mcr |= CAN_MCR_ABOM_Msk; /* RM0008 §24.4.1 */
    }

    /* Time-triggered communication — IR: registers[0] MCR.TTCM */
    if (pConfig->timeTrigger)
    {
        mcr |= CAN_MCR_TTCM_Msk; /* RM0008 §24.4.1 */
    }

    /* Debug freeze — default enabled */
    mcr |= CAN_MCR_DBF_Msk; /* RM0008 §24.4.1 */

    /* Write composed value in one shot */
    CANx->MCR = mcr; /* IR: configuration_strategies[0] — RM0008 §24.4.1 */
}

/*===========================================================================*/
/* CAN_LL_ComposeBTR                                                          */
/*===========================================================================*/

/**
 * @brief  Compose and write BTR with bit timing and operation mode.
 *
 *         Computes the full BTR value from config fields and writes it in
 *         a single assignment. Includes loopback and silent mode bits.
 *
 *         Guard: INV_CAN_001 — BTR writable only in init mode (INAK=1).
 *         IR: configuration_strategies[0] INIT_BASIC step 6
 *         IR: configuration_strategies[1] LOOPBACK_DEBUG — LBKM=1
 *         IR: configuration_strategies[2] SILENT_MODE — SILM=1
 *         IR: timing_constraints.baud_rate_equation
 *
 *         Baud rate formula (ir/can_ir_summary.json timing_constraints):
 *           BaudRate = APB1_CLK / ((BRP+1) * (1 + (TS1+1) + (TS2+1)))
 *
 * @param[in]  CANx    Pointer to CAN peripheral.
 * @param[in]  pConfig Configuration with bit timing and mode.
 *                     Caller must guarantee non-NULL and valid ranges.
 */
void CAN_LL_ComposeBTR(CAN_TypeDef *CANx, const Can_ConfigType *pConfig)
{
    uint32_t btr = 0U;

    /* Guard: INV_CAN_001 — BTR only writable when INAK=1 — RM0008 §24.4.1 */

    /* BRP: prescaler-1 — IR: registers[7] BTR.BRP */
    btr |= (((uint32_t)(pConfig->bitTiming.prescaler - 1U)) << CAN_BTR_BRP_Pos) & CAN_BTR_BRP_Msk;

    /* TS1: timeSeg1-1 — IR: registers[7] BTR.TS1 */
    btr |= (((uint32_t)(pConfig->bitTiming.timeSeg1 - 1U)) << CAN_BTR_TS1_Pos) & CAN_BTR_TS1_Msk;

    /* TS2: timeSeg2-1 — IR: registers[7] BTR.TS2 */
    btr |= (((uint32_t)(pConfig->bitTiming.timeSeg2 - 1U)) << CAN_BTR_TS2_Pos) & CAN_BTR_TS2_Msk;

    /* SJW: syncJumpW-1 — IR: registers[7] BTR.SJW */
    btr |= (((uint32_t)(pConfig->bitTiming.syncJumpW - 1U)) << CAN_BTR_SJW_Pos) & CAN_BTR_SJW_Msk;

    /* Loopback mode — IR: configuration_strategies[1] LOOPBACK_DEBUG */
    if ((pConfig->mode == CAN_MODE_LOOPBACK) ||
        (pConfig->mode == CAN_MODE_LOOPBACK_SILENT))
    {
        btr |= CAN_BTR_LBKM_Msk; /* RM0008 §24.4.1 */
    }

    /* Silent mode — IR: configuration_strategies[2] SILENT_MODE */
    if ((pConfig->mode == CAN_MODE_SILENT) ||
        (pConfig->mode == CAN_MODE_LOOPBACK_SILENT))
    {
        btr |= CAN_BTR_SILM_Msk; /* RM0008 §24.4.1 */
    }

    /* Write composed value in one shot */
    CANx->BTR = btr; /* IR: configuration_strategies[0] — RM0008 §24.4.1 */
}
