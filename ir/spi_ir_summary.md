# SPI 外设文档摘要
**芯片**: STM32F103 (STM32F1 系列)
**手册**: RM0008 Rev 21
**章节**: §25 SPI (pp.672–722)
**生成时间**: 2026-04-12

---

## 1. 外设实例与基地址

| 实例 | 基地址 | 总线 | 时钟频率 | 来源 |
|------|--------|------|---------|------|
| SPI1 | 0x40013000 | APB2 | 72 MHz (最高) | RM0008 §3.3 Table 3 p.51 |
| SPI2 | 0x40003800 | APB1 | 36 MHz | RM0008 §3.3 Table 3 p.52 |
| SPI3 | 0x40003C00 | APB1 | 36 MHz | RM0008 §3.3 Table 3 p.52 |

---

## 2. 寄存器总览

| 寄存器 | 偏移 | 位宽 | 复位值 | access | 说明 |
|--------|------|------|--------|--------|------|
| CR1     | 0x00 | 16 | 0x0000 | RW     | 控制寄存器1（模式/时钟/帧格式）|
| CR2     | 0x04 | 16 | 0x0000 | RW     | 控制寄存器2（中断/DMA使能）|
| SR      | 0x08 | 16 | 0x0002 | mixed  | 状态寄存器（含 W0C/RC_SEQ 字段）|
| DR      | 0x0C | 16 | 0x0000 | RW     | 数据寄存器（读/写触发不同效果）|
| CRCPR   | 0x10 | 16 | 0x0007 | RW     | CRC 多项式寄存器 |
| RXCRCR  | 0x14 | 16 | 0x0000 | RO     | 接收 CRC 寄存器 |
| TXCRCR  | 0x18 | 16 | 0x0000 | RO     | 发送 CRC 寄存器 |
| I2SCFGR | 0x1C | 16 | 0x0000 | RW     | I2S 配置（SPI 模式下 I2SMOD=0）|
| I2SPR   | 0x20 | 16 | 0x0002 | RW     | I2S 预分频（SPI 模式下不使用）|

### SR 字段 access type 详情（关键）

| 字段 | 位 | access | 清除方法 |
|------|----|--------|---------|
| BSY    | 7 | RO     | 硬件自动清除 |
| OVR    | 6 | RC_SEQ | 读 DR，再读 SR（SEQ_OVR_CLEAR）|
| MODF   | 5 | RC_SEQ | 读 SR，再写 CR1（SEQ_MODF_CLEAR）|
| CRCERR | 4 | W0C    | 向该位写 0 |
| UDR    | 3 | RO     | I2S 从机专用，SPI 模式不使用 |
| CHSIDE | 2 | RO     | I2S 专用 |
| TXE    | 1 | RO     | 硬件自动清除 |
| RXNE   | 0 | RO     | 读 DR 自动清除 |

---

## 3. 时钟与复位配置

| 实例 | 时钟使能寄存器 | 使能位 | 复位寄存器 | 复位位 |
|------|-------------|--------|-----------|--------|
| SPI1 | RCC_APB2ENR | bit 12 (SPI1EN) | RCC_APB2RSTR | bit 12 (SPI1RST) |
| SPI2 | RCC_APB1ENR | bit 14 (SPI2EN) | RCC_APB1RSTR | bit 14 (SPI2RST) |
| SPI3 | RCC_APB1ENR | bit 15 (SPI3EN) | RCC_APB1RSTR | bit 15 (SPI3RST) |

---

## 4. 中断向量

| 实例 | IRQ 编号 | 优先级编号 | Handler |
|------|---------|-----------|---------|
| SPI1 | 35 | 42 | SPI1_IRQHandler |
| SPI2 | 36 | 43 | SPI2_IRQHandler |
| SPI3 | 51 | 58 | SPI3_IRQHandler |

中断源（共用一个向量）：TXE（CR2.TXEIE）、RXNE（CR2.RXNEIE）、OVR/MODF/CRCERR（CR2.ERRIE）

---

## 5. DMA 通道

| 实例 | 方向 | DMA 控制器 | 通道 | 触发标志 | 使能位 |
|------|------|-----------|------|---------|--------|
| SPI1 | RX | DMA1 | CH2 | SR.RXNE | CR2.RXDMAEN |
| SPI1 | TX | DMA1 | CH3 | SR.TXE  | CR2.TXDMAEN |
| SPI2 | RX | DMA1 | CH4 | SR.RXNE | CR2.RXDMAEN |
| SPI2 | TX | DMA1 | CH5 | SR.TXE  | CR2.TXDMAEN |
| SPI3 | RX | DMA2 | CH1 | SR.RXNE | CR2.RXDMAEN |
| SPI3 | TX | DMA2 | CH2 | SR.TXE  | CR2.TXDMAEN |

---

## 6. GPIO 引脚配置

| 实例 | SCK | MISO | MOSI | NSS |
|------|-----|------|------|-----|
| SPI1 | PA5 (AF_PP 50MHz) | PA6 (Input_Float) | PA7 (AF_PP 50MHz) | PA4 (AF_PP / GPIO out) |
| SPI2 | PB13 (AF_PP 50MHz) | PB14 (Input_Float) | PB15 (AF_PP 50MHz) | PB12 (AF_PP / GPIO out) |
| SPI3 | PB3 (AF_PP 50MHz) | PB4 (Input_Float) | PB5 (AF_PP 50MHz) | PA15 (AF_PP / GPIO out) |

