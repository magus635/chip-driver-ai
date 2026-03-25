/**
 * src/can_driver.c — CAN 驱动框架示例（供 AI 填写实现）
 *
 * 此文件是用户提供的驱动框架模板，AI 将在 TODO 标注处填写具体实现。
 * AI 必须参照 docs/ 目录下的芯片手册完成寄存器配置。
 *
 * 框架约定：
 *   - 错误码通过返回值传递，0 = 成功，负数 = 错误
 *   - 公共 API 声明在 src/include/can_driver.h 中
 *   - 不得修改函数签名
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
 * @param  baudrate_kbps  目标波特率（kbps），如 500、1000
 * @retval 0  成功
 *        -1  进入初始化模式超时
 *        -2  退出初始化模式超时
 *        -3  波特率参数无法满足
 */
int32_t can_init(uint32_t baudrate_kbps)
{
    /* TODO: 使能 CAN1 外设时钟
     * 参考：芯片手册 RCC 章节 —— APB1 外设时钟使能寄存器
     * 示例：RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;
     */

    /* TODO: 配置 CAN GPIO 引脚（TX/RX）为复用功能
     * 参考：芯片手册 GPIO 章节和引脚复用表
     */

    /* TODO: 请求进入初始化模式
     * 参考：芯片手册 CAN 章节 —— MCR 寄存器 INRQ 位
     */

    /* TODO: 等待硬件确认进入初始化模式（INAK 标志）
     * 使用 can_wait_inak() 内部函数，超时返回 -1
     */

    /* TODO: 关闭睡眠模式
     * 参考：MCR 寄存器 SLEEP 位
     */

    /* TODO: 计算并配置波特率寄存器 BTR
     * 使用 can_calc_btr() 内部函数
     * 参考：芯片手册 CAN_BTR 寄存器（BRP、TS1、TS2、SJW 字段）
     */

    /* TODO: 配置工作模式（正常模式/回环模式）
     * 参考：BTR 寄存器 LBKM、SILM 位
     */

    /* TODO: 退出初始化模式（清除 INRQ 位）*/

    /* TODO: 等待硬件确认退出初始化模式（INAK=0）
     * 超时返回 -2
     */

    g_can_init_done = 1;
    g_can_state = CAN_STATE_READY;
    return 0;
}

/**
 * @brief  发送一帧 CAN 报文
 * @param  msg  指向待发送报文结构体的指针
 * @retval 0  成功放入发送邮箱
 *        -1  所有发送邮箱均忙
 *        -2  参数无效
 */
int32_t can_transmit(const can_msg_t *msg)
{
    if (msg == NULL) return -2;
    if (g_can_state != CAN_STATE_READY) return -3;

    /* TODO: 查找空闲发送邮箱
     * 参考：TSR 寄存器 TME0/TME1/TME2 位
     */

    /* TODO: 填写发送邮箱（ID、DLC、数据）
     * 参考：TIxR、TDTxR、TDLxR、TDHxR 寄存器
     */

    /* TODO: 请求发送（设置 TXRQ 位）*/

    g_can_tx_count++;
    return 0;
}

/**
 * @brief  从接收 FIFO 读取一帧报文
 * @param  fifo_num  FIFO 编号（0 或 1）
 * @param  msg       输出：接收到的报文
 * @retval 0  成功
 *        -1  FIFO 为空
 */
int32_t can_receive(uint8_t fifo_num, can_msg_t *msg)
{
    if (msg == NULL) return -2;

    /* TODO: 检查 FIFO 是否有待读报文
     * 参考：RF0R/RF1R 寄存器 FMP 字段
     */

    /* TODO: 读取报文 ID、DLC、数据
     * 参考：RIxR、RDTxR、RDLxR、RDHxR 寄存器
     */

    /* TODO: 释放 FIFO（设置 RFOM 位）*/

    g_can_rx_count++;
    return 0;
}

/* ── 内部函数实现 ────────────────────────────────────────── */

/**
 * @brief 等待 INAK 标志（进入或退出初始化模式的确认）
 * @param timeout_ms  超时毫秒数
 */
static int32_t can_wait_inak(uint32_t timeout_ms)
{
    /* TODO: 实现带超时的轮询等待
     * 参考：MSR 寄存器 INAK 位
     */
    (void)timeout_ms;
    return 0;
}

/**
 * @brief  根据波特率和总线时钟计算 BTR 寄存器值
 * @param  baudrate_kbps  目标波特率
 * @param  apb1_mhz       APB1 时钟（MHz）
 * @param  btr_val        输出：BTR 寄存器值
 */
static int32_t can_calc_btr(uint32_t baudrate_kbps, uint32_t apb1_mhz, uint32_t *btr_val)
{
    /* TODO: 实现 BRP、TS1、TS2 字段的计算
     * 公式参考：芯片手册 CAN 章节波特率计算说明
     * 约束：采样点建议 75%~87.5%，SJW 建议 1
     */
    (void)baudrate_kbps;
    (void)apb1_mhz;
    *btr_val = 0;
    return -3; /* AI 实现后移除此返回 */
}
