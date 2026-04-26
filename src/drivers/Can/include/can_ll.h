/**
 * @file    can_ll.h
 * @brief   STM32F103C8T6 CAN 硬件抽象底端层 (Low-Level)
 *
 * @note    提供原子级硬件操作的 static inline 函数。
 *          W1C 寄存器副作用（MSR.ERRI/WKUI/SLAKI, TSR.RQCP0/1/2,
 *          RF0R.FULL0/FOVR0, RF1R.FULL1/FOVR1）完全封装在此层。
 *          禁止 include can_drv.h 或 can_api.h（规则 R8-8）。
 *
 * Source:  RM0008 Rev 14, Chapter 24
 *          ir/can_ir_summary.json v2.0
 */

#ifndef CAN_LL_H
#define CAN_LL_H

#include "can_reg.h"
#include "can_types.h"
#include "can_cfg.h"

#include <stddef.h>
#include <stdbool.h>

/*===========================================================================*/
/* Non-inline composite init helpers (implemented in can_ll.c)               */
/*===========================================================================*/

/**
 * @brief Configure MCR operational bits (TXFP, RFLM, NART, AWUM, ABOM, TTCM, DBF).
 *        Guard: INV_CAN_001 — caller must ensure INAK=1.
 * @param[in] CANx    Pointer to CAN peripheral.
 * @param[in] pConfig Configuration structure. Must not be NULL.
 */
void CAN_LL_ComposeMCR(CAN_TypeDef *CANx, const Can_ConfigType *pConfig);

/**
 * @brief Configure BTR with bit timing and operation mode in a single write.
 *        Guard: INV_CAN_001 — BTR writable only in init mode (INAK=1).
 * @param[in] CANx    Pointer to CAN peripheral.
 * @param[in] pConfig Configuration with bit timing and mode. Must not be NULL.
 */
void CAN_LL_ComposeBTR(CAN_TypeDef *CANx, const Can_ConfigType *pConfig);

/*===========================================================================*/
/* RCC clock enable / disable / reset                                        */
/* Source: RM0008 §7.3.8 p.111; ir/can_ir_summary.json clock[]              */
/*===========================================================================*/

/** @brief Enable CAN1 peripheral clock on APB1. */
static inline void CAN_LL_EnableClock(CAN_TypeDef *CANx)
{
    (void)CANx; /* Only CAN1 exists on STM32F103C8T6 */
    RCC_APB1ENR |= RCC_APB1ENR_CAN1EN_Msk; /* RM0008 §7.3.8 p.111 */
}

/** @brief Disable CAN1 peripheral clock on APB1. */
static inline void CAN_LL_DisableClock(CAN_TypeDef *CANx)
{
    (void)CANx;
    RCC_APB1ENR &= ~RCC_APB1ENR_CAN1EN_Msk; /* RM0008 §7.3.8 p.111 */
}

/** @brief Reset CAN1 peripheral via RCC. */
static inline void CAN_LL_ResetPeripheral(CAN_TypeDef *CANx)
{
    (void)CANx;
    RCC_APB1RSTR |= RCC_APB1RSTR_CAN1RST_Msk;
    RCC_APB1RSTR &= ~RCC_APB1RSTR_CAN1RST_Msk;
}

/*===========================================================================*/
/* GPIO pin remap                                                             */
/* Source: ir/can_ir_summary.json configuration_strategies[] PIN_REMAP        */
/*===========================================================================*/

/**
 * @brief Set CAN1 GPIO pin remapping via AFIO_MAPR[14:13].
 * @param[in] remap  Remap selection.
 */
static inline void CAN_LL_SetPinRemap(Can_PinRemap_e remap)
{
    uint32_t mapr;

    /* Enable AFIO clock — RM0008 §7.3.7 p.111 */
    RCC_APB2ENR |= RCC_APB2ENR_AFIOEN_Msk;

    mapr = AFIO_MAPR;
    mapr &= ~AFIO_MAPR_CAN_REMAP_Msk;
    mapr |= (((uint32_t)remap << AFIO_MAPR_CAN_REMAP_Pos) & AFIO_MAPR_CAN_REMAP_Msk);
    AFIO_MAPR = mapr; /* RM0008 §9.3.10 */
}

