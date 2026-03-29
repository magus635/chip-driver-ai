#ifndef CAN_REG_H
#define CAN_REG_H

#include <stdint.h>

/* Layer 1: bxCAN Register Mapping (STM32F407)
 * STRICT CONFORMITY: No business logic.
 */

#define __I  volatile const
#define __O  volatile
#define __IO volatile

typedef struct {
    __IO uint32_t TIR;    /* [31:21] STDID, [20:3] EXID, [2] IDE, [1] RTR, [0] TXRQ */
    __IO uint32_t TDTR;   /* [3:0] DLC */
    __IO uint32_t TDLR;   /* Data low bytes [0..3] */
    __IO uint32_t TDHR;   /* Data high bytes [4..7] */
} CAN_TxMailBox_TypeDef;

typedef struct {
    __I uint32_t RIR;    /* [31:21] STDID, [20:3] EXID, [2] IDE, [1] RTR */
    __I uint32_t RDTR;   /* [3:0] DLC */
    __I uint32_t RDLR;   /* Data low bytes [0..3] */
    __I uint32_t RDHR;   /* Data high bytes [4..7] */
} CAN_FIFOMailBox_TypeDef;

typedef struct {
    __IO uint32_t FR1;   /* Filter Register 1 */
    __IO uint32_t FR2;   /* Filter Register 2 */
} CAN_FilterRegister_TypeDef;

typedef struct {
    __IO uint32_t MCR;   /* [15] RESET, [6] ABOM, [5] AWUM, [4] NART, [3] RFLM, [2] TXFP, [1] SLEEP, [0] INRQ */
    __IO uint32_t MSR;   /* [1] SLAK, [0] INAK */
    __IO uint32_t TSR;   /* [28] TME2, [27] TME1, [26] TME0, [1] TXOK0, [0] RQCP0... */
    __IO uint32_t RF0R;  /* [5] RFOM0, [3] FULL0, [1:0] FMP0 */
    __IO uint32_t RF1R;  /* [5] RFOM1... */
    __IO uint32_t IER;   /* [15] ERRIE, [10] BOFIE... */
    __IO uint32_t ESR;   /* [31:24] REC, [23:16] TEC, [6:4] LEC, [2] BOFF, [1] EPVF, [0] EWGF */
    __IO uint32_t BTR;   /* [31] SILM, [30] LBKM, [25:24] SJW, [22:20] TS2, [19:16] TS1, [9:0] BRP */
    uint32_t RESERVED0[88];
    CAN_TxMailBox_TypeDef sTxMailBox[3];
    CAN_FIFOMailBox_TypeDef sFIFOMailBox[2];
    uint32_t RESERVED1[12];
    __IO uint32_t FMR;   /* [0] FINIT, [13:8] CAN2SB */
    __IO uint32_t FM1R;  /* [27:0] FBMx (0:Mask mode, 1:List mode) */
    uint32_t RESERVED2;
    __IO uint32_t FS1R;  /* [27:0] FSCx (0:16bit, 1:32bit) */
    uint32_t RESERVED3;
    __IO uint32_t FFA1R; /* [27:0] FFAx (0:FIFO0, 1:FIFO1) */
    uint32_t RESERVED4;
    __IO uint32_t FA1R;  /* [27:0] FACTx (0:Disabled, 1:Active) */
    uint32_t RESERVED5[8];
    CAN_FilterRegister_TypeDef sFilterRegister[28];
} CAN_TypeDef;

#define CAN1_BASE (0x40006400UL)
#define CAN1      ((CAN_TypeDef *)CAN1_BASE)

/* MCR Bit Masks */
#define CAN_MCR_INRQ_Msk   (1UL << 0)
#define CAN_MCR_SLEEP_Msk  (1UL << 1)
#define CAN_MCR_NART_Msk   (1UL << 4)
#define CAN_MCR_ABOM_Msk   (1UL << 6)
#define CAN_MCR_RESET_Msk  (1UL << 15)

/* MSR Bit Masks */
#define CAN_MSR_INAK_Msk   (1UL << 0)
#define CAN_MSR_SLAK_Msk   (1UL << 1)

/* TSR Bit Masks */
#define CAN_TSR_TME0_Msk   (1UL << 26)
#define CAN_TSR_TME1_Msk   (1UL << 27)
#define CAN_TSR_TME2_Msk   (1UL << 28)

/* RF0R Bits */
#define CAN_RF0R_FMP0_Msk  (0x3UL << 0)
#define CAN_RF0R_RFOM0_Msk (1UL << 5)

/* ESR Bit Masks (榨干模式监控点) */
#define CAN_ESR_EWGF_Msk   (1UL << 0)
#define CAN_ESR_EPVF_Msk   (1UL << 1)
#define CAN_ESR_BOFF_Msk   (1UL << 2)
#define CAN_ESR_LEC_Msk    (7UL << 4)
#define CAN_ESR_TEC_Msk    (0xFFUL << 16)
#define CAN_ESR_REC_Msk    (0xFFUL << 24)

/* BTR Bits (榨干波特率配置) */
#define CAN_BTR_BRP_Pos    (0U)
#define CAN_BTR_TS1_Pos    (16U)
#define CAN_BTR_TS2_Pos    (20U)
#define CAN_BTR_SJW_Pos    (24U)
#define CAN_BTR_LBKM_Msk   (1UL << 30)
#define CAN_BTR_SILM_Msk   (1UL << 31)

#define CAN_FMR_FINIT_Msk  (1UL << 0)

#endif 
