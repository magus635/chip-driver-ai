/**
 * @file    can_reg.h
 * @brief   STM32F103C8T6 CAN 寄存器映射层
 *
 * @note    仅包含物理地址、寄存器偏移、位域宏定义和寄存器结构体。
 *          禁止包含任何逻辑代码或函数实现。
 *
 * Source:  RM0008 Rev 14, Chapter 24 - Controller area network (bxCAN)
 *          ir/can_ir_summary.json v2.0
 */

#ifndef CAN_REG_H
#define CAN_REG_H

#include <stdint.h>

/*===========================================================================*/
/* CMSIS-style access qualifier macros                                        */
/*===========================================================================*/

#ifndef __IO
#define __IO   volatile         /* Read/Write */
#endif

#ifndef __I
#define __I    volatile const   /* Read only  */
#endif

#ifndef __O
#define __O    volatile         /* Write only */
#endif

/*===========================================================================*/
/* CAN base address                                                          */
/* Source: RM0008 §24.0, ir/can_ir_summary.json instances[0]                */
/*===========================================================================*/

#define CAN1_BASE_ADDR          ((uint32_t)0x40006400UL)  /* APB1 */

/*===========================================================================*/
/* CAN TX Mailbox structure                                                   */
/* Source: RM0008 §24.5, offsets 0x180-0x1AC                                */
/*===========================================================================*/

typedef struct
{
    __IO uint32_t TIR;      /* TX mailbox identifier register    */
    __IO uint32_t TDTR;     /* TX mailbox data length and time   */
    __IO uint32_t TDLR;     /* TX mailbox data low register      */
    __IO uint32_t TDHR;     /* TX mailbox data high register     */
} CAN_TxMailBox_TypeDef;

/*===========================================================================*/
/* CAN RX FIFO Mailbox structure                                              */
/* Source: RM0008 §24.5, offsets 0x1B0-0x1CC                                */
/*===========================================================================*/

typedef struct
{
    __I  uint32_t RIR;      /* RX FIFO mailbox identifier register   */
    __I  uint32_t RDTR;     /* RX FIFO mailbox data length and time  */
    __I  uint32_t RDLR;     /* RX FIFO mailbox data low register     */
    __I  uint32_t RDHR;     /* RX FIFO mailbox data high register    */
} CAN_FIFOMailBox_TypeDef;

/*===========================================================================*/
/* CAN Filter Bank structure                                                  */
/* Source: RM0008 §24.5, offsets 0x240-0x31C                                */
/*===========================================================================*/

typedef struct
{
    __IO uint32_t FR1;      /* Filter bank register 1 */
    __IO uint32_t FR2;      /* Filter bank register 2 */
} CAN_FilterBank_TypeDef;

/*===========================================================================*/
/* CAN register structure                                                     */
/* Source: RM0008 §24.5; ir/can_ir_summary.json registers[]                 */
/*===========================================================================*/

typedef struct
{
    __IO uint32_t MCR;           /* Master Control Register,         offset: 0x00, RM0008 §24.4.1 */
    __IO uint32_t MSR;           /* Master Status Register,          offset: 0x04, RM0008 §24.4.1 */
    __IO uint32_t TSR;           /* Transmit Status Register,        offset: 0x08, RM0008 §24.4.2 */
    __IO uint32_t RF0R;          /* Receive FIFO 0 Register,         offset: 0x0C, RM0008 §24.4.3 */
    __IO uint32_t RF1R;          /* Receive FIFO 1 Register,         offset: 0x10, RM0008 §24.4.3 */
    __IO uint32_t IER;           /* Interrupt Enable Register,       offset: 0x14, RM0008 §24.5   */
    __IO uint32_t ESR;           /* Error Status Register,           offset: 0x18, RM0008 §24.4.6 */
    __IO uint32_t BTR;           /* Bit Timing Register,             offset: 0x1C, RM0008 §24.4.1 */
         uint32_t RESERVED0[88]; /* Reserved, 0x20-0x17F                                          */
    CAN_TxMailBox_TypeDef   sTxMailBox[3];   /* TX mailboxes 0-2,   offset: 0x180-0x1AC           */
    CAN_FIFOMailBox_TypeDef sFIFOMailBox[2]; /* RX FIFO 0-1,        offset: 0x1B0-0x1CC           */
         uint32_t RESERVED1[12]; /* Reserved, 0x1D0-0x1FF                                         */
    __IO uint32_t FMR;           /* Filter Master Register,          offset: 0x200, RM0008 §24.5  */
    __IO uint32_t FM1R;          /* Filter Mode Register,            offset: 0x204, RM0008 §24.5  */
         uint32_t RESERVED2;     /* Reserved, 0x208                                               */
    __IO uint32_t FS1R;          /* Filter Scale Register,           offset: 0x20C, RM0008 §24.5  */
         uint32_t RESERVED3;     /* Reserved, 0x210                                               */
    __IO uint32_t FFA1R;         /* Filter FIFO Assignment Register, offset: 0x214, RM0008 §24.5  */
         uint32_t RESERVED4;     /* Reserved, 0x218                                               */
    __IO uint32_t FA1R;          /* Filter Activation Register,      offset: 0x21C, RM0008 §24.5  */
         uint32_t RESERVED5[8];  /* Reserved, 0x220-0x23F                                         */
    CAN_FilterBank_TypeDef  sFilterBank[14]; /* Filter banks 0-13,  offset: 0x240-0x31C           */
} CAN_TypeDef;