/*===========================================================================*/
/* MCR — Initialization mode entry/exit                                      */
/* Source: RM0008 §24.4.1; ir/can_ir_summary.json atomic_sequences[]         */
/* Guard: INV_CAN_001 — BTR writable only when INRQ=1 (INAK acknowledged)   */
/*===========================================================================*/

/**
 * @brief Request initialization mode (set INRQ, clear SLEEP).
 *        SEQ_CAN_INIT_MODE step 1 — RM0008 §24.4.1
 * @param[in] CANx  Pointer to CAN peripheral.
 */
static inline void CAN_LL_RequestInitMode(CAN_TypeDef *CANx)
{
    uint32_t mcr = CANx->MCR;
    mcr |= CAN_MCR_INRQ_Msk;
    mcr &= ~CAN_MCR_SLEEP_Msk; /* Must exit sleep to enter init */
    CANx->MCR = mcr; /* IR: atomic_sequences[0] step 1 — RM0008 §24.4.1 */
}

/**
 * @brief Check if initialization mode is acknowledged (INAK=1).
 *        SEQ_CAN_INIT_MODE step 2 — RM0008 §24.4.1
 * @param[in] CANx  Pointer to CAN peripheral.
 * @return true if INAK=1.
 */
static inline bool CAN_LL_IsInitAck(const CAN_TypeDef *CANx)
{
    return ((CANx->MSR & CAN_MSR_INAK_Msk) != 0U); /* RM0008 §24.4.1 */
}

/**
 * @brief Request normal operation mode (clear INRQ).
 *        SEQ_CAN_EXIT_INIT_MODE step 1 — RM0008 §24.4.1
 * @param[in] CANx  Pointer to CAN peripheral.
 */
static inline void CAN_LL_RequestNormalMode(CAN_TypeDef *CANx)
{
    CANx->MCR &= ~CAN_MCR_INRQ_Msk; /* IR: atomic_sequences[1] step 1 — RM0008 §24.4.1 */
}

/**
 * @brief Check if CAN is in sleep mode (SLAK=1).
 * @param[in] CANx  Pointer to CAN peripheral.
 * @return true if SLAK=1.
 */
static inline bool CAN_LL_IsSleepAck(const CAN_TypeDef *CANx)
{
    return ((CANx->MSR & CAN_MSR_SLAK_Msk) != 0U); /* RM0008 §24.4.1 */
}

/**
 * @brief Request sleep mode (set SLEEP, clear INRQ).
 * @param[in] CANx  Pointer to CAN peripheral.
 */
static inline void CAN_LL_RequestSleepMode(CAN_TypeDef *CANx)
{
    uint32_t mcr = CANx->MCR;
    mcr &= ~CAN_MCR_INRQ_Msk;
    mcr |= CAN_MCR_SLEEP_Msk;
    CANx->MCR = mcr; /* RM0008 §24.4.1 */
}

/**
 * @brief Request wakeup from sleep (clear SLEEP).
 * @param[in] CANx  Pointer to CAN peripheral.
 */
static inline void CAN_LL_RequestWakeup(CAN_TypeDef *CANx)
{
    CANx->MCR &= ~CAN_MCR_SLEEP_Msk; /* RM0008 §24.4.1 */
}

/*===========================================================================*/
/* MCR — Configuration bits (inline helpers for individual bits)             */
/* Composite MCR/BTR configuration: see CAN_LL_ComposeMCR / CAN_LL_ComposeBTR */
/* declared above, implemented in can_ll.c                                   */
/*===========================================================================*/

/*===========================================================================*/
/* MSR — Status flag reads and W1C clears                                    */
/* Source: RM0008 §24.4.1; ir/can_ir_summary.json registers[1]               */
/* NOTE: ERRI/WKUI/SLAKI are RC_W1 — direct write to clear, NO |=          */
/*===========================================================================*/

/** @brief Return raw MSR value. */
static inline uint32_t CAN_LL_ReadMSR(const CAN_TypeDef *CANx)
{
    return CANx->MSR; /* RM0008 §24.4.1 */
}

/**
 * @brief Clear ERRI flag (RC_W1: write 1 to bit 2 to clear).
 *        W1C safe pattern: write only the target bit, other W1C bits written as 0.
 * @param[in] CANx  Pointer to CAN peripheral.
 */
