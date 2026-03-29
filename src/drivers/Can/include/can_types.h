#ifndef CAN_TYPES_H
#define CAN_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/* Layer 0a: CAN Base Types (AUTOSAR-ready V2.3) */

typedef enum {
    CAN_OK          = 0,
    CAN_NOT_OK      = 1,
    CAN_BUSY        = 2,
    CAN_TIMEOUT     = 3,
    CAN_ERR_BUSOFF  = 4,
    CAN_ERR_PASSIVE = 5,
} Can_ReturnType;

typedef enum {
    CAN_MODE_NORMAL   = 0,
    CAN_MODE_LOOPBACK = 1,
    CAN_MODE_SILENT   = 2,
    CAN_MODE_SILENT_LOOPBACK = 3
} Can_ControllerModeType;

/* 榨干硬件特性：完整波特率与过滤器配置包 */
typedef struct {
    uint16_t prescaler;      /* BRP 1..1024 */
    uint8_t  sjw;            /* SJW 1..4 */
    uint8_t  prop_seg;       /* BS1 1..16 */
    uint8_t  phase_seg2;     /* BS2 1..8 */
    Can_ControllerModeType mode;
    bool auto_retransmit;    /* NART bit */
    bool bus_off_recovery;   /* ABOM bit */
} Can_ConfigType;

typedef struct {
    uint32_t id;             /* 11-bit Standard or 29-bit Extended */
    uint8_t  ide;            /* 0: Standard, 1: Extended */
    uint8_t  rtr;            /* 0: Data, 1: Remote */
    uint8_t  dlc;            /* 0..8 */
    uint8_t  data[8];        
} Can_PduType;

#endif 