---

## 7. 初始化序列

1. 使能 RCC 时钟（RCC_APB1/2ENR 对应 SPIxEN 位）
2. 配置 GPIO：SCK/MOSI/NSS → AF_PP 50MHz；MISO → Input_Float
3. 写 CR1（**SPE=0**）：设置 CPOL、CPHA、BR、MSTR、SSM/SSI、DFF、LSBFIRST
4. 写 CR2：设置 SSOE、TXDMAEN/RXDMAEN（DMA 模式）或 TXEIE/RXNEIE/ERRIE（中断模式）
5. 置 CR1.SPE=1 使能 SPI（**必须最后执行**）

> **关键约束**：步骤 3 的 BR/CPOL/CPHA 在 SPE=1 后不可修改（INV_SPI_002）

---

## 8. 硬件不变式（Invariants）

| ID | 表达式 | 严重性 | 说明 |
|----|--------|--------|------|
| INV_SPI_001 | CR1.MSTR==1 && CR1.SSM==1 → CR1.SSI==1 | ERROR | 软件 NSS 主机模式必须 SSI=1，否则触发 MODF |
| INV_SPI_002 | CR1.SPE==1 → !writable(CR1.BR/CPOL/CPHA) | ERROR | SPE 有效期间禁止修改时钟参数 |
| INV_SPI_003 | SR.BSY==1 → !writable(CR1.SPE) | WARNING | BSY 期间禁止清 SPE |
| INV_SPI_004 | CR1.SPE==1 → !writable(CR1.CRCEN/DFF) | ERROR | SPE 有效期间禁止修改 CRC/帧格式 |

---

## 9. 原子操作序列

### SEQ_OVR_CLEAR — 清除 OVR 标志
```
1. READ(DR)  → 读数据寄存器
2. READ(SR)  → 读状态寄存器
```
来源：RM0008 §25.3.10 p.693

### SEQ_MODF_CLEAR — 清除 MODF 标志
```
1. READ(SR)          → 读状态寄存器（MODF=1 时）
2. WRITE(CR1, val)   → 向 CR1 写当前值（无需改变内容）
```
来源：RM0008 §25.3.10 p.693

### SEQ_DISABLE_FULLDUPLEX — 全双工安全禁用
```
1. WAIT(SR.RXNE == 1) → 确认最后数据已接收
2. READ(DR)           → 清空 RX 缓冲
3. WAIT(SR.TXE == 1)  → 确认 TX 缓冲为空
4. WAIT(SR.BSY == 0)  → 确认传输完成
5. WRITE(CR1.SPE = 0) → 禁用 SPI
```
来源：RM0008 §25.3.8 p.692

---

## 10. 错误处理

| 错误 | 标志 | 触发条件 | 恢复方法 |
|------|------|---------|---------|
| Overrun    | SR.OVR    | 前次数据未读，新数据到达 | SEQ_OVR_CLEAR |
| Mode Fault | SR.MODF   | 主机模式下 NSS 变低（多主竞争）| SEQ_MODF_CLEAR；注意 MSTR/SPE 被硬件自动清除 |
| CRC Error  | SR.CRCERR | 接收 CRC 与计算值不符 | 写 SR.CRCERR=0（W0C）|
| Underrun   | SR.UDR    | I2S 从机发送欠载（SPI 模式无关）| 读 SR |

---

## 11. 操作模式

| 模式 | CR1 配置 |
|------|---------|
| 全双工主机 | MSTR=1, BIDIMODE=0, RXONLY=0 |
| 全双工从机 | MSTR=0, BIDIMODE=0, RXONLY=0 |
| 半双工主机-发 | MSTR=1, BIDIMODE=1, BIDIOE=1 |
| 半双工主机-收 | MSTR=1, BIDIMODE=1, BIDIOE=0 |
| 单工主机只收 | MSTR=1, BIDIMODE=0, RXONLY=1 |

---

## 12. 时序参数

| 参数 | 值 | 说明 |
|------|-----|------|
| SPI1 最大波特率 | 36 MHz | PCLK2 72MHz ÷ 2（BR=000）|
| SPI2/3 最大波特率 | 18 MHz | PCLK1 36MHz ÷ 2（BR=000）|
| TXE 超时（推荐）| 1000 ms | 轮询超时保护 |
| RXNE 超时（推荐）| 1000 ms | 轮询超时保护 |

---

## 13. 置信度说明

| 项目 | 置信度 | 说明 |
|------|--------|------|
| 寄存器映射 | 0.97 | 直接来自寄存器表 §25.5 |
| access type | 0.90 | OVR/MODF 为 RC_SEQ；CRCERR 标记为 rc_w0 → W0C |
| 中断向量号 | 0.93 | 来自 Table 63 p.194-196 |
| DMA 通道 | 0.88 | 来自 §13 DMA 请求表，需硬件验证 |

> **注意**：I2SCFGR 和 I2SPR 寄存器存在于 SPI/I2S 共享寄存器块中，SPI 模式下须保持 I2SCFGR.I2SMOD=0。
