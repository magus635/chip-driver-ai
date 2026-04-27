# GPIO + AFIO Peripheral IR Summary

<!-- ir_json_sha256: 91c72228a5ce6367e0ca00a921cbff62bfd46257d98b9905f0eb052807d2970e -->

**Chip:** STM32F103C8T6 (STM32F1)
**Peripheral:** GPIO_AFIO — General-purpose I/O and Alternate Function I/O
**Source:** RM0008 Rev 21, §9.1-§9.5
**Safety level:** QM

> 本 MD 是 `ir/gpio_afio_ir_summary.json` 的只读视图。**请勿手工编辑**。

## Instances

- **GPIOA** @ `0x40010800` (APB2)
- **GPIOB** @ `0x40010C00` (APB2)
- **GPIOC** @ `0x40011000` (APB2)
- **GPIOD** @ `0x40011400` (APB2)
- **GPIOE** @ `0x40011800` (APB2)
- **GPIOF** @ `0x40011C00` (APB2) — STM32F103C8T6 (LQFP48) 不引出 GPIOF/G，但寄存器空间存在
- **GPIOG** @ `0x40012000` (APB2) — 见 GPIOF
- **AFIO** @ `0x40010000` (APB2)

## Registers

| Name | Offset | Access | Description |
|---|---|---|---|
| `GPIO_CRL` | 0x00 | RW | GPIO 引脚 0~7 的模式与配置位（每引脚 4 bit：MODE[1:0] + CNF[1:0]） |
| `GPIO_CRH` | 0x04 | RW | 引脚 8~15 的模式与配置位 |
| `GPIO_IDR` | 0x08 | R | 引脚电平输入采样（只读，每 APB2 周期更新） |
| `GPIO_ODR` | 0x0C | RW | 引脚输出寄存器；建议通过 BSRR/BRR 原子修改，避免读改写竞态 |
| `GPIO_BSRR` | 0x10 | W | 原子置位/清零；高 16 位写 1 清 ODRx，低 16 位写 1 置 ODRx；读返回 0 |
| `GPIO_BRR` | 0x14 | W | 原子清零（功能等价于 BSRR 高 16 位） |
| `GPIO_LCKR` | 0x18 | RW | 锁定 CRL/CRH 配置；通过 LCKK 写序列激活，复位前不可解锁 |
| `AFIO_EVCR` | 0x00 | RW | Cortex EVENTOUT 事件输出引脚选择 |
| `AFIO_MAPR` | 0x04 | RW | 外设引脚重映射与 SWJ 调试接口配置 |
| `AFIO_EXTICR1` | 0x08 | RW | EXTI0~3 引脚源选择（0=PA,1=PB,...,6=PG） |
| `AFIO_EXTICR2` | 0x0C | RW | EXTI4~7 引脚源选择 |
| `AFIO_EXTICR3` | 0x10 | RW | EXTI8~11 引脚源选择 |
| `AFIO_EXTICR4` | 0x14 | RW | EXTI12~15 引脚源选择 |
| `AFIO_MAPR2` | 0x1C | RW | 扩展定时器重映射（TIM9~14、FSMC_NADV） |

## Operation modes

- **INPUT_FLOATING**: MODE=00, CNF=01 — 浮空输入
- **INPUT_PULL_UP**: MODE=00, CNF=10, ODR.bit=1 — 上拉输入
- **INPUT_PULL_DOWN**: MODE=00, CNF=10, ODR.bit=0 — 下拉输入
- **INPUT_ANALOG**: MODE=00, CNF=00 — 模拟输入（ADC/DAC 引脚必选）
- **OUTPUT_PP_10MHZ**: MODE=01, CNF=00 — 推挽输出 10MHz
- **OUTPUT_PP_2MHZ**: MODE=10, CNF=00 — 推挽输出 2MHz
- **OUTPUT_PP_50MHZ**: MODE=11, CNF=00 — 推挽输出 50MHz
- **OUTPUT_OD**: MODE=01/10/11, CNF=01 — 开漏输出
- **OUTPUT_AF_PP**: MODE=01/10/11, CNF=10 — 复用推挽输出
- **OUTPUT_AF_OD**: MODE=01/10/11, CNF=11 — 复用开漏输出

## Invariants (R8 守卫)

- **INV_GPIO_001** [WARNING]: 锁定的引脚配置位在复位前不可修改 — `LCKR.LCKK == 1 && LCKR.LCK[n] == 1 implies !writable(CRL/CRH.{MODE,CNF}[n])`
- **INV_GPIO_002** [CRITICAL]: AFIO 寄存器访问前必须先使能 AFIO 时钟 — `AFIOEN == 0 implies !writable(AFIO_*)`
- **INV_GPIO_003** [CRITICAL]: GPIO 寄存器访问前必须先使能 GPIOx 时钟 — `IOPxEN == 0 implies !writable(GPIOx_*)`

## Atomic sequences

- **SEQ_LCKR_LOCK**: GPIO 引脚配置锁定（一旦完成在复位前不可解除）
- **SEQ_BSRR_ATOMIC_WRITE**: 原子置位/清零，避免 ODR 读改写竞态

## Init sequence

1. (drv) RCC_APB2ENR |= AFIOEN | IOPxEN — 使能目标 GPIO bank + AFIO 时钟（必须最先）
2. (ll) GPIOx->CRL/CRH 写 MODE+CNF — 配置每个引脚 4-bit 字段
3. (drv) AFIO_MAPR / EXTICRn (可选) — 需重映射或 EXTI 时设置 AFIO 寄存器
4. (ll) GPIOx->ODR / BSRR (可选) — 设定输出引脚初值
5. (drv) GPIOx->LCKR LOCK 序列 (可选) — 对必须锁定的引脚走 LOCK 序列；之后不可改

## Errors

- `GPIO_PARAM` (1): 无效端口/引脚号 → 调用方修正参数
- `GPIO_LOCKED` (2): 引脚已被 LCKR 锁定，无法修改 → 复位 MCU 才能解锁
- `GPIO_LOCK_ABORT` (3): LOCK 序列写错，硬件已 abort → 重新执行 SEQ_LCKR_LOCK

## Pending reviews

- GPIO_PR_001 [ambiguous_behavior]: BSRR 高低 16 位同位同时写 1 时硬件优先级
