/**
 * src/bsp/include/can_bsp.h — CAN 外设寄存器定义 (STM32F407)
 *
 * 参考：RM0090 Chapter 32 - Controller area network (bxCAN)
 * 此文件定义 CAN 和 GPIO 寄存器结构体及位域宏
 */

#ifndef CAN_BSP_H
#define CAN_BSP_H

#include <stdint.h>

/* ── CAN 发送邮箱结构 ─────────────────────────────────────── */
typedef struct {
    volatile uint32_t TIR;      /**< 邮箱标识符寄存器       */
    volatile uint32_t TDTR;     /**< 数据长度和时间戳寄存器  */
    volatile uint32_t TDLR;     /**< 低位数据寄存器          */
    volatile uint32_t TDHR;     /**< 高位数据寄存器          */
} CAN_TxMailBox_TypeDef;

/* ── CAN 接收 FIFO 邮箱结构 ──────────────────────────────── */
typedef struct {
    volatile uint32_t RIR;      /**< 邮箱标识符寄存器       */
    volatile uint32_t RDTR;     /**< 数据长度和时间戳寄存器  */
    volatile uint32_t RDLR;     /**< 低位数据寄存器          */
    volatile uint32_t RDHR;     /**< 高位数据寄存器          */
} CAN_FIFOMailBox_TypeDef;

/* ── CAN 过滤器组寄存器结构 ──────────────────────────────── */
typedef struct {
    volatile uint32_t FR1;      /**< 过滤器寄存器 1 */
    volatile uint32_t FR2;      /**< 过滤器寄存器 2 */
} CAN_FilterRegister_TypeDef;

/* ── CAN 外设寄存器结构体 (RM0090 §32.9) ────────────────── */
typedef struct {
    volatile uint32_t MCR;          /**< 0x00 主控制寄存器            */
    volatile uint32_t MSR;          /**< 0x04 主状态寄存器            */
    volatile uint32_t TSR;          /**< 0x08 发送状态寄存器          */
    volatile uint32_t RF0R;         /**< 0x0C 接收FIFO 0寄存器       */
    volatile uint32_t RF1R;         /**< 0x10 接收FIFO 1寄存器       */
    volatile uint32_t IER;          /**< 0x14 中断使能寄存器          */
    volatile uint32_t ESR;          /**< 0x18 错误状态寄存器          */
    volatile uint32_t BTR;          /**< 0x1C 位时序寄存器            */
    uint32_t RESERVED0[88];         /**< 0x20 - 0x17F 保留            */
    CAN_TxMailBox_TypeDef sTxMailBox[3];       /**< 0x180 - 0x1AC */
    CAN_FIFOMailBox_TypeDef sFIFOMailBox[2];   /**< 0x1B0 - 0x1CC */
    uint32_t RESERVED1[12];         /**< 0x1D0 - 0x1FF 保留           */
    volatile uint32_t FMR;          /**< 0x200 过滤器主寄存器         */
    volatile uint32_t FM1R;         /**< 0x204 过滤器模式寄存器       */
    uint32_t RESERVED2;             /**< 0x208 保留                    */
    volatile uint32_t FS1R;         /**< 0x20C 过滤器比例寄存器       */
    uint32_t RESERVED3;             /**< 0x210 保留                    */
    volatile uint32_t FFA1R;        /**< 0x214 过滤器FIFO分配寄存器   */
    uint32_t RESERVED4;             /**< 0x218 保留                    */
    volatile uint32_t FA1R;         /**< 0x21C 过滤器激活寄存器       */
    uint32_t RESERVED5[8];          /**< 0x220 - 0x23F 保留           */
    CAN_FilterRegister_TypeDef sFilterRegister[28]; /**< 0x240 - 0x31F */
} CAN_TypeDef;

/* ── CAN 基地址 (RM0090 Table 1) ────────────────────────── */
#define CAN1_BASE   (0x40006400U)
#define CAN2_BASE   (0x40006800U)
#define CAN1        ((CAN_TypeDef *)CAN1_BASE)
#define CAN2        ((CAN_TypeDef *)CAN2_BASE)

