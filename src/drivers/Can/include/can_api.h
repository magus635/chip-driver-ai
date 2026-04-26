/**
 * @file    can_api.h
 * @brief   STM32F103C8T6 CAN 公共接口定义 (V4.0)
 */

#ifndef CAN_API_H
#define CAN_API_H

#include "can_types.h"

typedef struct
{
    uint8_t   bank;
    uint32_t  id;
    uint32_t  mask;
    uint8_t   fifo;
    bool      isExtended;
} Can_FilterConfigType;

Can_ReturnType Can_Init(const Can_ConfigType *pConfig);

/**
 * @brief  Configure a filter bank.
 */
Can_ReturnType Can_SetFilter(const Can_FilterConfigType *pFilter);

/**
 * @brief  De-initialize CAN peripheral.
 */
void Can_DeInit(void);

/**
 * @brief  Write a message to an empty transmit mailbox.
 */
Can_ReturnType Can_Write(const Can_PduType *pPdu);

/**
 * @brief  Read a message from specified FIFO (0 or 1).
 */
Can_ReturnType Can_Read(uint8_t fifo, Can_PduType *pPdu);

#endif /* CAN_API_H */
