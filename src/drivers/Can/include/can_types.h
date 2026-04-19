/**
 * @file    can_types.h
 * @brief   STM32F103C8T6 CAN 类型定义层
 *
 * @note    集中定义跨层共用的数据结构、枚举、错误码。
 *          禁止包含任何函数声明/实现或寄存器引用。
 *          只允许 include C 标准库头文件。
 *
 * Source:  RM0008 Rev 14, Chapter 24 - bxCAN
 *          ir/can_ir_summary.json v2.0
 */

#ifndef CAN_TYPES_H
#define CAN_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/*===========================================================================*/
/* Return / error code enumeration                                            */
/* Source: ir/can_ir_summary.json errors[]                                   */
/*===========================================================================*/

typedef enum
{
    CAN_OK              = 0,  /* Operation successful                        */
    CAN_ERR_BUSY        = 1,  /* Peripheral or transfer in progress          */
    CAN_ERR_PARAM       = 2,  /* Invalid parameter                           */
    CAN_ERR_TIMEOUT     = 3,  /* Wait loop exceeded timeout limit            */
    CAN_ERR_NO_MAILBOX  = 4,  /* No empty TX mailbox available               */
    CAN_ERR_NO_MESSAGE  = 5,  /* No message pending in RX FIFO               */
    CAN_ERR_NOT_INIT    = 6,  /* Module not initialised                      */
    CAN_ERR_STUFF       = 7,  /* LEC=1: Stuff error — RM0008 §24.4.6        */
    CAN_ERR_FORM        = 8,  /* LEC=2: Form error                          */
    CAN_ERR_ACK         = 9,  /* LEC=3: Acknowledgment error                */
    CAN_ERR_BIT_RECESS  = 10, /* LEC=4: Bit recessive error                 */
    CAN_ERR_BIT_DOMIN   = 11, /* LEC=5: Bit dominant error                  */
    CAN_ERR_CRC         = 12, /* LEC=6: CRC error                           */
    CAN_ERR_BUS_OFF     = 13, /* ESR.BOFF=1, TEC>255                        */
    CAN_ERR_PASSIVE     = 14, /* ESR.EPVF=1, TEC or REC>127                 */
    CAN_ERR_WARNING     = 15  /* ESR.EWGF=1, TEC or REC>96                  */
} Can_ReturnType;

/*===========================================================================*/
/* Operation mode                                                             */
/* Source: ir/can_ir_summary.json operation_modes[],                          */
/*         configuration_strategies[] LOOPBACK_DEBUG, SILENT_MODE             */
/*===========================================================================*/

typedef enum
{
    CAN_MODE_NORMAL     = 0U,  /* Normal mode: transmit and receive           */
    CAN_MODE_LOOPBACK   = 1U,  /* Loopback mode: self-test without bus        */
    CAN_MODE_SILENT     = 2U,  /* Silent mode: listen-only, no TX             */
    CAN_MODE_LOOPBACK_SILENT = 3U  /* Combined loopback + silent              */
} Can_OperationMode_e;

/*===========================================================================*/
/* TX FIFO priority                                                           */
/* Source: ir/can_ir_summary.json registers[0] MCR.TXFP                      */
/*===========================================================================*/

typedef enum
{
    CAN_TXPRIO_IDENTIFIER = 0U,  /* Priority by message identifier            */
    CAN_TXPRIO_FIFO       = 1U   /* Priority by request order (FIFO)          */
} Can_TxPriority_e;

/*===========================================================================*/
/* GPIO pin remap                                                             */
/* Source: ir/can_ir_summary.json configuration_strategies[] PIN_REMAP        */
/*===========================================================================*/

typedef enum
{
    CAN_REMAP_NONE    = 0U,  /* Default pins: PA11(RX)/PA12(TX)              */
    CAN_REMAP_PARTIAL = 2U,  /* Partial remap: PB8(RX)/PB9(TX)              */
    CAN_REMAP_FULL    = 3U   /* Full remap:    PD0(RX)/PD1(TX)              */
} Can_PinRemap_e;

/*===========================================================================*/
/* Filter mode                                                                */
/* Source: ir/can_ir_summary.json registers[] FM1R                           */
/*===========================================================================*/