/*===========================================================================*/
/* CAN instance pointer                                                       */
/*===========================================================================*/

#define CAN1    ((CAN_TypeDef *)CAN1_BASE_ADDR)

/*===========================================================================*/
/* MCR bitfield definitions                                                   */
/* Source: RM0008 §24.4.1; ir/can_ir_summary.json registers[0].bitfields[]  */
/*===========================================================================*/

#define CAN_MCR_INRQ_Pos        ((uint32_t)0U)
#define CAN_MCR_INRQ_Msk        ((uint32_t)(1UL << CAN_MCR_INRQ_Pos))

#define CAN_MCR_SLEEP_Pos       ((uint32_t)1U)
#define CAN_MCR_SLEEP_Msk       ((uint32_t)(1UL << CAN_MCR_SLEEP_Pos))

#define CAN_MCR_TXFP_Pos        ((uint32_t)2U)
#define CAN_MCR_TXFP_Msk        ((uint32_t)(1UL << CAN_MCR_TXFP_Pos))

#define CAN_MCR_RFLM_Pos        ((uint32_t)3U)
#define CAN_MCR_RFLM_Msk        ((uint32_t)(1UL << CAN_MCR_RFLM_Pos))

#define CAN_MCR_NART_Pos        ((uint32_t)4U)
#define CAN_MCR_NART_Msk        ((uint32_t)(1UL << CAN_MCR_NART_Pos))

#define CAN_MCR_AWUM_Pos        ((uint32_t)5U)
#define CAN_MCR_AWUM_Msk        ((uint32_t)(1UL << CAN_MCR_AWUM_Pos))

#define CAN_MCR_ABOM_Pos        ((uint32_t)6U)
#define CAN_MCR_ABOM_Msk        ((uint32_t)(1UL << CAN_MCR_ABOM_Pos))

#define CAN_MCR_TTCM_Pos        ((uint32_t)7U)
#define CAN_MCR_TTCM_Msk        ((uint32_t)(1UL << CAN_MCR_TTCM_Pos))

#define CAN_MCR_RESET_Pos       ((uint32_t)15U)
#define CAN_MCR_RESET_Msk       ((uint32_t)(1UL << CAN_MCR_RESET_Pos))

#define CAN_MCR_DBF_Pos         ((uint32_t)16U)
#define CAN_MCR_DBF_Msk         ((uint32_t)(1UL << CAN_MCR_DBF_Pos))

/*===========================================================================*/
/* MSR bitfield definitions                                                   */
/* Source: RM0008 §24.4.1; ir/can_ir_summary.json registers[1].bitfields[]  */
/* NOTE: ERRI/WKUI/SLAKI are RC_W1 — use direct write to clear, NO |=      */
/*===========================================================================*/

#define CAN_MSR_INAK_Pos        ((uint32_t)0U)
#define CAN_MSR_INAK_Msk        ((uint32_t)(1UL << CAN_MSR_INAK_Pos))

#define CAN_MSR_SLAK_Pos        ((uint32_t)1U)
#define CAN_MSR_SLAK_Msk        ((uint32_t)(1UL << CAN_MSR_SLAK_Pos))

