/**
 * @file    can_api.h
 * @brief   STM32F103C8T6 CAN 公共接口层 (API)
 *
 * @note    对上层唯一暴露的稳定接口。
 *          禁止 include can_ll.h 或 can_reg.h（规则 R8-7）。
 *          只 include can_types.h 和 can_cfg.h。
 *
 * Source:  RM0008 Rev 14, Chapter 24 - bxCAN
 *          ir/can_ir_summary.json v2.0
 */

#ifndef CAN_API_H
#define CAN_API_H

#include "can_types.h"
#include "can_cfg.h"

/*===========================================================================*/
/* Initialisation / De-initialisation                                         */
/*===========================================================================*/

/**
 * @brief  Initialise CAN1 according to the provided configuration.
 *
 *         Full init sequence (ir/can_ir_summary.json configuration_strategies[0]):
 *           1. Enable GPIO and AFIO clocks, configure pin remap.
 *           2. Enable CAN1 clock (RCC_APB1ENR.CAN1EN = 1).
 *           3. Request initialization mode (MCR.INRQ = 1).
 *           4. Wait for INAK = 1 (INV_CAN_001).
 *           5. Configure MCR operational bits.
 *           6. Configure BTR with baud rate and mode.
 *           7. Exit initialization mode (MCR.INRQ = 0).
 *           8. Wait for INAK = 0 (INV_CAN_002).
 *           9. Configure default accept-all filter.
 *          10. Enable interrupts if CAN_CFG_IRQ_ENABLE.
 *
 * @param[in]  pConfig  Pointer to configuration structure. Must not be NULL.
 * @return     CAN_OK on success, CAN_ERR_PARAM if pConfig is NULL or invalid,
 *             CAN_ERR_TIMEOUT if mode transition timed out.
 */
Can_ReturnType Can_Init(const Can_ConfigType *pConfig);

/**
 * @brief  De-initialise CAN1 and release the peripheral clock.
 *
 * @return     CAN_OK on success, CAN_ERR_NOT_INIT if not previously initialised.
 */
Can_ReturnType Can_DeInit(void);

/*===========================================================================*/
/* Filter configuration                                                       */
/*===========================================================================*/

/**
 * @brief  Configure a CAN filter bank.
 *         Enters filter init mode, configures the bank, then exits.
 *         Guard: INV_CAN_003 — filter registers writable only in FINIT mode.
 *         INV_CAN_004 — at least one filter must be active for reception.
 *
 * @param[in]  pFilter  Filter configuration. Must not be NULL.
 *                      filterNumber must be 0..13.
 * @return     CAN_OK or CAN_ERR_PARAM.
 */
Can_ReturnType Can_ConfigFilter(const Can_FilterConfigType *pFilter);

/*===========================================================================*/
/* Transmission                                                               */
/*===========================================================================*/

/**
 * @brief  Add a message to the next available TX mailbox.
 *         INV_CAN_006: checks TME before writing.
 *
 * @param[in]  pMsg  Message to transmit. Must not be NULL. DLC must be 0..8.
 * @return     TX result with status and mailbox index used.
 */
Can_TxResultType Can_Transmit(const Can_MessageType *pMsg);

/**
 * @brief  Abort a pending transmission in a specific mailbox.
 *
 * @param[in]  mailbox  Mailbox index (0-2).
 * @return     CAN_OK or CAN_ERR_PARAM.
 */
Can_ReturnType Can_AbortTransmit(uint8_t mailbox);

/**
 * @brief  Check if a TX mailbox transmission is complete.
 *
 * @param[in]  mailbox  Mailbox index (0-2).
 * @return     true if RQCP is set (transmission complete or aborted).
 */
bool Can_IsTxComplete(uint8_t mailbox);

/*===========================================================================*/
/* Reception                                                                  */
/*===========================================================================*/

