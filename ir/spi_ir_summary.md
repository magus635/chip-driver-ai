# SPI IR 摘要 — STM32F103C8T6

**版本**: IR 3.0 | **Safety Level**: QM | **来源**: RM0008 Rev 21 | **审计日期**: 2026-04-12
**置信度**: 0.95 (经 PDF 交叉验证)

---

## 设备约束

- **型号**: STM32F103C8T6 (Medium-Density, LQFP48)
- **可用 SPI**: SPI1, SPI2 (无 SPI3 — 仅 High-Density/Connectivity Line 有)
- **可用 DMA**: DMA1 (7 通道，无 DMA2)

---

## 外设实例

| 实例 | 基地址       | 总线 | 时钟频率  | 最大波特率 |
|------|-------------|------|----------|-----------|
| SPI1 | 0x40013000  | APB2 | 72 MHz   | 36 MHz (fPCLK/2) |
| SPI2 | 0x40003800  | APB1 | 36 MHz   | 18 MHz (fPCLK/2) |

---

## 寄存器映射

| 寄存器    | 偏移 | 位宽 | 复位值 | Access | 用途 |
|-----------|------|------|--------|--------|------|
| CR1       | 0x00 | 16   | 0x0000 | RW     | 主控制 (模式/时钟/使能) |
| CR2       | 0x04 | 16   | 0x0000 | RW     | 中断/DMA 使能 |
| SR        | 0x08 | 16   | 0x0002 | mixed  | 状态标志 (含 W0C/RC_SEQ 类型) |
| DR        | 0x0C | 16   | 0x0000 | RW     | 数据寄存器 |
| CRCPR     | 0x10 | 16   | 0x0007 | RW     | CRC 多项式 |
| RXCRCR    | 0x14 | 16   | 0x0000 | RO     | 接收 CRC 值 |
| TXCRCR    | 0x18 | 16   | 0x0000 | RO     | 发送 CRC 值 |
| I2SCFGR   | 0x1C | 16   | 0x0000 | RW     | I2S 配置 (SPI模式下 I2SMOD=0) |
| I2SPR     | 0x20 | 16   | 0x0002 | RW     | I2S 预分频 (SPI模式不用) |

---

## SR 状态寄存器 Access Type 详解

| 位域    | Access   | 清除方式                        | 来源 |
|---------|----------|---------------------------------|------|
| BSY     | RO       | 硬件自动清除                    | §25.5.3 |
| OVR     | RC_SEQ   | 依次读 DR → 读 SR               | §25.3.10 |
| MODF    | RC_SEQ   | 先读 SR → 再写 CR1               | §25.3.10 |
| CRCERR  | W0C      | 写 0 清除 (rc_w0)               | §25.5.3 |
| UDR     | RO       | 仅 I2S 模式                     | §25.5.3 |
| TXE     | RO       | 写 DR 时硬件清除                 | §25.5.3 |
| RXNE    | RO       | 读 DR 时硬件清除                 | §25.5.3 |

---

## 时钟配置

| 实例 | 使能寄存器    | 位  | 字段名  | 复位寄存器    | 位  |
|------|-------------|-----|---------|-------------|-----|
| SPI1 | RCC_APB2ENR | 12  | SPI1EN  | RCC_APB2RSTR | 12  |
| SPI2 | RCC_APB1ENR | 14  | SPI2EN  | RCC_APB1RSTR | 14  |

---

## 中断

| 实例 | IRQ | Priority | Handler          | 触发源 |
|------|-----|----------|------------------|--------|
| SPI1 | 35  | 42       | SPI1_IRQHandler  | TXE, RXNE, OVR, MODF, CRCERR |
| SPI2 | 36  | 43       | SPI2_IRQHandler  | TXE, RXNE, OVR, MODF, CRCERR |

---

## DMA 通道

| 实例 | 方向 | 控制器 | 通道 | 使能位       |
|------|------|--------|------|-------------|
| SPI1 | RX   | DMA1   | 2    | CR2.RXDMAEN |
| SPI1 | TX   | DMA1   | 3    | CR2.TXDMAEN |
| SPI2 | RX   | DMA1   | 4    | CR2.RXDMAEN |
| SPI2 | TX   | DMA1   | 5    | CR2.TXDMAEN |

---

## GPIO 引脚

### SPI1 (默认引脚, AFIO_MAPR.SPI1_REMAP=0)

| 信号 | 引脚 | 模式         | 备注 |
|------|------|-------------|------|
| SCK  | PA5  | AF_PP 50MHz | — |
| MISO | PA6  | Input_Float | — |
| MOSI | PA7  | AF_PP 50MHz | — |
| NSS  | PA4  | AF_PP 50MHz | 硬件 NSS; 软件 NSS 用 GPIO |

### SPI1 (重映射引脚, AFIO_MAPR.SPI1_REMAP=1)

| 信号 | 引脚  | 模式         | 备注 |
|------|-------|-------------|------|
| SCK  | PB3   | AF_PP 50MHz | 与 JTDO/TRACESWO 冲突 |
| MISO | PB4   | Input_Float | 与 NJTRST 冲突 |
| MOSI | PB5   | AF_PP 50MHz | — |
| NSS  | PA15  | AF_PP 50MHz | 与 JTDI 冲突 |

⚠️ 使用重映射引脚需先设置 `AFIO_MAPR.SWJ_CFG=0b010` (SWD-only, 禁用 JTAG)

### SPI2 (固定引脚, 无重映射)

| 信号 | 引脚  | 模式         |
|------|-------|-------------|
| SCK  | PB13  | AF_PP 50MHz |
| MISO | PB14  | Input_Float |
| MOSI | PB15  | AF_PP 50MHz |
| NSS  | PB12  | AF_PP 50MHz |

---

## 初始化序列

1. 使能 SPI 外设时钟 (RCC_APB1ENR/APB2ENR)
2. 配置 GPIO 引脚 (SCK/MOSI: AF_PP; MISO: Input_Float; NSS 按需)
3. 写 CR1: CPOL, CPHA, BR, MSTR, SSM/SSI, DFF, LSBFIRST (SPE=0)
4. 写 CR2: SSOE, DMA使能, 中断使能
5. 置 CR1.SPE=1 使能 SPI (此后 BR/CPOL/CPHA 不可修改)

---

## 不变式 (Invariants)

| ID          | 表达式 | 含义 | 严重度 |
|-------------|--------|------|--------|
| INV_SPI_001 | MSTR=1 && SSM=1 ⇒ SSI=1 | 软件 NSS 主模式必须拉高 SSI | ERROR |
| INV_SPI_002 | SPE=1 ⇒ BR/CPOL/CPHA 不可写 | 使能后禁改时钟参数 | ERROR |
| INV_SPI_003 | BSY=1 ⇒ SPE 不可清零 | 忙时禁止关闭 | WARNING |
| INV_SPI_004 | SPE=1 ⇒ CRCEN/DFF 不可写 | 使能后禁改帧格式 | ERROR |

---

## 审计变更记录

- 2026-04-12: 删除 SPI3 (C8T6 上不存在)
- 2026-04-12: 删除 DMA2 引用 (C8T6 只有 DMA1)
- 2026-04-12: 修正 CR1.LSBFIRST 描述中误引的 TI mode/FRF (F103 无此功能)
- 2026-04-12: 补充 SPI1 AFIO_MAPR 重映射引脚 (PA15/PB3/PB4/PB5)
- 2026-04-12: chip_model 精确化为 STM32F103C8T6
