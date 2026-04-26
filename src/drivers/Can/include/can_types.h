/**
 * @file    can_types.h
 * @brief   STM32F103C8T6 CAN 类型定义 (V4.0)
 */

#ifndef CAN_TYPES_H
#define CAN_TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    CAN_OK              = 0,
    CAN_ERR_BUSY        = 1,
    CAN_ERR_PARAM       = 2,
    CAN_ERR_TIMEOUT     = 3,
    CAN_ERR_NOT_INIT    = 4,
    CAN_ERR_EMPTY       = 5
} Can_ReturnType;

typedef enum
{
    CAN_STATE_UNINIT     = 0U,
    CAN_STATE_READY      = 1U,
    CAN_STATE_BUSY       = 2U,
    CAN_STATE_ERROR      = 3U
} Can_State_e;

typedef enum
{
    CAN_MODE_NORMAL          = 0U,
    CAN_MODE_INITIALIZATION  = 1U,
    CAN_MODE_SLEEP           = 2U,
    CAN_MODE_LOOPBACK        = 3U,
    CAN_MODE_SILENT          = 4U,
    CAN_MODE_LOOPBACK_SILENT = 5U
} Can_OperatingMode_e;

typedef enum { CAN_ID_STD = 0U, CAN_ID_EXT = 1U } Can_IdType_e;

typedef struct
{
    uint32_t      id;
    Can_IdType_e  idType;
    uint8_t       length;
    uint8_t       data[8];
} Can_PduType;

typedef struct
{
    Can_OperatingMode_e  mode;
    uint16_t             brp;    /* Prescaler: 1-1024 */
    uint8_t              ts1;    /* Time Segment 1: 1-16 */
    uint8_t              ts2;    /* Time Segment 2: 1-8 */
    uint8_t              sjw;    /* Resynchronization Jump Width: 1-4 */
    bool                 autoBusOff;
    bool                 autoWakeup;
    bool                 txPriority;
} Can_ConfigType;

#endif /* CAN_TYPES_H */