static inline void CAN_LL_ClearERRI(CAN_TypeDef *CANx)
{
    CANx->MSR = CAN_MSR_ERRI_Msk; /* W1C — RM0008 §24.4.1 */
}

/**
 * @brief Clear WKUI flag (RC_W1).
 * @param[in] CANx  Pointer to CAN peripheral.
 */
static inline void CAN_LL_ClearWKUI(CAN_TypeDef *CANx)
{
    CANx->MSR = CAN_MSR_WKUI_Msk; /* W1C — RM0008 §24.4.1 */
}

/**
 * @brief Clear SLAKI flag (RC_W1).
 * @param[in] CANx  Pointer to CAN peripheral.
 */
static inline void CAN_LL_ClearSLAKI(CAN_TypeDef *CANx)
{
    CANx->MSR = CAN_MSR_SLAKI_Msk; /* W1C — RM0008 §24.4.1 */
}

/*===========================================================================*/
/* TSR — Transmit status                                                      */
/* Source: RM0008 §24.4.2; ir/can_ir_summary.json registers[2]               */
/* NOTE: RQCP0/1/2 are RC_W1 — direct write to clear, NO |=                */
/*===========================================================================*/

/** @brief Return raw TSR value. */
static inline uint32_t CAN_LL_ReadTSR(const CAN_TypeDef *CANx)
{
    return CANx->TSR; /* RM0008 §24.4.2 */
}

/**
 * @brief Check if TX mailbox is empty (TMEx=1).
 *        INV_CAN_006: TMEx=1 implies can_transmit_mailbox_x
 * @param[in] CANx    Pointer to CAN peripheral.
 * @param[in] mailbox Mailbox index (0-2).
 * @return true if mailbox is empty.
 */
static inline bool CAN_LL_IsMailboxEmpty(const CAN_TypeDef *CANx, uint8_t mailbox)
{
    uint32_t tsr = CANx->TSR;
    uint32_t tmeMsk = (CAN_TSR_TME0_Msk << mailbox); /* TME0=bit26, TME1=bit27, TME2=bit28 */
    return ((tsr & tmeMsk) != 0U); /* RM0008 §24.4.2 */
}

/**
 * @brief Get the next empty mailbox code from TSR.CODE[1:0].
 * @param[in] CANx  Pointer to CAN peripheral.
 * @return Mailbox index (0-2), or CAN_TX_MAILBOX_NONE if all full.
 */
static inline uint8_t CAN_LL_GetFreeMailbox(const CAN_TypeDef *CANx)
{
    uint32_t tsr = CANx->TSR;
    uint8_t result = CAN_TX_MAILBOX_NONE;

    /* Check if at least one mailbox is empty */
    if ((tsr & (CAN_TSR_TME0_Msk | CAN_TSR_TME1_Msk | CAN_TSR_TME2_Msk)) != 0U)
    {
        result = (uint8_t)((tsr >> CAN_TSR_CODE_Pos) & 0x3U); /* RM0008 §24.4.2 */
    }

    return result;
}

/**
 * @brief Clear RQCP flag for a mailbox (RC_W1).
 *        W1C safe pattern: write only the target bit.
 * @param[in] CANx    Pointer to CAN peripheral.
 * @param[in] mailbox Mailbox index (0-2).
 */
static inline void CAN_LL_ClearRQCP(CAN_TypeDef *CANx, uint8_t mailbox)
{
    /* RQCP0=bit0, RQCP1=bit8, RQCP2=bit16 — offset by 8 bits each */
    uint32_t rqcpMsk = (CAN_TSR_RQCP0_Msk << ((uint32_t)mailbox * 8U));
    CANx->TSR = rqcpMsk; /* W1C — RM0008 §24.4.2 */
}

/**
 * @brief Check if transmission was successful for a mailbox (TXOK=1).
 * @param[in] CANx    Pointer to CAN peripheral.
 * @param[in] mailbox Mailbox index (0-2).
 * @return true if TXOK is set.
 */
static inline bool CAN_LL_IsTxOk(const CAN_TypeDef *CANx, uint8_t mailbox)
{
    uint32_t tsr = CANx->TSR;
    uint32_t txokMsk = (CAN_TSR_TXOK0_Msk << ((uint32_t)mailbox * 8U));
    return ((tsr & txokMsk) != 0U); /* RM0008 §24.4.2 */
}

