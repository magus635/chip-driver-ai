/**
 * @file    can_ll.h
 * @brief   STM32F103C8T6 CAN LL 层 (Layer 2 - Atomic Ops & Composition)
 *
 * Source:  ir/can_ir_summary.json v3.0
 */

#ifndef CAN_LL_H
#define CAN_LL_H

#include "can_reg.h"
#include "can_types.h"

/*===========================================================================*/
/* Mode Control                                                              */
/*===========================================================================*/

static inline void CAN_LL_EnterInitMode(CAN_TypeDef *CANx)
{
    CANx->MCR |= CAN_MCR_INRQ_Msk;
    CANx->MCR &= ~CAN_MCR_SLEEP_Msk;
}

static inline void CAN_LL_ExitInitMode(CAN_TypeDef *CANx)
{
    CANx->MCR &= ~CAN_MCR_INRQ_Msk;
}

static inline bool CAN_LL_IsInInitMode(const CAN_TypeDef *CANx)
{
    return ((CANx->MSR & CAN_MSR_INAK_Msk) != 0U);
}

/*===========================================================================*/
/* Filter Control                                                            */
/*===========================================================================*/

static inline void CAN_LL_EnterFilterInit(CAN_TypeDef *CANx)
{
    CANx->FMR |= CAN_FMR_FINIT_Msk;
}

static inline void CAN_LL_ExitFilterInit(CAN_TypeDef *CANx)
{
    CANx->FMR &= ~CAN_FMR_FINIT_Msk;
}

static inline void CAN_LL_ActivateFilter(CAN_TypeDef *CANx, uint8_t bank)
{
    CANx->FA1R |= (1UL << bank);
}

static inline void CAN_LL_DeactivateFilter(CAN_TypeDef *CANx, uint8_t bank)
{
    CANx->FA1R &= ~(1UL << bank);
}

/*===========================================================================*/
/* Register Access Guards (R3 compliance)                                    */
/*===========================================================================*/

static inline void CAN_LL_WriteBTR(CAN_TypeDef *CANx, uint32_t val)
{
    /* Guard: INV_CAN_001 — Must be in Init mode */
    if (!CAN_LL_IsInInitMode(CANx)) { return; }
    CANx->BTR = val;
}

static inline void CAN_LL_WriteMCR(CAN_TypeDef *CANx, uint32_t val)
{
    CANx->MCR = val;
}

/*===========================================================================*/
/* Composition Helpers (Implemented in can_ll.c)                             */
/*===========================================================================*/

uint32_t CAN_LL_ComposeBTR(const Can_ConfigType *pConfig);
uint32_t CAN_LL_ComposeMCR(const Can_ConfigType *pConfig);

static inline void CAN_LL_WriteTxMailbox(CAN_TypeDef *CANx, uint8_t mb, uint32_t tir, uint32_t tdtr, uint32_t tdlr, uint32_t tdhr)
{
    CANx->sTxMailBox[mb].TDTR = tdtr;
    CANx->sTxMailBox[mb].TDLR = tdlr;
    CANx->sTxMailBox[mb].TDHR = tdhr;
    CANx->sTxMailBox[mb].TIR = tir | CAN_TIR_TXRQ_Msk;
}

static inline void CAN_LL_ReadRxMailbox(const CAN_TypeDef *CANx, uint8_t fifo, uint32_t *rir, uint32_t *rdtr, uint32_t *rdlr, uint32_t *rdhr)
{
    *rir = CANx->sFIFOMailBox[fifo].RIR;
    *rdtr = CANx->sFIFOMailBox[fifo].RDTR;
    *rdlr = CANx->sFIFOMailBox[fifo].RDLR;
    *rdhr = CANx->sFIFOMailBox[fifo].RDHR;
}

static inline void CAN_LL_ReleaseFifo(CAN_TypeDef *CANx, uint8_t fifo)
{
    /* Rule #4: Avoid using bitwise OR-Assignment on W1C registers (RF0R/RF1R contain W1C bits like FOVR) */
    if (fifo == 0) { CANx->RF0R = CAN_RF0R_RFOM0_Msk; }
    else { CANx->RF1R = CAN_RF1R_RFOM1_Msk; }
}

static inline uint32_t CAN_LL_GetFifoStatus(const CAN_TypeDef *CANx, uint8_t fifo)
{
    return (fifo == 0) ? CANx->RF0R : CANx->RF1R;
}

static inline uint32_t CAN_LL_GetTSR(const CAN_TypeDef *CANx)
{
    return CANx->TSR;
}

static inline void CAN_LL_EnterFilterInitMode(CAN_TypeDef *CANx)
{
    CANx->FMR |= CAN_FMR_FINIT_Msk;
}

static inline void CAN_LL_ExitFilterInitMode(CAN_TypeDef *CANx)
{
    CANx->FMR &= ~CAN_FMR_FINIT_Msk;
}

static inline void CAN_LL_ConfigFilter(CAN_TypeDef *CANx, uint8_t bank, uint32_t fr1, uint32_t fr2, uint32_t mode, uint32_t scale, uint32_t fifo)
{
    /* Rule #4 Warning: FMR is already handled by Enter/Exit functions. 
       FA1R/FM1R/FS1R/FFA1R contain W1C or activation bits. */
    uint32_t mask = (1UL << bank);
    
    if (mode)  CANx->FM1R |= mask;  else CANx->FM1R &= ~mask;
    if (scale) CANx->FS1R |= mask;  else CANx->FS1R &= ~mask;
    if (fifo)  CANx->FFA1R |= mask; else CANx->FFA1R &= ~mask;
    
    CANx->sFilterBank[bank].FR1 = fr1;
    CANx->sFilterBank[bank].FR2 = fr2;
    
    CANx->FA1R |= mask; /* Activate filter */
}

#endif /* CAN_LL_H */
