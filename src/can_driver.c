/**
 * src/can_driver.c — CAN 驱动框架示例
 *
 * 请根据芯片手册（docs/ 目录下）和 TODO 提示实现此文件中的功能。
 */

#include "can_driver.h"

/* 建议的内部状态变量 */
volatile uint32_t g_can_init_done  = 0;
volatile uint32_t g_can_tx_count   = 0;
volatile uint32_t g_can_rx_count   = 0;
volatile uint32_t g_can_error_count = 0;

/**
 * @brief  初始化 CAN 控制器
 */
int32_t can_init(uint32_t baudrate_kbps)
{
    // TODO: 1. 使能外设时钟
    
    // TODO: 2. 进入初始化模式
    
    // TODO: 3. 配置波特率 (btr)
    
    // TODO: 4. 退出初始化并等待硬件就绪
    
    return -1; // 修改为 0 表示成功
}

/**
 * @brief  发送一帧 CAN 报文
 */
int32_t can_transmit(const can_msg_t *msg)
{
    if (!msg) return -1;
    // TODO: 实现发送逻辑
    return -1;
}

/**
 * @brief  从接收 FIFO 读取一帧报文
 */
int32_t can_receive(uint8_t fifo_num, can_msg_t *msg)
{
    if (!msg) return -1;
    // TODO: 实现接收逻辑
    return -1;
}
