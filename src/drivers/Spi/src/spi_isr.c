/**
 * @file    spi_isr.c
 * @brief   STM32F103C8T6 SPI 中断服务函数
 *
 * @note    仅包含向量表中断入口函数。
 *          所有逻辑处理委托给 Spi_IRQHandler()（在 spi_drv.c 中实现）。
 *          IRQ 编号：SPI1=35，SPI2=36
 *          Source: RM0008 §10.1.2 Table 63 p.196-197
 *
 * ISR 职责边界（架构规范强制要求）：
 *   步骤 1: 通过 LL API 读取并清除硬件中断标志（在 Spi_IRQHandler 中完成）
 *   步骤 2: 读取状态/数据寄存器，写入共享数据结构（在 Spi_IRQHandler 中完成）
 *   步骤 3: 调用回调通知上层（在 Spi_IRQHandler 中完成）
 *   禁止: 调用任何阻塞 OS API
 *   禁止: 直接操作 _drv 层状态机（通过 Spi_IRQHandler 间接推进）
 */

#include "spi_api.h"

/*===========================================================================*/
/* SPI1 global interrupt handler                                              */
/* IRQ number: 35                                                             */
/* Source: RM0008 §10.1.2 Table 63 p.196; §25.3.11 Table 182 p.694          */
/*===========================================================================*/

void SPI1_IRQHandler(void)
{
    /* Delegate all flag handling, data movement and callback to driver layer */
    Spi_IRQHandler(SPI_INSTANCE_1);
}

/*===========================================================================*/
/* SPI2 global interrupt handler                                              */
/* IRQ number: 36                                                             */
/* Source: RM0008 §10.1.2 Table 63 p.197; §25.3.11 Table 182 p.694          */
/*===========================================================================*/

void SPI2_IRQHandler(void)
{
    /* Delegate all flag handling, data movement and callback to driver layer */
    Spi_IRQHandler(SPI_INSTANCE_2);
}
