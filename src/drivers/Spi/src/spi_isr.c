/**
 * @file    spi_isr.c
 * @brief   STM32F103C8T6 SPI 中断服务函数
 *
 * @note    ISR 职责边界：通过 LL API 清除标志 → 派发到 Spi_IRQHandler。
 *          禁止在 ISR 中调用阻塞 OS API 或直接操作 drv 层状态机。
 *
 * Source:  ir/spi_ir_summary.json interrupts[]
 *          RM0008 Rev 21, Table 63 pp.194-195
 */

#include "spi_api.h"

/*===========================================================================*/
/* SPI1 IRQ Handler                                                           */
/* IRQ number: 35 — ir/spi_ir_summary.json interrupts[0]                    */
/* Handler: SPI1_IRQHandler — RM0008 Table 63 p.194                          */
/*===========================================================================*/

void SPI1_IRQHandler(void)
{
    Spi_IRQHandler(SPI_INSTANCE_1);
}

/*===========================================================================*/
/* SPI2 IRQ Handler                                                           */
/* IRQ number: 36 — ir/spi_ir_summary.json interrupts[1]                    */
/* Handler: SPI2_IRQHandler — RM0008 Table 63 p.195                          */
/*===========================================================================*/

void SPI2_IRQHandler(void)
{
    Spi_IRQHandler(SPI_INSTANCE_2);
}