/* ── CAN_MCR 位定义 (RM0090 §32.9.1) ────────────────────── */
#define CAN_MCR_INRQ    (1U << 0)   /**< 初始化请求          */
#define CAN_MCR_SLEEP   (1U << 1)   /**< 睡眠模式请求        */
#define CAN_MCR_TXFP    (1U << 2)   /**< 发送FIFO优先级      */
#define CAN_MCR_RFLM    (1U << 3)   /**< 接收FIFO锁定模式    */
#define CAN_MCR_NART    (1U << 4)   /**< 禁止自动重传        */
#define CAN_MCR_AWUM    (1U << 5)   /**< 自动唤醒模式        */
#define CAN_MCR_ABOM    (1U << 6)   /**< 自动离线管理        */
#define CAN_MCR_TTCM    (1U << 7)   /**< 时间触发通信模式    */
#define CAN_MCR_RESET   (1U << 15)  /**< 软件复位            */
#define CAN_MCR_DBF     (1U << 16)  /**< 调试冻结            */

/* ── CAN_MSR 位定义 (RM0090 §32.9.2) ────────────────────── */
#define CAN_MSR_INAK    (1U << 0)   /**< 初始化确认          */
#define CAN_MSR_SLAK    (1U << 1)   /**< 睡眠确认            */
#define CAN_MSR_ERRI    (1U << 2)   /**< 错误中断            */
#define CAN_MSR_WKUI    (1U << 3)   /**< 唤醒中断            */
#define CAN_MSR_SLAKI   (1U << 4)   /**< 睡眠确认中断        */
#define CAN_MSR_TXM     (1U << 8)   /**< 发送模式            */
#define CAN_MSR_RXM     (1U << 9)   /**< 接收模式            */
#define CAN_MSR_SAMP    (1U << 10)  /**< 上次采样点          */
#define CAN_MSR_RX      (1U << 11)  /**< CAN Rx 信号         */

/* ── CAN_TSR 位定义 (RM0090 §32.9.3) ────────────────────── */
#define CAN_TSR_RQCP0   (1U << 0)   /**< 邮箱0请求完成       */
#define CAN_TSR_TXOK0   (1U << 1)   /**< 邮箱0发送成功       */
#define CAN_TSR_ALST0   (1U << 2)   /**< 邮箱0仲裁丢失       */
#define CAN_TSR_TERR0   (1U << 3)   /**< 邮箱0发送错误       */
#define CAN_TSR_ABRQ0   (1U << 7)   /**< 中止邮箱0请求       */
#define CAN_TSR_RQCP1   (1U << 8)   /**< 邮箱1请求完成       */
#define CAN_TSR_TXOK1   (1U << 9)   /**< 邮箱1发送成功       */
#define CAN_TSR_ALST1   (1U << 10)  /**< 邮箱1仲裁丢失       */
#define CAN_TSR_TERR1   (1U << 11)  /**< 邮箱1发送错误       */
#define CAN_TSR_ABRQ1   (1U << 15)  /**< 中止邮箱1请求       */
#define CAN_TSR_RQCP2   (1U << 16)  /**< 邮箱2请求完成       */
#define CAN_TSR_TXOK2   (1U << 17)  /**< 邮箱2发送成功       */
#define CAN_TSR_ALST2   (1U << 18)  /**< 邮箱2仲裁丢失       */
#define CAN_TSR_TERR2   (1U << 19)  /**< 邮箱2发送错误       */
#define CAN_TSR_ABRQ2   (1U << 23)  /**< 中止邮箱2请求       */
#define CAN_TSR_CODE_Pos (24)
#define CAN_TSR_CODE_Msk (0x3U << CAN_TSR_CODE_Pos)
#define CAN_TSR_TME0    (1U << 26)  /**< 发送邮箱0空         */
#define CAN_TSR_TME1    (1U << 27)  /**< 发送邮箱1空         */
#define CAN_TSR_TME2    (1U << 28)  /**< 发送邮箱2空         */
#define CAN_TSR_LOW0    (1U << 29)  /**< 邮箱0最低优先级     */
#define CAN_TSR_LOW1    (1U << 30)  /**< 邮箱1最低优先级     */
#define CAN_TSR_LOW2    (1U << 31)  /**< 邮箱2最低优先级     */

/* ── CAN_RF0R 位定义 (RM0090 §32.9.4) ───────────────────── */
#define CAN_RF0R_FMP0_Pos  (0)
#define CAN_RF0R_FMP0_Msk  (0x3U << CAN_RF0R_FMP0_Pos)
#define CAN_RF0R_FULL0  (1U << 3)   /**< FIFO 0 满           */
#define CAN_RF0R_FOVR0  (1U << 4)   /**< FIFO 0 溢出         */
#define CAN_RF0R_RFOM0  (1U << 5)   /**< 释放FIFO 0输出邮箱  */

