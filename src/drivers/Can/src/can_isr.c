/**
 * @file    can_isr.c
 * @brief   STM32F103C8T6 CAN 中断服务函数 (V4.0)
 */

#include "can_reg.h"

void USB_HP_CAN1_TX_IRQHandler(void)
{
    /* TX Mailbox Empty */
}

void USB_LP_CAN1_RX0_IRQHandler(void)
{
    /* FIFO 0 Message Pending */
}

void CAN1_RX1_IRQHandler(void)
{
    /* FIFO 1 Message Pending */
}

void CAN1_SCE_IRQHandler(void)
{
    /* Status Change / Error */
}
