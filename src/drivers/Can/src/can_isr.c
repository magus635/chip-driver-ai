/**
 * @file    can_isr.c
 * @brief   STM32F103C8T6 CAN 中断服务函数
 *
 * @note    仅包含向量表中断入口函数。
 *          所有逻辑处理委托给 can_drv.c 中的 Handler 函数。
 *          Source: RM0008 §10.1.2 Table 63; ir/can_ir_summary.json interrupts[]
 *
 * ISR 职责边界（架构规范强制要求）：
 *   - 仅做函数分发，不处理任何业务逻辑
 *   - 禁止调用任何阻塞 OS API
 *   - 禁止直接操作寄存器
 *
 * IRQ mapping (ir/can_ir_summary.json interrupts[]):
 *   CAN1_TX   — IRQ 19 — Transmit interrupt
 *   CAN1_RX0  — IRQ 20 — Receive FIFO 0 interrupt
 *   CAN1_RX1  — IRQ 21 — Receive FIFO 1 interrupt
 *   CAN1_SCE  — IRQ 22 — Status Change / Error interrupt
 */

#include "can_api.h"

/*===========================================================================*/
/* CAN1 TX global interrupt handler                                          */
/* IRQ number: 19                                                             */
/* Source: ir/can_ir_summary.json interrupts[0] name="CAN1_TX"               */
/*===========================================================================*/

void USB_HP_CAN1_TX_IRQHandler(void)
{
    /* Delegate to driver layer — handles RQCP0/1/2 flags and TX callback */
    Can_TxIRQHandler();
}

/*===========================================================================*/
/* CAN1 RX0 global interrupt handler                                         */
/* IRQ number: 20                                                             */
/* Source: ir/can_ir_summary.json interrupts[1] name="CAN1_RX0"              */
/*===========================================================================*/

void USB_LP_CAN1_RX0_IRQHandler(void)
{
    /* Delegate to driver layer — handles FMP0/FULL0/FOVR0 and RX callback */
    Can_Rx0IRQHandler();
}

/*===========================================================================*/
/* CAN1 RX1 global interrupt handler                                         */
/* IRQ number: 21                                                             */
/* Source: ir/can_ir_summary.json interrupts[2] name="CAN1_RX1"              */
/*===========================================================================*/

void CAN1_RX1_IRQHandler(void)
{
    /* Delegate to driver layer — handles FMP1/FULL1/FOVR1 and RX callback */
    Can_Rx1IRQHandler();
}

/*===========================================================================*/
/* CAN1 SCE (Status Change / Error) global interrupt handler                 */
/* IRQ number: 22                                                             */
/* Source: ir/can_ir_summary.json interrupts[3] name="CAN1_SCE"              */
/*===========================================================================*/

void CAN1_SCE_IRQHandler(void)
{
    /* Delegate to driver layer — handles ERRI/EWGF/EPVF/BOFF/LEC/WKUI/SLAKI */
    Can_SceIRQHandler();
}