#define CAN_MSR_ERRI_Pos        ((uint32_t)2U)
#define CAN_MSR_ERRI_Msk        ((uint32_t)(1UL << CAN_MSR_ERRI_Pos))

#define CAN_MSR_WKUI_Pos        ((uint32_t)3U)
#define CAN_MSR_WKUI_Msk        ((uint32_t)(1UL << CAN_MSR_WKUI_Pos))

#define CAN_MSR_SLAKI_Pos       ((uint32_t)4U)
#define CAN_MSR_SLAKI_Msk       ((uint32_t)(1UL << CAN_MSR_SLAKI_Pos))

#define CAN_MSR_TXM_Pos         ((uint32_t)8U)
#define CAN_MSR_TXM_Msk         ((uint32_t)(1UL << CAN_MSR_TXM_Pos))

#define CAN_MSR_RXM_Pos         ((uint32_t)9U)
#define CAN_MSR_RXM_Msk         ((uint32_t)(1UL << CAN_MSR_RXM_Pos))

#define CAN_MSR_SAMP_Pos        ((uint32_t)10U)
#define CAN_MSR_SAMP_Msk        ((uint32_t)(1UL << CAN_MSR_SAMP_Pos))

#define CAN_MSR_RX_Pos          ((uint32_t)11U)
#define CAN_MSR_RX_Msk          ((uint32_t)(1UL << CAN_MSR_RX_Pos))

/*===========================================================================*/
/* TSR bitfield definitions                                                   */
/* Source: RM0008 §24.4.2; ir/can_ir_summary.json registers[2].bitfields[]  */
/* NOTE: RQCP0/1/2 are RC_W1 — use direct write to clear, NO |=            */
/*===========================================================================*/

#define CAN_TSR_RQCP0_Pos       ((uint32_t)0U)
#define CAN_TSR_RQCP0_Msk       ((uint32_t)(1UL << CAN_TSR_RQCP0_Pos))

#define CAN_TSR_TXOK0_Pos       ((uint32_t)1U)
#define CAN_TSR_TXOK0_Msk       ((uint32_t)(1UL << CAN_TSR_TXOK0_Pos))

#define CAN_TSR_ALST0_Pos       ((uint32_t)2U)
#define CAN_TSR_ALST0_Msk       ((uint32_t)(1UL << CAN_TSR_ALST0_Pos))

#define CAN_TSR_TERR0_Pos       ((uint32_t)3U)
#define CAN_TSR_TERR0_Msk       ((uint32_t)(1UL << CAN_TSR_TERR0_Pos))

#define CAN_TSR_ABRQ0_Pos       ((uint32_t)7U)
#define CAN_TSR_ABRQ0_Msk       ((uint32_t)(1UL << CAN_TSR_ABRQ0_Pos))

#define CAN_TSR_RQCP1_Pos       ((uint32_t)8U)
#define CAN_TSR_RQCP1_Msk       ((uint32_t)(1UL << CAN_TSR_RQCP1_Pos))

#define CAN_TSR_TXOK1_Pos       ((uint32_t)9U)
#define CAN_TSR_TXOK1_Msk       ((uint32_t)(1UL << CAN_TSR_TXOK1_Pos))

#define CAN_TSR_ALST1_Pos       ((uint32_t)10U)
#define CAN_TSR_ALST1_Msk       ((uint32_t)(1UL << CAN_TSR_ALST1_Pos))

#define CAN_TSR_TERR1_Pos       ((uint32_t)11U)
#define CAN_TSR_TERR1_Msk       ((uint32_t)(1UL << CAN_TSR_TERR1_Pos))

#define CAN_TSR_ABRQ1_Pos       ((uint32_t)15U)
#define CAN_TSR_ABRQ1_Msk       ((uint32_t)(1UL << CAN_TSR_ABRQ1_Pos))

#define CAN_TSR_RQCP2_Pos       ((uint32_t)16U)
#define CAN_TSR_RQCP2_Msk       ((uint32_t)(1UL << CAN_TSR_RQCP2_Pos))

#define CAN_TSR_TXOK2_Pos       ((uint32_t)17U)
#define CAN_TSR_TXOK2_Msk       ((uint32_t)(1UL << CAN_TSR_TXOK2_Pos))