/**
 * @brief Check if request is complete for a mailbox (RQCP=1).
 * @param[in] CANx    Pointer to CAN peripheral.
 * @param[in] mailbox Mailbox index (0-2).
 * @return true if RQCP is set.
 */
static inline bool CAN_LL_IsRequestComplete(const CAN_TypeDef *CANx, uint8_t mailbox)
{
    uint32_t tsr = CANx->TSR;
    uint32_t rqcpMsk = (CAN_TSR_RQCP0_Msk << ((uint32_t)mailbox * 8U));
    return ((tsr & rqcpMsk) != 0U); /* RM0008 §24.4.2 */
}

/**
 * @brief Abort transmission for a mailbox (set ABRQ).
 * @param[in] CANx    Pointer to CAN peripheral.
 * @param[in] mailbox Mailbox index (0-2).
 */
static inline void CAN_LL_AbortMailbox(CAN_TypeDef *CANx, uint8_t mailbox)
{
    /* ABRQ0=bit7, ABRQ1=bit15, ABRQ2=bit23 — offset by 8 bits */
    /* TSR contains W1C bits (RQCP0/1/2) — must NOT use RMW (R8-1) */
    /* Direct write: only ABRQ bit set, RQCP bits written as 0 (safe for W1C) */
    uint32_t abrqMsk = (CAN_TSR_ABRQ0_Msk << ((uint32_t)mailbox * 8U));
    CANx->TSR = abrqMsk; /* W1C-safe direct write — RM0008 §24.4.2 */
}

/*===========================================================================*/
/* RF0R / RF1R — Receive FIFO status                                         */
/* Source: RM0008 §24.4.3; ir/can_ir_summary.json registers[3-4]             */
/* NOTE: FULL/FOVR are RC_W1 — direct write to clear, NO |=                */
/*===========================================================================*/

/**
 * @brief Get number of pending messages in a FIFO.
 *        INV_CAN_007: FMP>0 implies can_read_fifo
 * @param[in] CANx      Pointer to CAN peripheral.
 * @param[in] fifoIndex FIFO index (0 or 1).
 * @return Number of pending messages (0-3).
 */
static inline uint8_t CAN_LL_GetFifoPending(const CAN_TypeDef *CANx, uint8_t fifoIndex)
{
    uint32_t rfr;
    if (fifoIndex == CAN_RX_FIFO_0)
    {
        rfr = CANx->RF0R; /* RM0008 §24.4.3 */
    }
    else
    {
        rfr = CANx->RF1R; /* RM0008 §24.4.3 */
    }
    return (uint8_t)(rfr & 0x3U);
}

/**
 * @brief Release the output mailbox of a FIFO (write RFOM).
 * @param[in] CANx      Pointer to CAN peripheral.
 * @param[in] fifoIndex FIFO index (0 or 1).
 */
static inline void CAN_LL_ReleaseFifo(CAN_TypeDef *CANx, uint8_t fifoIndex)
{
    if (fifoIndex == CAN_RX_FIFO_0)
    {
        CANx->RF0R = CAN_RF0R_RFOM0_Msk; /* WO — RM0008 §24.4.3 */
    }
    else
    {
        CANx->RF1R = CAN_RF1R_RFOM1_Msk; /* WO — RM0008 §24.4.3 */
    }
}

/**
 * @brief Check if FIFO is full.
 * @param[in] CANx      Pointer to CAN peripheral.
 * @param[in] fifoIndex FIFO index (0 or 1).
 * @return true if FIFO full flag is set.
 */
static inline bool CAN_LL_IsFifoFull(const CAN_TypeDef *CANx, uint8_t fifoIndex)
{
    if (fifoIndex == CAN_RX_FIFO_0)
    {
        return ((CANx->RF0R & CAN_RF0R_FULL0_Msk) != 0U);
    }
    else
    {
        return ((CANx->RF1R & CAN_RF1R_FULL1_Msk) != 0U);
    }
}

/**
 * @brief Check if FIFO overrun occurred.
 * @param[in] CANx      Pointer to CAN peripheral.
 * @param[in] fifoIndex FIFO index (0 or 1).
 * @return true if overrun flag is set.
 */