/**
 * @brief  Receive a message from the specified RX FIFO (polling).
 *         INV_CAN_007: checks FMP>0 before reading.
 *         Reads the message and releases the FIFO output mailbox.
 *
 * @param[in]  fifoIndex  FIFO index (0 or 1).
 * @param[out] pMsg       Output message buffer. Must not be NULL.
 * @return     CAN_OK on success, CAN_ERR_NO_MESSAGE if FIFO empty,
 *             CAN_ERR_PARAM if invalid.
 */
Can_ReturnType Can_Receive(uint8_t fifoIndex, Can_MessageType *pMsg);

/**
 * @brief  Get number of pending messages in a RX FIFO.
 *
 * @param[in]  fifoIndex  FIFO index (0 or 1).
 * @return     Number of pending messages (0-3).
 */
uint8_t Can_GetRxPending(uint8_t fifoIndex);

/*===========================================================================*/
/* Error and status                                                           */
/*===========================================================================*/

/**
 * @brief  Get the current error status snapshot.
 *
 * @param[out] pStatus  Output status structure. Must not be NULL.
 * @return     CAN_OK or CAN_ERR_PARAM.
 */
Can_ReturnType Can_GetErrorStatus(Can_ErrorStatusType *pStatus);

/**
 * @brief  Get the current driver state.
 * @return Current Can_State_e value.
 */
Can_State_e Can_GetState(void);

/*===========================================================================*/
/* Sleep / Wakeup                                                             */
/*===========================================================================*/

/**
 * @brief  Request CAN sleep mode.
 * @return CAN_OK on success, CAN_ERR_TIMEOUT if SLAK not acknowledged.
 */
Can_ReturnType Can_Sleep(void);

/**
 * @brief  Request wakeup from sleep mode.
 * @return CAN_OK on success, CAN_ERR_TIMEOUT if wakeup not acknowledged.
 */
Can_ReturnType Can_Wakeup(void);

/*===========================================================================*/
/* Callback registration                                                      */
/*===========================================================================*/

/**
 * @brief  Register a TX completion callback (called from ISR context).
 * @param[in]  callback  Function pointer (may be NULL to deregister).
 * @return     CAN_OK.
 */
Can_ReturnType Can_RegisterTxCallback(Can_TxCallback_t callback);

/**
 * @brief  Register a RX message received callback (called from ISR context).
 * @param[in]  callback  Function pointer (may be NULL to deregister).
 * @return     CAN_OK.
 */
Can_ReturnType Can_RegisterRxCallback(Can_RxCallback_t callback);

/**
 * @brief  Register an error/status change callback (called from ISR context).
 * @param[in]  callback  Function pointer (may be NULL to deregister).
 * @return     CAN_OK.
 */
Can_ReturnType Can_RegisterErrorCallback(Can_ErrorCallback_t callback);

/*===========================================================================*/
/* ISR entry points (called from can_isr.c vector handlers)                  */
/*===========================================================================*/

/**
 * @brief  CAN TX interrupt handler.
 *         Handles RQCP0/1/2 flags, invokes TX callback.
 *         Source: ir/can_ir_summary.json interrupts[0] CAN1_TX
 */
void Can_TxIRQHandler(void);

/**
 * @brief  CAN RX FIFO 0 interrupt handler.
 *         Handles FMP0, FULL0, FOVR0 flags, invokes RX callback.
 *         Source: ir/can_ir_summary.json interrupts[1] CAN1_RX0
 */
void Can_Rx0IRQHandler(void);

/**
 * @brief  CAN RX FIFO 1 interrupt handler.
 *         Handles FMP1, FULL1, FOVR1 flags, invokes RX callback.
 *         Source: ir/can_ir_summary.json interrupts[2] CAN1_RX1
 */
void Can_Rx1IRQHandler(void);

/**
 * @brief  CAN Status Change / Error interrupt handler.
 *         Handles ERRI, EWGF, EPVF, BOFF, LEC, WKUI, SLAKI flags.
 *         Source: ir/can_ir_summary.json interrupts[3] CAN1_SCE
 */
void Can_SceIRQHandler(void);

#endif /* CAN_API_H */