#define CAN_TSR_ALST2_Pos       ((uint32_t)18U)
#define CAN_TSR_ALST2_Msk       ((uint32_t)(1UL << CAN_TSR_ALST2_Pos))

#define CAN_TSR_TERR2_Pos       ((uint32_t)19U)
#define CAN_TSR_TERR2_Msk       ((uint32_t)(1UL << CAN_TSR_TERR2_Pos))

#define CAN_TSR_ABRQ2_Pos       ((uint32_t)23U)
#define CAN_TSR_ABRQ2_Msk       ((uint32_t)(1UL << CAN_TSR_ABRQ2_Pos))

#define CAN_TSR_CODE_Pos        ((uint32_t)24U)
#define CAN_TSR_CODE_Msk        ((uint32_t)(0x3UL << CAN_TSR_CODE_Pos))

#define CAN_TSR_TME0_Pos        ((uint32_t)26U)
#define CAN_TSR_TME0_Msk        ((uint32_t)(1UL << CAN_TSR_TME0_Pos))

#define CAN_TSR_TME1_Pos        ((uint32_t)27U)
#define CAN_TSR_TME1_Msk        ((uint32_t)(1UL << CAN_TSR_TME1_Pos))

#define CAN_TSR_TME2_Pos        ((uint32_t)28U)
#define CAN_TSR_TME2_Msk        ((uint32_t)(1UL << CAN_TSR_TME2_Pos))

#define CAN_TSR_LOW0_Pos        ((uint32_t)29U)
#define CAN_TSR_LOW0_Msk        ((uint32_t)(1UL << CAN_TSR_LOW0_Pos))

#define CAN_TSR_LOW1_Pos        ((uint32_t)30U)
#define CAN_TSR_LOW1_Msk        ((uint32_t)(1UL << CAN_TSR_LOW1_Pos))

#define CAN_TSR_LOW2_Pos        ((uint32_t)31U)
#define CAN_TSR_LOW2_Msk        ((uint32_t)(1UL << CAN_TSR_LOW2_Pos))

/*===========================================================================*/
/* RF0R bitfield definitions                                                  */
/* Source: RM0008 §24.4.3; ir/can_ir_summary.json registers[3].bitfields[]  */
/* NOTE: FULL0/FOVR0 are RC_W1 — use direct write to clear, NO |=          */
/*===========================================================================*/

#define CAN_RF0R_FMP0_Pos       ((uint32_t)0U)
#define CAN_RF0R_FMP0_Msk       ((uint32_t)(0x3UL << CAN_RF0R_FMP0_Pos))

#define CAN_RF0R_FULL0_Pos      ((uint32_t)3U)
#define CAN_RF0R_FULL0_Msk      ((uint32_t)(1UL << CAN_RF0R_FULL0_Pos))

#define CAN_RF0R_FOVR0_Pos     ((uint32_t)4U)
#define CAN_RF0R_FOVR0_Msk    ((uint32_t)(1UL << CAN_RF0R_FOVR0_Pos))

#define CAN_RF0R_RFOM0_Pos      ((uint32_t)5U)
#define CAN_RF0R_RFOM0_Msk      ((uint32_t)(1UL << CAN_RF0R_RFOM0_Pos))

/*===========================================================================*/
/* RF1R bitfield definitions                                                  */
/* Source: RM0008 §24.4.3; ir/can_ir_summary.json registers[4].bitfields[]  */
/* NOTE: FULL1/FOVR1 are RC_W1 — use direct write to clear, NO |=          */
/*===========================================================================*/

#define CAN_RF1R_FMP1_Pos       ((uint32_t)0U)
#define CAN_RF1R_FMP1_Msk       ((uint32_t)(0x3UL << CAN_RF1R_FMP1_Pos))

#define CAN_RF1R_FULL1_Pos      ((uint32_t)3U)
#define CAN_RF1R_FULL1_Msk      ((uint32_t)(1UL << CAN_RF1R_FULL1_Pos))

#define CAN_RF1R_FOVR1_Pos     ((uint32_t)4U)
#define CAN_RF1R_FOVR1_Msk    ((uint32_t)(1UL << CAN_RF1R_FOVR1_Pos))

#define CAN_RF1R_RFOM1_Pos      ((uint32_t)5U)
#define CAN_RF1R_RFOM1_Msk      ((uint32_t)(1UL << CAN_RF1R_RFOM1_Pos))

