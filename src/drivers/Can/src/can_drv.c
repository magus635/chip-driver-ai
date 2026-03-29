/**
 * @file can_drv.c
 * @brief High-level bxCAN driver logic (V2.3 HARDWARE EXHAUSTION).
 * @note Features: 
 *       1. Baudrate flexibility via ConfigType
 *       2. Triple TX mailbox multiplexing (parallel transmission)
 *       3. Error State Recording (LEC, TEC, REC)
 *       4. Finite timeouts on all mode transitions.
 */

#include "can_api.h"
#include "can_ll.h"
#include "rcc_api.h"
#include "port_api.h"

/* Internal counters hiding globals */
static uint32_t g_can_tx_msg_count = 0;
static uint32_t g_can_rx_msg_count = 0;
static uint8_t  g_can_last_lec = 0;

Can_ReturnType Can_Init(const Can_ConfigType *ConfigPtr) {
    if (ConfigPtr == (void*)0) return CAN_NOT_OK;
    
    CAN_TypeDef *can = CAN1;
    uint32_t timeout = 50000U;

    /* 启动时钟与初始化引脚 (MCAL Cross-Module) */
    /* Rcc_Api_EnableCan1(); */
    /* Port_Api_InitCanPins(); */

    CAN_LL_EnterInitMode(can);
    while ((!CAN_LL_IsInitModeEntered(can)) && (--timeout > 0U)) {}
    if (timeout == 0U) return CAN_TIMEOUT;

    /* 榨干点：完全灵活的波特率配置与工作模式 */
    CAN_LL_SetBaudrate(can, ConfigPtr);

    /* 退出并应用配置 (NART, ABOM etc.) */
    CAN_LL_ExitInitMode(can, ConfigPtr);
    timeout = 50000U;
    while ((!CAN_LL_IsInitModeExited(can)) && (--timeout > 0U)) {}
    if (timeout == 0U) return CAN_TIMEOUT;

    /* 初始化过滤器 (默认全部通过，上层可通过 Filter API 调整) */
    CAN_LL_InitFilter(can);
    
    return CAN_OK;
}

Can_ReturnType Can_Write(const Can_PduType *PduInfo) {
    CAN_TypeDef *can = CAN1;
    int8_t mailbox_idx;

    if (PduInfo == (void*)0) return CAN_NOT_OK;

    /* 榨干点：自动利用硬件 3 个 TX 邮箱，多报文并发缓冲 */
    mailbox_idx = CAN_LL_GetFreeTxMailbox(can);
    if (mailbox_idx < 0) {
        return CAN_BUSY; /* 说明 3 个邮箱全满了，正在发送中 */
    }

    CAN_LL_WriteTxMailbox(can, (uint8_t)mailbox_idx, PduInfo);
    
    g_can_tx_msg_count++;
    return CAN_OK;
}

Can_ReturnType Can_Receive(Can_PduType *PduInfo) {
    CAN_TypeDef *can = CAN1;

    if (PduInfo == (void*)0) return CAN_NOT_OK;

    if (CAN_LL_IsRxFifo0Empty(can)) {
        return CAN_BUSY;
    }

    CAN_LL_ReadRxFifo0(can, PduInfo);
    CAN_LL_ReleaseRxFifo0(can);
    
    g_can_rx_msg_count++;

    /* 同步最新错误状态 */
    uint32_t esr = CAN_LL_GetESR(can);
    g_can_last_lec = (uint8_t)((esr & CAN_ESR_LEC_Msk) >> 4);
    
    return CAN_OK;
}

void Can_GetControllerErrorState(uint8_t *Txc, uint8_t *Rxc, uint8_t *Lec) {
    CAN_TypeDef *can = CAN1;
    uint32_t esr = CAN_LL_GetESR(can);
    
    if (Txc != (void*)0) *Txc = (uint8_t)((esr & CAN_ESR_TEC_Msk) >> 16);
    if (Rxc != (void*)0) *Rxc = (uint8_t)((esr & CAN_ESR_REC_Msk) >> 24);
    if (Lec != (void*)0) *Lec = (uint8_t)((esr & CAN_ESR_LEC_Msk) >> 4);
}
