/**
 * @file    gpioafio_isr.c
 * @brief   GPIO_AFIO 模块 ISR 豁免声明（codegen-agent.md §2.7）
 *
 * GPIO 自身无 IRQ：所有引脚级中断由 EXTI 模块（ARM Cortex-M3 NVIC）处理；
 * AFIO 仅参与 EXTI 源选择寄存器，无独立 IRQHandler。
 * 本文件保留为占位符，以满足 check-arch #9 的"必交付文件清单"约束。
 */