/*===========================================================================*/
/* IER bitfield definitions                                                   */
/* Source: RM0008 §24.5; ir/can_ir_summary.json registers[5].bitfields[]    */
/*===========================================================================*/

#define CAN_IER_TMEIE_Pos       ((uint32_t)0U)
#define CAN_IER_TMEIE_Msk       ((uint32_t)(1UL << CAN_IER_TMEIE_Pos))

#define CAN_IER_FMPIE0_Pos      ((uint32_t)1U)
#define CAN_IER_FMPIE0_Msk      ((uint32_t)(1UL << CAN_IER_FMPIE0_Pos))

#define CAN_IER_FFIE0_Pos       ((uint32_t)2U)
#define CAN_IER_FFIE0_Msk       ((uint32_t)(1UL << CAN_IER_FFIE0_Pos))

#define CAN_IER_FOVIE0_Pos      ((uint32_t)3U)
#define CAN_IER_FOVIE0_Msk      ((uint32_t)(1UL << CAN_IER_FOVIE0_Pos))

#define CAN_IER_FMPIE1_Pos      ((uint32_t)4U)
#define CAN_IER_FMPIE1_Msk      ((uint32_t)(1UL << CAN_IER_FMPIE1_Pos))

#define CAN_IER_FFIE1_Pos       ((uint32_t)5U)
#define CAN_IER_FFIE1_Msk       ((uint32_t)(1UL << CAN_IER_FFIE1_Pos))

#define CAN_IER_FOVIE1_Pos      ((uint32_t)6U)
#define CAN_IER_FOVIE1_Msk      ((uint32_t)(1UL << CAN_IER_FOVIE1_Pos))

#define CAN_IER_EWGIE_Pos       ((uint32_t)8U)
#define CAN_IER_EWGIE_Msk       ((uint32_t)(1UL << CAN_IER_EWGIE_Pos))

#define CAN_IER_EPVIE_Pos       ((uint32_t)9U)
#define CAN_IER_EPVIE_Msk       ((uint32_t)(1UL << CAN_IER_EPVIE_Pos))

#define CAN_IER_BOFIE_Pos       ((uint32_t)10U)
#define CAN_IER_BOFIE_Msk       ((uint32_t)(1UL << CAN_IER_BOFIE_Pos))

#define CAN_IER_LECIE_Pos       ((uint32_t)11U)
#define CAN_IER_LECIE_Msk       ((uint32_t)(1UL << CAN_IER_LECIE_Pos))

#define CAN_IER_ERRIE_Pos       ((uint32_t)15U)
#define CAN_IER_ERRIE_Msk       ((uint32_t)(1UL << CAN_IER_ERRIE_Pos))

#define CAN_IER_WKUIE_Pos       ((uint32_t)16U)
#define CAN_IER_WKUIE_Msk       ((uint32_t)(1UL << CAN_IER_WKUIE_Pos))

#define CAN_IER_SLKIE_Pos       ((uint32_t)17U)
#define CAN_IER_SLKIE_Msk       ((uint32_t)(1UL << CAN_IER_SLKIE_Pos))

/*===========================================================================*/
/* ESR bitfield definitions                                                   */
/* Source: RM0008 §24.4.6; ir/can_ir_summary.json registers[6].bitfields[]  */
/*===========================================================================*/

#define CAN_ESR_EWGF_Pos        ((uint32_t)0U)
#define CAN_ESR_EWGF_Msk        ((uint32_t)(1UL << CAN_ESR_EWGF_Pos))

#define CAN_ESR_EPVF_Pos        ((uint32_t)1U)
#define CAN_ESR_EPVF_Msk        ((uint32_t)(1UL << CAN_ESR_EPVF_Pos))

#define CAN_ESR_BOFF_Pos        ((uint32_t)2U)
#define CAN_ESR_BOFF_Msk        ((uint32_t)(1UL << CAN_ESR_BOFF_Pos))

#define CAN_ESR_LEC_Pos         ((uint32_t)4U)
#define CAN_ESR_LEC_Msk         ((uint32_t)(0x7UL << CAN_ESR_LEC_Pos))

#define CAN_ESR_TEC_Pos         ((uint32_t)16U)
#define CAN_ESR_TEC_Msk         ((uint32_t)(0xFFUL << CAN_ESR_TEC_Pos))