static inline bool CAN_LL_IsFifoOverrun(const CAN_TypeDef *CANx, uint8_t fifoIndex)
{
    if (fifoIndex == CAN_RX_FIFO_0)
    {
        return ((CANx->RF0R & CAN_RF0R_FOVR0_Msk) != 0U);
    }
    else
    {
        return ((CANx->RF1R & CAN_RF1R_FOVR1_Msk) != 0U);
    }
}

/**
 * @brief Clear FIFO full flag (RC_W1).
 * @param[in] CANx      Pointer to CAN peripheral.
 * @param[in] fifoIndex FIFO index (0 or 1).
 */
static inline void CAN_LL_ClearFifoFull(CAN_TypeDef *CANx, uint8_t fifoIndex)
{
    if (fifoIndex == CAN_RX_FIFO_0)
    {
        CANx->RF0R = CAN_RF0R_FULL0_Msk; /* W1C — RM0008 §24.4.3 */
    }
    else
    {
        CANx->RF1R = CAN_RF1R_FULL1_Msk; /* W1C — RM0008 §24.4.3 */
    }
}

/**
 * @brief Clear FIFO overrun flag (RC_W1).
 * @param[in] CANx      Pointer to CAN peripheral.
 * @param[in] fifoIndex FIFO index (0 or 1).
 */
static inline void CAN_LL_ClearFifoOverrun(CAN_TypeDef *CANx, uint8_t fifoIndex)
{
    if (fifoIndex == CAN_RX_FIFO_0)
    {
        CANx->RF0R = CAN_RF0R_FOVR0_Msk; /* W1C — RM0008 §24.4.3 */
    }
    else
    {
        CANx->RF1R = CAN_RF1R_FOVR1_Msk; /* W1C — RM0008 §24.4.3 */
    }
}

/*===========================================================================*/
/* IER — Interrupt enable/disable                                             */
/* Source: RM0008 §24.5; ir/can_ir_summary.json registers[5]                 */
/*===========================================================================*/

/** @brief Enable TX mailbox empty interrupt (TMEIE). */
static inline void CAN_LL_EnableTMEIE(CAN_TypeDef *CANx)
{
    CANx->IER |= CAN_IER_TMEIE_Msk; /* RM0008 §24.5 */
}

/** @brief Disable TX mailbox empty interrupt. */
static inline void CAN_LL_DisableTMEIE(CAN_TypeDef *CANx)
{
    CANx->IER &= ~CAN_IER_TMEIE_Msk; /* RM0008 §24.5 */
}

/** @brief Enable FIFO 0 message pending interrupt (FMPIE0). */
static inline void CAN_LL_EnableFMPIE0(CAN_TypeDef *CANx)
{
    CANx->IER |= CAN_IER_FMPIE0_Msk; /* RM0008 §24.5 */
}

/** @brief Disable FIFO 0 message pending interrupt. */
static inline void CAN_LL_DisableFMPIE0(CAN_TypeDef *CANx)
{
    CANx->IER &= ~CAN_IER_FMPIE0_Msk; /* RM0008 §24.5 */
}

/** @brief Enable FIFO 1 message pending interrupt (FMPIE1). */
static inline void CAN_LL_EnableFMPIE1(CAN_TypeDef *CANx)
{
    CANx->IER |= CAN_IER_FMPIE1_Msk; /* RM0008 §24.5 */
}

/** @brief Disable FIFO 1 message pending interrupt. */
static inline void CAN_LL_DisableFMPIE1(CAN_TypeDef *CANx)
{
    CANx->IER &= ~CAN_IER_FMPIE1_Msk; /* RM0008 §24.5 */
}

/** @brief Enable error interrupt (ERRIE — master error interrupt gate). */
static inline void CAN_LL_EnableERRIE(CAN_TypeDef *CANx)
{
    CANx->IER |= CAN_IER_ERRIE_Msk; /* RM0008 §24.5 */
}

/** @brief Disable error interrupt. */
static inline void CAN_LL_DisableERRIE(CAN_TypeDef *CANx)
{
    CANx->IER &= ~CAN_IER_ERRIE_Msk; /* RM0008 §24.5 */
}

