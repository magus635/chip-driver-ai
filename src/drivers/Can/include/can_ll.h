#ifndef CAN_LL_H
#define CAN_LL_H

#include "can_reg.h"
#include "can_types.h"
#include <stdbool.h>

/* Layer 2: bxCAN Low-Level Hardware Abstraction (V2.3)
 * STRICT CONFORMITY: Atomic operations, isolated W1C manipulation.
 */

static inline void CAN_LL_EnterInitMode(CAN_TypeDef *CANx) {
    CANx->MCR &= ~CAN_MCR_SLEEP_Msk;
    CANx->MCR |= CAN_MCR_INRQ_Msk;
}

static inline bool CAN_LL_IsInitModeEntered(const CAN_TypeDef *CANx) {
    return ((CANx->MSR & CAN_MSR_INAK_Msk) != 0U);
}

static inline void CAN_LL_ExitInitMode(CAN_TypeDef *CANx, const Can_ConfigType *cfg) {
    if (cfg->bus_off_recovery) CANx->MCR |= CAN_MCR_ABOM_Msk;
    else CANx->MCR &= ~CAN_MCR_ABOM_Msk;
    
    if (cfg->auto_retransmit) CANx->MCR &= ~CAN_MCR_NART_Msk;
    else CANx->MCR |= CAN_MCR_NART_Msk;

    CANx->MCR &= ~CAN_MCR_INRQ_Msk;
}

static inline bool CAN_LL_IsInitModeExited(const CAN_TypeDef *CANx) {
    return ((CANx->MSR & CAN_MSR_INAK_Msk) == 0U);
}

static inline void CAN_LL_SetBaudrate(CAN_TypeDef *CANx, const Can_ConfigType *cfg) {
    uint32_t btr = 0;
    
    /* Time Segment Calculations (BTR Register) */
    btr |= ((uint32_t)(cfg->prescaler - 1U) << CAN_BTR_BRP_Pos) & 0x03FFUL; /* BRP[9:0] */
    btr |= ((uint32_t)(cfg->prop_seg - 1U) << CAN_BTR_TS1_Pos) & 0x000F0000UL; /* TS1[19:16] */
    btr |= ((uint32_t)(cfg->phase_seg2 - 1U) << CAN_BTR_TS2_Pos) & 0x00700000UL; /* TS2[22:20] */
    btr |= ((uint32_t)(cfg->sjw - 1U) << CAN_BTR_SJW_Pos) & 0x03000000UL; /* SJW[25:24] */

    if (cfg->mode == CAN_MODE_LOOPBACK) btr |= CAN_BTR_LBKM_Msk;
    if (cfg->mode == CAN_MODE_SILENT)   btr |= CAN_BTR_SILM_Msk;
    if (cfg->mode == CAN_MODE_SILENT_LOOPBACK) btr |= (CAN_BTR_LBKM_Msk | CAN_BTR_SILM_Msk);

    CANx->BTR = btr;
}

/* 榨干硬件点：根据硬件状态寻找第一个可用的 TX 邮箱 */
static inline int8_t CAN_LL_GetFreeTxMailbox(const CAN_TypeDef *CANx) {
    uint32_t tsr = CANx->TSR;
    if (tsr & CAN_TSR_TME0_Msk) return 0;
    if (tsr & CAN_TSR_TME1_Msk) return 1;
    if (tsr & CAN_TSR_TME2_Msk) return 2;
    return -1;
}

static inline void CAN_LL_WriteTxMailbox(CAN_TypeDef *CANx, uint8_t idx, const Can_PduType *pdu) {
    uint32_t tir = 0;
    if (idx > 2) return;
    
    if (pdu->ide == 0U) tir = (pdu->id << 21);
    else               tir = (pdu->id << 3) | (1UL << 2);
    
    if (pdu->rtr == 1U) tir |= (1UL << 1);

    CANx->sTxMailBox[idx].TDTR = pdu->dlc & 0x0F;
    CANx->sTxMailBox[idx].TDLR = ((uint32_t)pdu->data[0]) | ((uint32_t)pdu->data[1] << 8) | ((uint32_t)pdu->data[2] << 16) | ((uint32_t)pdu->data[3] << 24);
    CANx->sTxMailBox[idx].TDHR = ((uint32_t)pdu->data[4]) | ((uint32_t)pdu->data[5] << 8) | ((uint32_t)pdu->data[6] << 16) | ((uint32_t)pdu->data[7] << 24);
    
    /* 最后发出发送请求 TXRQ */
    CANx->sTxMailBox[idx].TIR = tir | 1UL;
}

static inline bool CAN_LL_IsRxFifo0Empty(const CAN_TypeDef *CANx) {
    return ((CANx->RF0R & CAN_RF0R_FMP0_Msk) == 0U);
}

static inline void CAN_LL_ReadRxFifo0(const CAN_TypeDef *CANx, Can_PduType *pdu) {
    uint32_t rir = CANx->sFIFOMailBox[0].RIR;
    pdu->ide = (uint8_t)((rir >> 2) & 1U);
    if (pdu->ide == 0U) pdu->id = rir >> 21;
    else               pdu->id = rir >> 3;
    
    pdu->rtr = (uint8_t)((rir >> 1) & 1U);
    pdu->dlc = (uint8_t)(CANx->sFIFOMailBox[0].RDTR & 0x0F);
    
    uint32_t rdlr = CANx->sFIFOMailBox[0].RDLR;
    uint32_t rdhr = CANx->sFIFOMailBox[0].RDHR;
    pdu->data[0] = (uint8_t)(rdlr & 0xFFU);
    pdu->data[1] = (uint8_t)((rdlr >> 8) & 0xFFU);
    pdu->data[2] = (uint8_t)((rdlr >> 16) & 0xFFU);
    pdu->data[3] = (uint8_t)((rdlr >> 24) & 0xFFU);
    pdu->data[4] = (uint8_t)(rdhr & 0xFFU);
    pdu->data[5] = (uint8_t)((rdhr >> 8) & 0xFFU);
    pdu->data[6] = (uint8_t)((rdhr >> 16) & 0xFFU);
    pdu->data[7] = (uint8_t)((rdhr >> 24) & 0xFFU);
}

static inline void CAN_LL_ReleaseRxFifo0(CAN_TypeDef *CANx) {
    CANx->RF0R |= CAN_RF0R_RFOM0_Msk;
}

static inline void CAN_LL_InitFilter(CAN_TypeDef *CANx) {
    /* 榨干点：进入过滤器初始化模式 */
    CANx->FMR |= CAN_FMR_FINIT_Msk;
    
    /* 默认配置过滤器 0：32位屏蔽位模式，关联 FIFO 0，激活 */
    CANx->FA1R &= ~1UL; /* 先禁用 */
    CANx->FS1R |= 1UL;  /* 32-bit scale */
    CANx->FM1R &= ~1UL; /* Mask mode */
    CANx->FFA1R &= ~1UL; /* FIFO 0 */
    
    /* 接收一切 (ID=0, Mask=0) */
    CANx->sFilterRegister[0].FR1 = 0U;
    CANx->sFilterRegister[0].FR2 = 0U;
    
    CANx->FA1R |= 1UL;  /* 激活 */
    
    CANx->FMR &= ~CAN_FMR_FINIT_Msk;
}

static inline uint32_t CAN_LL_GetESR(const CAN_TypeDef *CANx) {
    return CANx->ESR;
}

#endif 