#define CAN_ESR_REC_Pos         ((uint32_t)24U)
#define CAN_ESR_REC_Msk         ((uint32_t)(0xFFUL << CAN_ESR_REC_Pos))

/*===========================================================================*/
/* BTR bitfield definitions                                                   */
/* Source: RM0008 §24.4.1; ir/can_ir_summary.json registers[7].bitfields[]  */
/*===========================================================================*/

#define CAN_BTR_BRP_Pos         ((uint32_t)0U)
#define CAN_BTR_BRP_Msk         ((uint32_t)(0x3FFUL << CAN_BTR_BRP_Pos))

#define CAN_BTR_TS1_Pos         ((uint32_t)16U)
#define CAN_BTR_TS1_Msk         ((uint32_t)(0xFUL << CAN_BTR_TS1_Pos))

#define CAN_BTR_TS2_Pos         ((uint32_t)20U)
#define CAN_BTR_TS2_Msk         ((uint32_t)(0x7UL << CAN_BTR_TS2_Pos))

#define CAN_BTR_SJW_Pos         ((uint32_t)24U)
#define CAN_BTR_SJW_Msk         ((uint32_t)(0x3UL << CAN_BTR_SJW_Pos))

#define CAN_BTR_LBKM_Pos        ((uint32_t)30U)
#define CAN_BTR_LBKM_Msk        ((uint32_t)(1UL << CAN_BTR_LBKM_Pos))

#define CAN_BTR_SILM_Pos        ((uint32_t)31U)
#define CAN_BTR_SILM_Msk        ((uint32_t)(1UL << CAN_BTR_SILM_Pos))

/*===========================================================================*/
/* FMR bitfield definitions                                                   */
/* Source: RM0008 §24.5; ir/can_ir_summary.json registers[8].bitfields[]    */
/*===========================================================================*/

#define CAN_FMR_FINIT_Pos       ((uint32_t)0U)
#define CAN_FMR_FINIT_Msk       ((uint32_t)(1UL << CAN_FMR_FINIT_Pos))

/*===========================================================================*/
/* TX Mailbox identifier register (TIR) bitfield definitions                 */
/* Source: RM0008 §24.5                                                      */
/*===========================================================================*/

#define CAN_TIR_TXRQ_Pos        ((uint32_t)0U)
#define CAN_TIR_TXRQ_Msk        ((uint32_t)(1UL << CAN_TIR_TXRQ_Pos))

#define CAN_TIR_RTR_Pos         ((uint32_t)1U)
#define CAN_TIR_RTR_Msk         ((uint32_t)(1UL << CAN_TIR_RTR_Pos))

#define CAN_TIR_IDE_Pos         ((uint32_t)2U)
#define CAN_TIR_IDE_Msk         ((uint32_t)(1UL << CAN_TIR_IDE_Pos))

#define CAN_TIR_EXID_Pos        ((uint32_t)3U)
#define CAN_TIR_EXID_Msk        ((uint32_t)(0x3FFFFUL << CAN_TIR_EXID_Pos))

#define CAN_TIR_STID_Pos        ((uint32_t)21U)
#define CAN_TIR_STID_Msk        ((uint32_t)(0x7FFUL << CAN_TIR_STID_Pos))

/*===========================================================================*/
/* TX Mailbox data length register (TDTR) bitfield definitions               */
/* Source: RM0008 §24.5                                                      */
/*===========================================================================*/

#define CAN_TDTR_DLC_Pos        ((uint32_t)0U)
#define CAN_TDTR_DLC_Msk        ((uint32_t)(0xFUL << CAN_TDTR_DLC_Pos))

#define CAN_TDTR_TGT_Pos        ((uint32_t)8U)
#define CAN_TDTR_TGT_Msk        ((uint32_t)(1UL << CAN_TDTR_TGT_Pos))

#define CAN_TDTR_TIME_Pos       ((uint32_t)16U)
#define CAN_TDTR_TIME_Msk       ((uint32_t)(0xFFFFUL << CAN_TDTR_TIME_Pos))

/*===========================================================================*/
/* RX FIFO mailbox identifier register (RIR) bitfield definitions            */
/* Source: RM0008 §24.5                                                      */
/*===========================================================================*/