/** @brief Enable bus-off interrupt (BOFIE). */
static inline void CAN_LL_EnableBOFIE(CAN_TypeDef *CANx)
{
    CANx->IER |= CAN_IER_BOFIE_Msk; /* RM0008 §24.5 */
}

/** @brief Enable error passive interrupt (EPVIE). */
static inline void CAN_LL_EnableEPVIE(CAN_TypeDef *CANx)
{
    CANx->IER |= CAN_IER_EPVIE_Msk; /* RM0008 §24.5 */
}

/** @brief Enable error warning interrupt (EWGIE). */
static inline void CAN_LL_EnableEWGIE(CAN_TypeDef *CANx)
{
    CANx->IER |= CAN_IER_EWGIE_Msk; /* RM0008 §24.5 */
}

/** @brief Enable last error code interrupt (LECIE). */
static inline void CAN_LL_EnableLECIE(CAN_TypeDef *CANx)
{
    CANx->IER |= CAN_IER_LECIE_Msk; /* RM0008 §24.5 */
}

/** @brief Read current IER value. */
static inline uint32_t CAN_LL_ReadIER(const CAN_TypeDef *CANx)
{
    return CANx->IER; /* RM0008 §24.5 */
}

/** @brief Disable all CAN interrupts. */
static inline void CAN_LL_DisableAllInterrupts(CAN_TypeDef *CANx)
{
    CANx->IER = 0U; /* RM0008 §24.5 */
}

/*===========================================================================*/
/* ESR — Error status                                                         */
/* Source: RM0008 §24.4.6; ir/can_ir_summary.json registers[6]               */
/*===========================================================================*/

/** @brief Read raw ESR value. */
static inline uint32_t CAN_LL_ReadESR(const CAN_TypeDef *CANx)
{
    return CANx->ESR; /* RM0008 §24.4.6 */
}

/** @brief Get last error code (LEC[2:0], 0=no error). */
static inline uint8_t CAN_LL_GetLastErrorCode(const CAN_TypeDef *CANx)
{
    return (uint8_t)((CANx->ESR & CAN_ESR_LEC_Msk) >> CAN_ESR_LEC_Pos); /* RM0008 §24.4.6 */
}

/** @brief Clear last error code by writing 0 to LEC field. */
static inline void CAN_LL_ClearLEC(CAN_TypeDef *CANx)
{
    CANx->ESR &= ~CAN_ESR_LEC_Msk; /* RW field — RM0008 §24.4.6 */
}

/** @brief Get transmit error counter. */
static inline uint8_t CAN_LL_GetTEC(const CAN_TypeDef *CANx)
{
    return (uint8_t)((CANx->ESR & CAN_ESR_TEC_Msk) >> CAN_ESR_TEC_Pos); /* RM0008 §24.4.6 */
}

/** @brief Get receive error counter. */
static inline uint8_t CAN_LL_GetREC(const CAN_TypeDef *CANx)
{
    return (uint8_t)((CANx->ESR & CAN_ESR_REC_Msk) >> CAN_ESR_REC_Pos); /* RM0008 §24.4.6 */
}

/** @brief Check bus-off flag. */
static inline bool CAN_LL_IsBusOff(const CAN_TypeDef *CANx)
{
    return ((CANx->ESR & CAN_ESR_BOFF_Msk) != 0U); /* RM0008 §24.4.6 */
}

/** @brief Check error passive flag. */
static inline bool CAN_LL_IsErrorPassive(const CAN_TypeDef *CANx)
{
    return ((CANx->ESR & CAN_ESR_EPVF_Msk) != 0U); /* RM0008 §24.4.6 */
}

/** @brief Check error warning flag. */
static inline bool CAN_LL_IsErrorWarning(const CAN_TypeDef *CANx)
{
    return ((CANx->ESR & CAN_ESR_EWGF_Msk) != 0U); /* RM0008 §24.4.6 */
}

/*===========================================================================*/
/* TX Mailbox — write message                                                 */
/* Source: RM0008 §24.4.2                                                    */
/*===========================================================================*/

/**
 * @brief Write a CAN message to a TX mailbox and request transmission.
 * @param[in] CANx    Pointer to CAN peripheral.
 * @param[in] mailbox Mailbox index (0-2).
 * @param[in] pMsg    Message to transmit.
 */