typedef enum
{
    CAN_FILTER_MASK = 0U,  /* Mask mode: ID & mask comparison               */
    CAN_FILTER_LIST = 1U   /* List mode: exact ID match                     */
} Can_FilterMode_e;

/*===========================================================================*/
/* Filter scale                                                               */
/* Source: ir/can_ir_summary.json registers[] FS1R                           */
/*===========================================================================*/

typedef enum
{
    CAN_FILTER_SCALE_16BIT = 0U,  /* Dual 16-bit filter                     */
    CAN_FILTER_SCALE_32BIT = 1U   /* Single 32-bit filter                   */
} Can_FilterScale_e;

/*===========================================================================*/
/* Filter FIFO assignment                                                     */
/* Source: ir/can_ir_summary.json registers[] FFA1R                          */
/*===========================================================================*/

typedef enum
{
    CAN_FILTER_FIFO0 = 0U,  /* Assign filter to FIFO 0                      */
    CAN_FILTER_FIFO1 = 1U   /* Assign filter to FIFO 1                      */
} Can_FilterFifo_e;

/*===========================================================================*/
/* ID type                                                                    */
/* Source: ir/can_ir_summary.json functional_model.key_features[]            */
/*===========================================================================*/

typedef enum
{
    CAN_ID_STD = 0U,  /* Standard 11-bit identifier                         */
    CAN_ID_EXT = 1U   /* Extended 29-bit identifier                         */
} Can_IdType_e;

/*===========================================================================*/
/* Frame type                                                                 */
/*===========================================================================*/

typedef enum
{
    CAN_FRAME_DATA   = 0U,  /* Data frame                                   */
    CAN_FRAME_REMOTE = 1U   /* Remote transmission request frame            */
} Can_FrameType_e;

/*===========================================================================*/
/* Runtime module state                                                       */
/*===========================================================================*/

typedef enum
{
    CAN_STATE_UNINIT    = 0U,  /* Module not initialised                     */
    CAN_STATE_READY     = 1U,  /* Initialised, normal operation              */
    CAN_STATE_BUSY_TX   = 2U,  /* Transmit in progress                      */
    CAN_STATE_SLEEP     = 3U,  /* Sleep mode                                 */
    CAN_STATE_ERROR     = 4U   /* Error state, needs recovery                */
} Can_State_e;

/*===========================================================================*/
/* TX mailbox index                                                           */
/*===========================================================================*/

#define CAN_TX_MAILBOX_0        ((uint8_t)0U)
#define CAN_TX_MAILBOX_1        ((uint8_t)1U)
#define CAN_TX_MAILBOX_2        ((uint8_t)2U)
#define CAN_TX_MAILBOX_COUNT    ((uint8_t)3U)
#define CAN_TX_MAILBOX_NONE     ((uint8_t)0xFFU)

/*===========================================================================*/
/* RX FIFO index                                                              */
/*===========================================================================*/

#define CAN_RX_FIFO_0           ((uint8_t)0U)
#define CAN_RX_FIFO_1           ((uint8_t)1U)
#define CAN_RX_FIFO_COUNT       ((uint8_t)2U)

/*===========================================================================*/
/* Filter bank count (STM32F103 CAN1-only = 14)                              */
/* Source: ir/can_ir_summary.json generation_metadata.notes[]                */
/*===========================================================================*/

#define CAN_FILTER_BANK_COUNT   ((uint8_t)14U)

/*===========================================================================*/
/* Bit timing configuration                                                   */
/* Source: ir/can_ir_summary.json timing_constraints,                         */
/*         registers[7] BTR bitfields                                        */
/*===========================================================================*/

typedef struct
{
    uint16_t prescaler;   /* Baud rate prescaler (1-1024, actual = BRP+1)     */
    uint8_t  timeSeg1;    /* Time segment 1 (1-16 Tq, actual = TS1+1)        */
    uint8_t  timeSeg2;    /* Time segment 2 (1-8 Tq, actual = TS2+1)         */
    uint8_t  syncJumpW;   /* Resynchronization jump width (1-4 Tq, SJW+1)    */
} Can_BitTimingType;

/*===========================================================================*/
/* Filter configuration                                                       */
/*===========================================================================*/

