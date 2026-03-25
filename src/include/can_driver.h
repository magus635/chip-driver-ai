/**
 * src/include/can_driver.h — CAN 驱动公共接口
 */

#ifndef CAN_DRIVER_H
#define CAN_DRIVER_H

#include <stdint.h>

/* ── 驱动状态 ────────────────────────────────────────────── */
typedef enum {
    CAN_STATE_RESET  = 0,
    CAN_STATE_READY  = 1,
    CAN_STATE_BUSOFF = 2,
    CAN_STATE_ERROR  = 3,
} can_state_t;

/* ── CAN 报文结构 ─────────────────────────────────────────── */
typedef struct {
    uint32_t id;          /**< 帧 ID（11位标准帧或29位扩展帧） */
    uint8_t  ide;         /**< 0=标准帧, 1=扩展帧 */
    uint8_t  rtr;         /**< 0=数据帧, 1=远程帧 */
    uint8_t  dlc;         /**< 数据长度（0-8） */
    uint8_t  data[8];     /**< 数据域 */
} can_msg_t;

/* ── 公共 API ─────────────────────────────────────────────── */
int32_t can_init(uint32_t baudrate_kbps);
int32_t can_transmit(const can_msg_t *msg);
int32_t can_receive(uint8_t fifo_num, can_msg_t *msg);

/* ── 供调试监控的外部变量声明 ────────────────────────────── */
extern volatile uint32_t  g_can_init_done;
extern volatile uint32_t  g_can_tx_count;
extern volatile uint32_t  g_can_rx_count;
extern volatile uint32_t  g_can_error_count;
extern volatile can_state_t g_can_state;

#endif /* CAN_DRIVER_H */