/* ── CAN_RF1R 位定义 (RM0090 §32.9.5) ───────────────────── */
#define CAN_RF1R_FMP1_Pos  (0)
#define CAN_RF1R_FMP1_Msk  (0x3U << CAN_RF1R_FMP1_Pos)
#define CAN_RF1R_FULL1  (1U << 3)   /**< FIFO 1 满           */
#define CAN_RF1R_FOVR1  (1U << 4)   /**< FIFO 1 溢出         */
#define CAN_RF1R_RFOM1  (1U << 5)   /**< 释放FIFO 1输出邮箱  */

/* ── CAN_IER 位定义 (RM0090 §32.9.6) ────────────────────── */
#define CAN_IER_TMEIE   (1U << 0)   /**< 发送邮箱空中断使能   */
#define CAN_IER_FMPIE0  (1U << 1)   /**< FIFO 0消息挂起中断   */
#define CAN_IER_FFIE0   (1U << 2)   /**< FIFO 0满中断         */
#define CAN_IER_FOVIE0  (1U << 3)   /**< FIFO 0溢出中断       */
#define CAN_IER_FMPIE1  (1U << 4)   /**< FIFO 1消息挂起中断   */
#define CAN_IER_FFIE1   (1U << 5)   /**< FIFO 1满中断         */
#define CAN_IER_FOVIE1  (1U << 6)   /**< FIFO 1溢出中断       */
#define CAN_IER_EWGIE   (1U << 8)   /**< 错误警告中断         */
#define CAN_IER_EPVIE   (1U << 9)   /**< 错误被动中断         */
#define CAN_IER_BOFIE   (1U << 10)  /**< 离线中断             */
#define CAN_IER_LECIE   (1U << 11)  /**< 上次错误码中断       */
#define CAN_IER_ERRIE   (1U << 15)  /**< 错误中断使能         */
#define CAN_IER_WKUIE   (1U << 16)  /**< 唤醒中断使能         */
#define CAN_IER_SLKIE   (1U << 17)  /**< 睡眠中断使能         */

/* ── CAN_ESR 位定义 (RM0090 §32.9.7) ────────────────────── */
#define CAN_ESR_EWGF    (1U << 0)   /**< 错误警告标志         */
#define CAN_ESR_EPVF    (1U << 1)   /**< 错误被动标志         */
#define CAN_ESR_BOFF    (1U << 2)   /**< 离线标志             */
#define CAN_ESR_LEC_Pos (4)
#define CAN_ESR_LEC_Msk (0x7U << CAN_ESR_LEC_Pos)
#define CAN_ESR_TEC_Pos (16)
#define CAN_ESR_TEC_Msk (0xFFU << CAN_ESR_TEC_Pos)
#define CAN_ESR_REC_Pos (24)
#define CAN_ESR_REC_Msk (0xFFU << CAN_ESR_REC_Pos)

/* ── CAN_BTR 位定义 (RM0090 §32.9.8) ────────────────────── */
#define CAN_BTR_BRP_Pos (0)
#define CAN_BTR_BRP_Msk (0x3FFU << CAN_BTR_BRP_Pos)  /**< 波特率预分频 */
#define CAN_BTR_TS1_Pos (16)
#define CAN_BTR_TS1_Msk (0xFU << CAN_BTR_TS1_Pos)    /**< 时间段1      */
#define CAN_BTR_TS2_Pos (20)
#define CAN_BTR_TS2_Msk (0x7U << CAN_BTR_TS2_Pos)    /**< 时间段2      */
#define CAN_BTR_SJW_Pos (24)
#define CAN_BTR_SJW_Msk (0x3U << CAN_BTR_SJW_Pos)    /**< 重同步跳跃宽度 */
#define CAN_BTR_LBKM    (1U << 30)  /**< 回环模式             */
#define CAN_BTR_SILM    (1U << 31)  /**< 静默模式             */

/* ── CAN_TIxR 位定义 (发送邮箱标识符) ────────────────────── */
#define CAN_TIR_TXRQ    (1U << 0)   /**< 发送请求             */
#define CAN_TIR_RTR     (1U << 1)   /**< 远程发送请求         */
#define CAN_TIR_IDE     (1U << 2)   /**< 标识符扩展           */
#define CAN_TIR_EXID_Pos (3)
#define CAN_TIR_EXID_Msk (0x3FFFFU << CAN_TIR_EXID_Pos) /**< 扩展ID   */
#define CAN_TIR_STID_Pos (21)
#define CAN_TIR_STID_Msk (0x7FFU << CAN_TIR_STID_Pos)   /**< 标准ID   */