typedef struct
{
    uint8_t           filterNumber;   /* Filter bank index (0-13)             */
    Can_FilterMode_e  mode;           /* Mask or list mode                    */
    Can_FilterScale_e scale;          /* 16-bit or 32-bit scale               */
    Can_FilterFifo_e  fifoAssign;     /* Assign to FIFO 0 or 1               */
    bool              enable;         /* Activate this filter bank            */
    uint32_t          idHigh;         /* Filter ID or ID1 (high 16 bits)      */
    uint32_t          idLow;          /* Filter mask or ID2 (low 16 bits)     */
    uint32_t          maskIdHigh;     /* Mask high or ID3                     */
    uint32_t          maskIdLow;      /* Mask low or ID4                      */
} Can_FilterConfigType;

/*===========================================================================*/
/* Main configuration structure (passed to Can_Init)                         */
/* Source: ir/can_ir_summary.json configuration_strategies[] INIT_BASIC      */
/*===========================================================================*/

typedef struct
{
    Can_OperationMode_e  mode;          /* Normal / loopback / silent          */
    Can_BitTimingType    bitTiming;     /* Bit timing parameters               */
    Can_TxPriority_e     txPriority;    /* TX mailbox priority scheme           */
    bool                 autoRetransmit;/* true = auto-retransmit, false = NART */
    bool                 autoBusOff;    /* true = auto bus-off recovery (ABOM)  */
    bool                 autoWakeup;    /* true = auto wakeup on bus activity   */
    bool                 rxFifoLocked;  /* true = locked FIFO mode (RFLM)      */
    bool                 timeTrigger;   /* true = time triggered comm (TTCM)    */
    Can_PinRemap_e       pinRemap;      /* GPIO pin remapping selection         */
} Can_ConfigType;

/*===========================================================================*/
/* CAN message (TX and RX)                                                    */
/*===========================================================================*/

typedef struct
{
    uint32_t        id;          /* Message identifier (11-bit or 29-bit)     */
    Can_IdType_e    idType;      /* Standard or extended ID                   */
    Can_FrameType_e frameType;   /* Data or remote frame                     */
    uint8_t         dlc;         /* Data length code (0-8)                    */
    uint8_t         data[8];     /* Data payload                              */
    uint16_t        timestamp;   /* Hardware timestamp (valid if TTCM=1)      */
    uint8_t         filterIndex; /* Filter match index (RX only)              */
} Can_MessageType;

/*===========================================================================*/
/* TX result (returned after adding message to mailbox)                      */
/*===========================================================================*/

typedef struct
{
    Can_ReturnType  status;       /* Operation result                         */
    uint8_t         mailbox;      /* Mailbox used (0-2, or NONE if failed)    */
} Can_TxResultType;

/*===========================================================================*/
/* Error status snapshot (returned by Can_GetErrorStatus)                    */
/* Source: ir/can_ir_summary.json registers[6] ESR                          */
/*===========================================================================*/

typedef struct
{
    Can_State_e  state;         /* Current driver state                       */
    bool         errorWarning;  /* ESR.EWGF — TEC or REC > 96                */
    bool         errorPassive;  /* ESR.EPVF — TEC or REC > 127               */
    bool         busOff;        /* ESR.BOFF — TEC > 255                      */
    uint8_t      lastErrorCode; /* ESR.LEC[2:0] — 0=none,1-6=error types     */
    uint8_t      txErrorCount;  /* ESR.TEC[7:0]                              */
    uint8_t      rxErrorCount;  /* ESR.REC[7:0]                              */
} Can_ErrorStatusType;

/*===========================================================================*/
/* Callback types                                                             */
/*===========================================================================*/

/** @brief TX completion callback. */
typedef void (*Can_TxCallback_t)(uint8_t mailbox, Can_ReturnType result);

/** @brief RX message received callback. */
typedef void (*Can_RxCallback_t)(uint8_t fifoIndex, const Can_MessageType *pMsg);

/** @brief Error/status change callback. */
typedef void (*Can_ErrorCallback_t)(Can_ReturnType error, const Can_ErrorStatusType *pStatus);

#endif /* CAN_TYPES_H */