static inline void CAN_LL_WriteMailbox(CAN_TypeDef *CANx, uint8_t mailbox,
                                       const Can_MessageType *pMsg)
{
    uint32_t tir = 0U;

    /* Set identifier — RM0008 §24.5 TX mailbox */
    if (pMsg->idType == CAN_ID_EXT)
    {
        tir = ((pMsg->id & 0x1FFFFFFFUL) << CAN_TIR_EXID_Pos) | CAN_TIR_IDE_Msk;
    }
    else
    {
        tir = ((pMsg->id & 0x7FFUL) << CAN_TIR_STID_Pos);
    }

    /* RTR bit */
    if (pMsg->frameType == CAN_FRAME_REMOTE)
    {
        tir |= CAN_TIR_RTR_Msk;
    }

    /* DLC — RM0008 §24.5 */
    CANx->sTxMailBox[mailbox].TDTR = ((uint32_t)pMsg->dlc & CAN_TDTR_DLC_Msk);

    /* Data — RM0008 §24.5 */
    CANx->sTxMailBox[mailbox].TDLR =
        ((uint32_t)pMsg->data[0])        |
        ((uint32_t)pMsg->data[1] << 8U)  |
        ((uint32_t)pMsg->data[2] << 16U) |
        ((uint32_t)pMsg->data[3] << 24U);

    CANx->sTxMailBox[mailbox].TDHR =
        ((uint32_t)pMsg->data[4])        |
        ((uint32_t)pMsg->data[5] << 8U)  |
        ((uint32_t)pMsg->data[6] << 16U) |
        ((uint32_t)pMsg->data[7] << 24U);

    /* Request transmission (TXRQ=1) — must be last write to TIR */
    tir |= CAN_TIR_TXRQ_Msk;
    CANx->sTxMailBox[mailbox].TIR = tir; /* RM0008 §24.4.2 */
}

/*===========================================================================*/
/* RX FIFO — read message                                                    */
/* Source: RM0008 §24.4.3                                                    */
/*===========================================================================*/

/**
 * @brief Read a CAN message from a RX FIFO mailbox.
 *        INV_CAN_007: caller must verify FMP>0 before calling.
 *        Does NOT release the FIFO — caller must call CAN_LL_ReleaseFifo().
 * @param[in]  CANx      Pointer to CAN peripheral.
 * @param[in]  fifoIndex FIFO index (0 or 1).
 * @param[out] pMsg      Output message structure.
 */
static inline void CAN_LL_ReadFifoMailbox(const CAN_TypeDef *CANx, uint8_t fifoIndex,
                                           Can_MessageType *pMsg)
{
    uint32_t rir  = CANx->sFIFOMailBox[fifoIndex].RIR;
    uint32_t rdtr = CANx->sFIFOMailBox[fifoIndex].RDTR;
    uint32_t rdlr = CANx->sFIFOMailBox[fifoIndex].RDLR;
    uint32_t rdhr = CANx->sFIFOMailBox[fifoIndex].RDHR;

    /* ID type and identifier — RM0008 §24.5 */
    if ((rir & CAN_RIR_IDE_Msk) != 0U)
    {
        pMsg->idType = CAN_ID_EXT;
        pMsg->id = (rir >> CAN_RIR_EXID_Pos) & 0x1FFFFFFFUL;
    }
    else
    {
        pMsg->idType = CAN_ID_STD;
        pMsg->id = (rir >> CAN_RIR_STID_Pos) & 0x7FFUL;
    }

    /* Frame type */
    pMsg->frameType = ((rir & CAN_RIR_RTR_Msk) != 0U) ? CAN_FRAME_REMOTE : CAN_FRAME_DATA;

    /* DLC */
    pMsg->dlc = (uint8_t)(rdtr & CAN_RDTR_DLC_Msk);

    /* Filter match index */
    pMsg->filterIndex = (uint8_t)((rdtr & CAN_RDTR_FMI_Msk) >> CAN_RDTR_FMI_Pos);

    /* Timestamp */
    pMsg->timestamp = (uint16_t)((rdtr & CAN_RDTR_TIME_Msk) >> CAN_RDTR_TIME_Pos);

    /* Data bytes — RM0008 §24.5 */
    pMsg->data[0] = (uint8_t)(rdlr & 0xFFU);
    pMsg->data[1] = (uint8_t)((rdlr >> 8U) & 0xFFU);
    pMsg->data[2] = (uint8_t)((rdlr >> 16U) & 0xFFU);
    pMsg->data[3] = (uint8_t)((rdlr >> 24U) & 0xFFU);
    pMsg->data[4] = (uint8_t)(rdhr & 0xFFU);
    pMsg->data[5] = (uint8_t)((rdhr >> 8U) & 0xFFU);
    pMsg->data[6] = (uint8_t)((rdhr >> 16U) & 0xFFU);
    pMsg->data[7] = (uint8_t)((rdhr >> 24U) & 0xFFU);
}

