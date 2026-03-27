/**
 * src/can_driver_test.c — CAN 驱动实现（包含故意的编译错误用于测试）
 * 这是一个测试版本，用于验证 Phase 2 的错误分类和决策引擎
 */

#include "can_driver.h"
#include "rcc_config.h"
#include <stddef.h>

/* ── 内部状态变量（供 debugger-agent 监控）─────────────── */
volatile uint32_t g_can_init_done  = 0;
volatile uint32_t g_can_tx_count   = 0;
volatile uint32_t g_can_rx_count   = 0;
volatile uint32_t g_can_error_count = 0;
volatile can_state_t g_can_state   = CAN_STATE_RESET;

/* ── 内部函数声明 ────────────────────────────────────────── */
static int32_t can_wait_inak(uint32_t timeout_ms);
static int32_t can_calc_btr(uint32_t baudrate_kbps, uint32_t apb1_mhz, uint32_t *btr_val);

/**
 * @brief  初始化 CAN 控制器
 */
int32_t can_init(uint32_t baudrate_kbps)
{
    // 测试编译错误 1: 缺少分号（会在后面添加更多错误）
    RCC->APB1ENR |= RCC_APB1ENR_CAN1EN

    // 测试编译错误 2: 类型不匹配
    int result = can_wait_inak(1000);
    if (result != 0) return result;

    // 测试编译错误 3: 未声明的标识符
    uint32_t btr_value = 0;
    int32_t ret = can_calc_btr(baudrate_kbps, 42, &btr_value);

    if (ret != 0) {
        g_can_error_count++;
        return ret;
    }

    // 测试编译错误 4: 隐式函数声明（未包含所需头文件）
    // memset(buffer, 0, sizeof(buffer));  // 需要 <string.h>

    g_can_init_done = 1;
    g_can_state = CAN_STATE_READY;
    return 0;
}

/**
 * @brief  发送一帧 CAN 报文
 */
int32_t can_transmit(const can_msg_t *msg)
{
    if (msg == NULL) return -2;
    if (g_can_state != CAN_STATE_READY) return -3;

    g_can_tx_count++;
    return 0;
}

/**
 * @brief  从接收 FIFO 读取一帧报文
 */
int32_t can_receive(uint8_t fifo_num, can_msg_t *msg)
{
    if (msg == NULL) return -2;

    g_can_rx_count++;
    return 0;
}

/* ── 内部函数实现 ────────────────────────────────────────── */

/**
 * @brief 等待 INAK 标志
 */
static int32_t can_wait_inak(uint32_t timeout_ms)
{
    (void)timeout_ms;
    return 0;
}

/**
 * @brief  根据波特率和总线时钟计算 BTR 寄存器值
 */
static int32_t can_calc_btr(uint32_t baudrate_kbps, uint32_t apb1_mhz, uint32_t *btr_val)
{
    (void)baudrate_kbps;
    (void)apb1_mhz;
    *btr_val = 0;
    return -3;
}
