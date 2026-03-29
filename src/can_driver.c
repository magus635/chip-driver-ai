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

#define CAN1_BASE        0x40006400U
#define RCC_AHB1ENR      (*((volatile uint32_t *)0x40023830U))
#define RCC_APB1ENR      (*((volatile uint32_t *)0x40023840U))
#define RCC_AHB1ENR_GPIOAEN (1U << 0)
#define RCC_APB1ENR_CAN1EN  (1U << 25)

/* GPIOA (PA11, PA12) */
#define GPIOA_BASE       0x40020000U
#define GPIOA_MODER      (*((volatile uint32_t *)(GPIOA_BASE + 0x00U)))
#define GPIOA_OTYPER     (*((volatile uint32_t *)(GPIOA_BASE + 0x04U)))
#define GPIOA_OSPEEDR    (*((volatile uint32_t *)(GPIOA_BASE + 0x08U)))
#define GPIOA_PUPDR      (*((volatile uint32_t *)(GPIOA_BASE + 0x0CU)))
#define GPIOA_AFRH       (*((volatile uint32_t *)(GPIOA_BASE + 0x24U))) // Pins 8-15

/* CAN1 Registers */
#define CAN_MCR          (*((volatile uint32_t *)(CAN1_BASE + 0x00U)))
#define CAN_MSR          (*((volatile uint32_t *)(CAN1_BASE + 0x04U)))
#define CAN_TSR          (*((volatile uint32_t *)(CAN1_BASE + 0x08U)))
#define CAN_RF0R         (*((volatile uint32_t *)(CAN1_BASE + 0x0CU)))
#define CAN_BTR          (*((volatile uint32_t *)(CAN1_BASE + 0x1CU)))

/* Mailbox Registers */
#define CAN_TI0R         (*((volatile uint32_t *)(CAN1_BASE + 0x180U)))
#define CAN_TDT0R        (*((volatile uint32_t *)(CAN1_BASE + 0x184U)))
#define CAN_TDL0R        (*((volatile uint32_t *)(CAN1_BASE + 0x188U)))
#define CAN_TDH0R        (*((volatile uint32_t *)(CAN1_BASE + 0x18CU)))

#define CAN_RI0R         (*((volatile uint32_t *)(CAN1_BASE + 0x1B0U)))
#define CAN_RDT0R        (*((volatile uint32_t *)(CAN1_BASE + 0x1B4U)))
#define CAN_RDL0R        (*((volatile uint32_t *)(CAN1_BASE + 0x1B8U)))
#define CAN_RDH0R        (*((volatile uint32_t *)(CAN1_BASE + 0x1BCU)))

/* Filter Registers */
#define CAN_FMR          (*((volatile uint32_t *)(CAN1_BASE + 0x200U)))
#define CAN_FA1R         (*((volatile uint32_t *)(CAN1_BASE + 0x21CU)))
#define CAN_F0R1         (*((volatile uint32_t *)(CAN1_BASE + 0x240U)))
#define CAN_F0R2         (*((volatile uint32_t *)(CAN1_BASE + 0x244U)))

/**
 * @brief  初始化 CAN 控制器
 */