/*===========================================================================*/
/* Filter — configuration                                                     */
/* Source: RM0008 §24.5; ir/can_ir_summary.json registers[8-12]              */
/* Guard: INV_CAN_003 — filter registers writable only when FINIT=1          */
/*===========================================================================*/

/**
 * @brief Enter filter initialization mode (set FINIT).
 *        SEQ_CAN_ENTER_FILTER_INIT — RM0008 §24.5
 * @param[in] CANx  Pointer to CAN peripheral.
 */
static inline void CAN_LL_EnterFilterInit(CAN_TypeDef *CANx)
{
    CANx->FMR |= CAN_FMR_FINIT_Msk; /* IR: atomic_sequences[2] — RM0008 §24.5 */
}

/**
 * @brief Exit filter initialization mode (clear FINIT).
 *        SEQ_CAN_EXIT_FILTER_INIT — RM0008 §24.5
 * @param[in] CANx  Pointer to CAN peripheral.
 */
static inline void CAN_LL_ExitFilterInit(CAN_TypeDef *CANx)
{
    CANx->FMR &= ~CAN_FMR_FINIT_Msk; /* IR: atomic_sequences[3] — RM0008 §24.5 */
}

/**
 * @brief Configure a single filter bank.
 *        Guard: INV_CAN_003 — must be in filter init mode (FINIT=1).
 * @param[in] CANx    Pointer to CAN peripheral.
 * @param[in] pFilter Filter configuration.
 */
static inline void CAN_LL_ConfigureFilter(CAN_TypeDef *CANx,
                                           const Can_FilterConfigType *pFilter)
{
    uint32_t filterBit = (1UL << (uint32_t)pFilter->filterNumber);

    /* Guard: INV_CAN_003 — FINIT must be 1 — RM0008 §24.5 */

    /* Deactivate filter before modification */
    CANx->FA1R &= ~filterBit; /* RM0008 §24.5 */

    /* Filter mode: 0=mask, 1=list — RM0008 §24.5 */
    if (pFilter->mode == CAN_FILTER_LIST)
    {
        CANx->FM1R |= filterBit;
    }
    else
    {
        CANx->FM1R &= ~filterBit;
    }

    /* Filter scale: 0=16-bit, 1=32-bit — RM0008 §24.5 */
    if (pFilter->scale == CAN_FILTER_SCALE_32BIT)
    {
        CANx->FS1R |= filterBit;
    }
    else
    {
        CANx->FS1R &= ~filterBit;
    }

    /* FIFO assignment: 0=FIFO0, 1=FIFO1 — RM0008 §24.5 */
    if (pFilter->fifoAssign == CAN_FILTER_FIFO1)
    {
        CANx->FFA1R |= filterBit;
    }
    else
    {
        CANx->FFA1R &= ~filterBit;
    }

    /* Filter register values — RM0008 §24.5 */
    CANx->sFilterBank[pFilter->filterNumber].FR1 =
        ((pFilter->idHigh & 0xFFFFU) << 16U) | (pFilter->idLow & 0xFFFFU);
    CANx->sFilterBank[pFilter->filterNumber].FR2 =
        ((pFilter->maskIdHigh & 0xFFFFU) << 16U) | (pFilter->maskIdLow & 0xFFFFU);

    /* Activate filter if requested */
    if (pFilter->enable)
    {
        CANx->FA1R |= filterBit; /* RM0008 §24.5 */
    }
}

#endif /* CAN_LL_H */