#define CAN_RIR_RTR_Pos         ((uint32_t)1U)
#define CAN_RIR_RTR_Msk         ((uint32_t)(1UL << CAN_RIR_RTR_Pos))

#define CAN_RIR_IDE_Pos         ((uint32_t)2U)
#define CAN_RIR_IDE_Msk         ((uint32_t)(1UL << CAN_RIR_IDE_Pos))

#define CAN_RIR_EXID_Pos        ((uint32_t)3U)
#define CAN_RIR_EXID_Msk        ((uint32_t)(0x3FFFFUL << CAN_RIR_EXID_Pos))

#define CAN_RIR_STID_Pos        ((uint32_t)21U)
#define CAN_RIR_STID_Msk        ((uint32_t)(0x7FFUL << CAN_RIR_STID_Pos))

/*===========================================================================*/
/* RX FIFO mailbox data length register (RDTR) bitfield definitions          */
/* Source: RM0008 §24.5                                                      */
/*===========================================================================*/

#define CAN_RDTR_DLC_Pos        ((uint32_t)0U)
#define CAN_RDTR_DLC_Msk        ((uint32_t)(0xFUL << CAN_RDTR_DLC_Pos))

#define CAN_RDTR_FMI_Pos        ((uint32_t)8U)
#define CAN_RDTR_FMI_Msk        ((uint32_t)(0xFFUL << CAN_RDTR_FMI_Pos))

#define CAN_RDTR_TIME_Pos       ((uint32_t)16U)
#define CAN_RDTR_TIME_Msk       ((uint32_t)(0xFFFFUL << CAN_RDTR_TIME_Pos))

/*===========================================================================*/
/* RCC register addresses for CAN clock enable                               */
/* Source: RM0008 §7.3.8 p.111; ir/can_ir_summary.json clock[]              */
/*===========================================================================*/

#ifndef RCC_BASE_ADDR
#define RCC_BASE_ADDR           ((uint32_t)0x40021000UL)
#endif

#ifndef RCC_APB1ENR
#define RCC_APB1ENR             (*((__IO uint32_t *)(RCC_BASE_ADDR + 0x1CUL)))
#endif

#ifndef RCC_APB1RSTR
#define RCC_APB1RSTR            (*((__IO uint32_t *)(RCC_BASE_ADDR + 0x10UL)))
#endif

/* CAN1 on APB1, bit 25 */
#define RCC_APB1ENR_CAN1EN_Pos  ((uint32_t)25U)
#define RCC_APB1ENR_CAN1EN_Msk  ((uint32_t)(1UL << RCC_APB1ENR_CAN1EN_Pos))

#define RCC_APB1RSTR_CAN1RST_Pos ((uint32_t)25U)
#define RCC_APB1RSTR_CAN1RST_Msk ((uint32_t)(1UL << RCC_APB1RSTR_CAN1RST_Pos))

/*===========================================================================*/
/* AFIO remap register for CAN1 pin remapping                                */
/* Source: RM0008 §9.3.10; ir/can_ir_summary.json configuration_strategies[] */
/*===========================================================================*/

#ifndef AFIO_BASE_ADDR
#define AFIO_BASE_ADDR          ((uint32_t)0x40010000UL)
#endif

#ifndef AFIO_MAPR
#define AFIO_MAPR               (*((__IO uint32_t *)(AFIO_BASE_ADDR + 0x04UL)))
#endif

#ifndef RCC_APB2ENR
#define RCC_APB2ENR             (*((__IO uint32_t *)(RCC_BASE_ADDR + 0x18UL)))
#endif

#ifndef RCC_APB2ENR_AFIOEN_Pos
#define RCC_APB2ENR_AFIOEN_Pos  ((uint32_t)0U)
#define RCC_APB2ENR_AFIOEN_Msk  ((uint32_t)(1UL << RCC_APB2ENR_AFIOEN_Pos))
#endif

/* CAN remap: MAPR[14:13] */
/* 00 = default (PA11/PA12), 10 = partial (PB8/PB9), 11 = full (PD0/PD1) */
#define AFIO_MAPR_CAN_REMAP_Pos  ((uint32_t)13U)
#define AFIO_MAPR_CAN_REMAP_Msk  ((uint32_t)(0x3UL << AFIO_MAPR_CAN_REMAP_Pos))

#endif /* CAN_REG_H */