int32_t can_init(uint32_t baudrate_kbps)
{
    // 1. 使能外设时钟 (GPIOA & CAN1)
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC_APB1ENR |= RCC_APB1ENR_CAN1EN;

    // 2. 配置 PA11(RX) 和 PA12(TX) 复用模式 (AF9)
    // MODER: 10 (AF) for PIN11 and PIN12
    GPIOA_MODER &= ~((3U << 22) | (3U << 24));
    GPIOA_MODER |= ((2U << 22) | (2U << 24));
    
    // OTYPER: Push-Pull (0)
    GPIOA_OTYPER &= ~((1U << 11) | (1U << 12));
    
    // OSPEEDR: High speed (10)
    GPIOA_OSPEEDR &= ~((3U << 22) | (3U << 24));
    GPIOA_OSPEEDR |= ((2U << 22) | (2U << 24));
    
    // PUPDR: Pull-up (01)
    GPIOA_PUPDR &= ~((3U << 22) | (3U << 24));
    GPIOA_PUPDR |= ((1U << 22) | (1U << 24));
    
    // AFRH: AF9 for Pin 11, 12. bit 12-15 and 16-19
    GPIOA_AFRH &= ~((0xFU << 12) | (0xFU << 16));
    GPIOA_AFRH |= ((0x9U << 12) | (0x9U << 16));

    // 3. 请求进入初始化模式
    CAN_MCR |= (1U << 0); // INRQ = 1
    while (!(CAN_MSR & (1U << 0))) {} // 等待 INAK = 1

    CAN_MCR &= ~(1U << 1); // 退出清睡眠模式 (SLEEP = 0)
    CAN_MCR |= (1U << 6);  // 自动离线管理 (ABOM = 1)

    // 4. 配置波特率 (假设 APB1=42MHz, 配置 500kbps)
    if (baudrate_kbps == 500) {
        // SJW=1(0<<24), TS2=2(1<<20), TS1=11(10<<16), BRP=6(5) -> 42M/(6*(1+11+2))=500k
        CAN_BTR = (0U << 24) | (1U << 20) | (10U << 16) | 5U;
    }

    // 5. 退出初始化模式
    CAN_MCR &= ~(1U << 0); // INRQ = 0
    while (CAN_MSR & (1U << 0)) {} // 等待 INAK = 0

    // 6. 配置过滤器 (接收一切)
    CAN_FMR |= (1U << 0);  // FINIT = 1
    CAN_FA1R |= 1U;        // 激活过滤器 0
    CAN_F0R1 = 0;          // ID = 0, Mask = 0
    CAN_F0R2 = 0;
    CAN_FMR &= ~(1U << 0); // FINIT = 0

    g_can_init_done = 1;
    return 0; 
}

/**
 * @brief  发送一帧 CAN 报文
 */
int32_t can_transmit(const can_msg_t *msg)
{
    if (!msg) return -1;
    
    // 检查邮箱 0 是否空闲 (TME0)
    if (!(CAN_TSR & (1U << 26))) {
        return -1; // 满
    }

    // 设置 ID
    if (msg->ide == 0) {
        CAN_TI0R = (msg->id << 21);
    } else {
        CAN_TI0R = (msg->id << 3) | (1U << 2);
    }

    // RTR: Remote Transmission Request
    if (msg->rtr == 1) {
        CAN_TI0R |= (1U << 1);
    }

    // DLC
    CAN_TDT0R = msg->dlc & 0x0F;

    // Data
    CAN_TDL0R = msg->data[0] | (msg->data[1] << 8) | (msg->data[2] << 16) | (msg->data[3] << 24);
    CAN_TDH0R = msg->data[4] | (msg->data[5] << 8) | (msg->data[6] << 16) | (msg->data[7] << 24);

    // 发送请求 TXRQ
    CAN_TI0R |= 1U;
    
    g_can_tx_count++;
    return 0;
}

/**
 * @brief  从接收 FIFO 读取一帧报文
 */
int32_t can_receive(uint8_t fifo_num, can_msg_t *msg)
{
    if (!msg) return -1;
    (void)fifo_num; // 仅示例使用 FIFO0

    // 检查 FIFO0 报文数量 FMP0
    if ((CAN_RF0R & 0x03) == 0) {
        return -1; // 空
    }

    msg->ide = (CAN_RI0R >> 2) & 1U;
    if (msg->ide == 0) {
        msg->id = CAN_RI0R >> 21;
    } else {
        msg->id = CAN_RI0R >> 3;
    }

    msg->rtr = (CAN_RI0R >> 1) & 1U;
    msg->dlc = CAN_RDT0R & 0x0F;

    uint32_t low = CAN_RDL0R;
    uint32_t high = CAN_RDH0R;
    
    msg->data[0] = (uint8_t)(low & 0xFF);
    msg->data[1] = (uint8_t)((low >> 8) & 0xFF);
    msg->data[2] = (uint8_t)((low >> 16) & 0xFF);
    msg->data[3] = (uint8_t)((low >> 24) & 0xFF);
    msg->data[4] = (uint8_t)(high & 0xFF);
    msg->data[5] = (uint8_t)((high >> 8) & 0xFF);
    msg->data[6] = (uint8_t)((high >> 16) & 0xFF);
    msg->data[7] = (uint8_t)((high >> 24) & 0xFF);

    // 释放 FIFO0 输出邮箱 RFOM0
    CAN_RF0R |= (1U << 5);

    g_can_rx_count++;
    return 0;
}
