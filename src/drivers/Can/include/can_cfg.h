/**
 * @file    can_cfg.h
 * @brief   STM32F103C8T6 CAN 编译期配置层
 *
 * @note    只允许使用 #define，禁止包含类型定义或函数声明。
 *          禁止 include 任何项目头文件。
 *          所有宏命名带 CAN_CFG_ 前缀。
 *
 * Source:  RM0008 Rev 14, Chapter 24
 *          ir/can_ir_summary.json v2.0
 */

#ifndef CAN_CFG_H
#define CAN_CFG_H

/*===========================================================================*/
/* Instance enable                                                            */
/* STM32F103C8T6 has only CAN1                                               */
/*===========================================================================*/

#define CAN_CFG_CAN1_ENABLE         (1U)

/*===========================================================================*/
/* Timeout (loop iteration counts)                                            */
/* All polling loops must exit after this many iterations.                   */
/*===========================================================================*/

#define CAN_CFG_TIMEOUT_LOOPS       ((uint32_t)100000U)

/*===========================================================================*/
/* Default bit timing: 500 kbps @ APB1 = 36 MHz                             */
/* Source: ir/can_ir_summary.json timing_constraints.common_baud_rates[]     */
/* BaudRate = 36MHz / ((5+1) * (1 + (8+1) + (1+1))) = 36M / (6*12) = 500k  */
/*===========================================================================*/

#define CAN_CFG_DEFAULT_PRESCALER   ((uint16_t)6U)   /* BRP+1 = 6            */
#define CAN_CFG_DEFAULT_TS1         ((uint8_t)9U)     /* TS1+1 = 9 Tq        */
#define CAN_CFG_DEFAULT_TS2         ((uint8_t)2U)     /* TS2+1 = 2 Tq        */
#define CAN_CFG_DEFAULT_SJW         ((uint8_t)1U)     /* SJW+1 = 1 Tq        */

/*===========================================================================*/
/* Interrupt feature selection                                                */
/*===========================================================================*/

/* Enable interrupt-driven TX/RX/error support (1=enabled, 0=polling only) */
#define CAN_CFG_IRQ_ENABLE          (1U)

/* CAN1_TX IRQ number (NVIC position 19) */
/* Source: ir/can_ir_summary.json interrupts[0] */
#define CAN_CFG_CAN1_TX_IRQ_NUMBER  (19U)

/* CAN1_RX0 IRQ number (NVIC position 20) */
/* Source: ir/can_ir_summary.json interrupts[1] */
#define CAN_CFG_CAN1_RX0_IRQ_NUMBER (20U)

/* CAN1_RX1 IRQ number (NVIC position 21) */
/* Source: ir/can_ir_summary.json interrupts[2] */
#define CAN_CFG_CAN1_RX1_IRQ_NUMBER (21U)

/* CAN1_SCE IRQ number (NVIC position 22) */
/* Source: ir/can_ir_summary.json interrupts[3] */
#define CAN_CFG_CAN1_SCE_IRQ_NUMBER (22U)

/* Default IRQ preemption priority */
/* Source: embedded-c-coding-standard.md §5.4 N-04 */
#define CAN_CFG_IRQ_PRIORITY        (4U)

/*===========================================================================*/
/* TX mailbox count                                                           */
/*===========================================================================*/

#define CAN_CFG_TX_MAILBOX_COUNT    (3U)

/*===========================================================================*/
/* Filter bank count (STM32F103 single-CAN = 14)                             */
/* Source: ir/can_ir_summary.json generation_metadata.notes[]                */
/*===========================================================================*/

#define CAN_CFG_FILTER_BANK_COUNT   (14U)

#endif /* CAN_CFG_H */