/* ── CAN_TDTxR 位定义 (发送邮箱数据长度) ─────────────────── */
#define CAN_TDTR_DLC_Pos (0)
#define CAN_TDTR_DLC_Msk (0xFU << CAN_TDTR_DLC_Pos)   /**< 数据长度码  */
#define CAN_TDTR_TGT    (1U << 8)   /**< 发送全局时间         */
#define CAN_TDTR_TIME_Pos (16)
#define CAN_TDTR_TIME_Msk (0xFFFFU << CAN_TDTR_TIME_Pos)

/* ── CAN_RIxR 位定义 (接收邮箱标识符) ────────────────────── */
#define CAN_RIR_RTR     (1U << 1)   /**< 远程发送请求         */
#define CAN_RIR_IDE     (1U << 2)   /**< 标识符扩展           */
#define CAN_RIR_EXID_Pos (3)
#define CAN_RIR_EXID_Msk (0x3FFFFU << CAN_RIR_EXID_Pos)
#define CAN_RIR_STID_Pos (21)
#define CAN_RIR_STID_Msk (0x7FFU << CAN_RIR_STID_Pos)

/* ── CAN_RDTxR 位定义 (接收邮箱数据长度) ─────────────────── */
#define CAN_RDTR_DLC_Pos (0)
#define CAN_RDTR_DLC_Msk (0xFU << CAN_RDTR_DLC_Pos)
#define CAN_RDTR_FMI_Pos (8)
#define CAN_RDTR_FMI_Msk (0xFFU << CAN_RDTR_FMI_Pos)
#define CAN_RDTR_TIME_Pos (16)
#define CAN_RDTR_TIME_Msk (0xFFFFU << CAN_RDTR_TIME_Pos)

/* ── CAN_FMR 位定义 (过滤器主寄存器) ────────────────────── */
#define CAN_FMR_FINIT   (1U << 0)   /**< 过滤器初始化模式     */
#define CAN_FMR_CAN2SB_Pos (8)
#define CAN_FMR_CAN2SB_Msk (0x3FU << CAN_FMR_CAN2SB_Pos)

/* ── GPIO 寄存器结构体 (RM0090 §8.4) ────────────────────── */
typedef struct {
    volatile uint32_t MODER;    /**< 0x00 模式寄存器           */
    volatile uint32_t OTYPER;   /**< 0x04 输出类型寄存器       */
    volatile uint32_t OSPEEDR;  /**< 0x08 输出速度寄存器       */
    volatile uint32_t PUPDR;    /**< 0x0C 上拉/下拉寄存器     */
    volatile uint32_t IDR;      /**< 0x10 输入数据寄存器       */
    volatile uint32_t ODR;      /**< 0x14 输出数据寄存器       */
    volatile uint32_t BSRR;     /**< 0x18 位设置/复位寄存器   */
    volatile uint32_t LCKR;     /**< 0x1C 配置锁定寄存器       */
    volatile uint32_t AFR[2];   /**< 0x20 复用功能寄存器 [0]=AFRL [1]=AFRH */
} GPIO_TypeDef;

/* ── GPIO 基地址 (RM0090 Table 1) ────────────────────────── */
#define GPIOA_BASE  (0x40020000U)
#define GPIOB_BASE  (0x40020400U)
#define GPIOC_BASE  (0x40020800U)
#define GPIOD_BASE  (0x40020C00U)
#define GPIOA       ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB       ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC       ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD       ((GPIO_TypeDef *)GPIOD_BASE)

/* ── GPIO 配置常量 ──────────────────────────────────────── */
#define GPIO_MODER_AF       (0x2U)  /**< 复用功能模式         */
#define GPIO_OTYPER_PP      (0x0U)  /**< 推挽输出             */
#define GPIO_OSPEEDR_HIGH   (0x2U)  /**< 高速                 */
#define GPIO_PUPDR_NONE     (0x0U)  /**< 无上拉/下拉         */
#define GPIO_PUPDR_PU       (0x1U)  /**< 上拉                 */
#define GPIO_PUPDR_PD       (0x2U)  /**< 下拉                 */

/* ── CAN GPIO 复用功能编号 ──────────────────────────────── */
#define GPIO_AF9_CAN        (0x9U)  /**< AF9: CAN1/CAN2       */

/* ── RCC AHB1 时钟使能位 ────────────────────────────────── */
#define RCC_AHB1ENR_GPIOAEN (1U << 0)
#define RCC_AHB1ENR_GPIOBEN (1U << 1)
#define RCC_AHB1ENR_GPIOCEN (1U << 2)
#define RCC_AHB1ENR_GPIODEN (1U << 3)

#endif /* CAN_BSP_H */
