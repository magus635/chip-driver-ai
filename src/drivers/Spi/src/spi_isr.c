/**
 * @file    spi_isr.c
 * @brief   STM32F103C8T6 SPI 中断服务函数 (V4.0)
 */

#include "spi_reg.h"
#include "spi_ll.h"

/**
 * @brief  Shared SPI1 ISR
 */
void SPI1_IRQHandler(void)
{
    /* Clear flags or call callbacks here */
}

/**
 * @brief  Shared SPI2 ISR
 */
void SPI2_IRQHandler(void)
{
}
