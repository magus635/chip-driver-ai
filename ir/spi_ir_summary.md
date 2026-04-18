# SPI 外设 IR 摘要 — STM32F103C8T6

**来源**: RM0008 Rev 14, Doc ID 13902  
**目标芯片**: STM32F103C8T6（Blue Pill，medium-density）  
**生成日期**: 2026-04-13  
**生成者**: doc-analyst agent  

---

## 1. 外设概述

| 属性 | SPI1 | SPI2 |
|------|------|------|
| 基地址 | 0x40013000 | 0x40003800 |
| 时钟总线 | APB2 | APB1 |
| 最高时钟 | PCLK2/2 = 最高 36 MHz（PCLK2=72MHz） | PCLK1/2 = 最高 18 MHz（PCLK1=36MHz） |
| 中断号（IRQn） | 35（Position）/ IRQ 42 | 36（Position）/ IRQ 43 |
| RCC 时钟使能 | RCC_APB2ENR.SPI1EN（bit 12） | RCC_APB1ENR.SPI2EN（bit 14） |
| RCC 复位 | RCC_APB2RSTR.SPI1RST（bit 12） | RCC_APB1RSTR.SPI2RST（bit 14） |

> **注意**：STM32F103C8T6 仅有 SPI1 和 SPI2，**没有 SPI3**。SPI3 仅存在于高密度/XL密度/连接线设备。  
> 来源：RM0008 §25.1, §2.3（peripheral availability）

---

## 2. 寄存器映射

所有 SPI 寄存器均为 **16-bit** 有效位宽，对齐到 32-bit 边界（高 16 位保留）。  
来源：RM0008 §25.5 p.714

| 偏移 | 寄存器名 | 复位值 | access | 说明 |
|------|---------|--------|--------|------|
| 0x00 | SPI_CR1 | 0x0000 | RW | 控制寄存器 1 |
| 0x04 | SPI_CR2 | 0x0000 | RW（mixed） | 控制寄存器 2 |
| 0x08 | SPI_SR | 0x0002 | mixed | 状态寄存器 |
| 0x0C | SPI_DR | 0x0000 | RW | 数据寄存器 |
| 0x10 | SPI_CRCPR | 0x0007 | RW | CRC 多项式寄存器 |
| 0x14 | SPI_RXCRCR | 0x0000 | RO | 接收 CRC 寄存器 |
| 0x18 | SPI_TXCRCR | 0x0000 | RO | 发送 CRC 寄存器 |
| 0x1C | SPI_I2SCFGR | 0x0000 | RW | I2S 配置寄存器（SPI 模式不使用） |
| 0x20 | SPI_I2SPR | 0x0002 | RW | I2S 预分频寄存器（SPI 模式不使用） |

---

## 3. 寄存器位域详解

### 3.1 SPI_CR1（偏移 0x00，复位 0x0000）

来源：RM0008 §25.5.1, p.714-715

| 位 | 名称 | access | 描述 |
|----|------|--------|------|
| 15 | BIDIMODE | RW | 双向数据模式使能：0=2线单向，1=1线双向 |
| 14 | BIDIOE | RW | 双向模式输出使能：0=接收，1=发送（主机用MOSI，从机用MISO） |
| 13 | CRCEN | RW | 硬件CRC使能。**必须在SPE=0时写入** |
| 12 | CRCNEXT | RW | CRC传输下一帧。DMA模式下保持清零 |
| 11 | DFF | RW | 数据帧格式：0=8bit，1=16bit。**必须在SPE=0时配置** |
| 10 | RXONLY | RW | 仅接收模式（配合BIDIMODE=0） |
| 9 | SSM | RW | 软件从机管理：0=硬件NSS，1=软件NSS（NSS由SSI决定） |
| 8 | SSI | RW | 内部从机选择（仅SSM=1时有效，值强制到NSS引脚） |
| 7 | LSBFIRST | RW | 帧格式：0=MSB先，1=LSB先。**通信进行中不得修改** |
| 6 | SPE | RW | SPI使能：0=外设禁用，1=外设使能。**禁用前须等待BSY=0** |
| 5:3 | BR[2:0] | RW | 波特率控制（fPCLK分频）：000=÷2, 001=÷4, 010=÷8, 011=÷16, 100=÷32, 101=÷64, 110=÷128, 111=÷256。**通信进行中不得修改（LOCK字段）** |
| 2 | MSTR | RW | 主从选择：0=从机，1=主机。**通信进行中不得修改** |
| 1 | CPOL | RW | 时钟极性：0=空闲低，1=空闲高。**通信进行中不得修改（LOCK字段）** |
| 0 | CPHA | RW | 时钟相位：0=第1边沿采样，1=第2边沿采样。**通信进行中不得修改（LOCK字段）** |

**LOCK字段**（SPE=1 时不可修改）：BR[2:0]、CPOL、CPHA、MSTR、SSM、DFF

### 3.2 SPI_CR2（偏移 0x04，复位 0x0000）

来源：RM0008 §25.5.2, p.716

| 位 | 名称 | access | 描述 |
|----|------|--------|------|
| 15:8 | Reserved | - | 保留，须保持复位值 |
| 7 | TXEIE | RW | TXE中断使能：1=TXE置位时产生中断 |
| 6 | RXNEIE | RW | RXNE中断使能：1=RXNE置位时产生中断 |
| 5 | ERRIE | RW | 错误中断使能：1=CRCERR/OVR/MODF发生时产生中断 |
| 4:3 | Reserved | - | 保留 |
| 2 | SSOE | RW | SS输出使能（主机模式）：1=NSS输出使能，不可用于多主机 |
| 1 | TXDMAEN | RW | 发送DMA使能：TXE置位时产生DMA请求 |
| 0 | RXDMAEN | RW | 接收DMA使能：RXNE置位时产生DMA请求 |

### 3.3 SPI_SR（偏移 0x08，复位 0x0002）

来源：RM0008 §25.5.3, p.717

| 位 | 名称 | access | 描述 | 清除方式 |
|----|------|--------|------|---------|
| 15:8 | Reserved | - | 保留 | - |
| 7 | BSY | RO | 忙标志：1=SPI正在通信或TX缓冲区非空。硬件设置和清除 | 只读，不可写入 |
| 6 | OVR | RC_SEQ | 溢出标志：1=发生溢出（主机已发送但从机未读取RXNE） | 先读DR再读SR（SEQ_OVR_CLEAR） |
| 5 | MODF | RC_SEQ | 模式故障：1=主机检测到NSS=0（硬件NSS）或SSI=0（软件NSS） | 先读/写SR再写CR1（SEQ_MODF_CLEAR） |
| 4 | CRCERR | W0C | CRC错误：1=接收CRC与RXCRCR不匹配。软件写0清除 | 写0清除（`SPI_SR &= ~CRCERR_Msk`） |
| 3 | UDR | RC_SEQ | 欠载标志（仅I2S模式）：不适用于SPI | I2S专用 |
| 2 | CHSIDE | RO | 通道侧（仅I2S模式）：不适用于SPI | 只读 |
| 1 | TXE | RO | 发送缓冲区空：1=TX缓冲区为空，可写入下一数据 | 写SPI_DR自动清除 |
| 0 | RXNE | RO | 接收缓冲区非空：1=RX缓冲区有新数据 | 读SPI_DR自动清除 |

**SR 的 mixed 注意事项**：
- CRCERR = W0C（写0清除），需特殊处理
- OVR、MODF = RC_SEQ（序列清除），需执行 atomic_sequence
- BSY、TXE、RXNE、CHSIDE、UDR = RO
- 禁止对 SR 整体使用 `|=` 或 `&=`，必须按位域粒度操作

### 3.4 SPI_DR（偏移 0x0C，复位 0x0000）

来源：RM0008 §25.5.4, p.718

| 位 | 名称 | access | 描述 |
|----|------|--------|------|
| 15:0 | DR[15:0] | RW | 数据寄存器。写入→TX缓冲区；读取→RX缓冲区。8bit帧仅用DR[7:0]，MSB强制为0 |

### 3.5 SPI_CRCPR（偏移 0x10，复位 0x0007）

来源：RM0008 §25.5.5, p.718

| 位 | 名称 | access | 描述 |
|----|------|--------|------|
| 15:0 | CRCPOLY[15:0] | RW | CRC多项式。默认值0x0007。I2S模式不使用 |

### 3.6 SPI_RXCRCR（偏移 0x14，复位 0x0000）

来源：RM0008 §25.5.6, p.719

| 位 | 名称 | access | 描述 |
|----|------|--------|------|
| 15:0 | RXCRC[15:0] | RO | 接收CRC值。8bit帧仅低8位有效。BSY=1时读值可能不正确 |

### 3.7 SPI_TXCRCR（偏移 0x18，复位 0x0000）

来源：RM0008 §25.5.7, p.719

| 位 | 名称 | access | 描述 |
|----|------|--------|------|
| 15:0 | TXCRC[15:0] | RO | 发送CRC值。8bit帧仅低8位有效。BSY=1时读值可能不正确 |

### 3.8 SPI_I2SCFGR（偏移 0x1C，复位 0x0000）— SPI模式不使用

来源：RM0008 §25.5.8, p.720

> STM32F103C8T6（medium-density）不支持 I2S 模式。这些寄存器须保持默认值（I2SMOD=0）。

### 3.9 SPI_I2SPR（偏移 0x20，复位 0x0002）— SPI模式不使用

来源：RM0008 §25.5.9, p.721

---

## 4. 初始化序列

### 4.1 主机模式初始化（含 SPE=1 前置 guard）

来源：RM0008 §25.3.3, p.680

| 步骤 | 操作 | 寄存器/字段 | 层 | 备注 |
|------|------|------------|-----|------|
| 1 | 使能 SPI 时钟 | RCC_APB2ENR.SPI1EN（SPI1）或 RCC_APB1ENR.SPI2EN（SPI2） | drv | 必须先于任何外设访问 |
| 2 | 确保 SPE=0（LOCK guard） | SPI_CR1.SPE=0 | ll | 保证后续配置字段可写 |
| 3 | 配置 BR[2:0]（波特率） | SPI_CR1.BR | ll | SPE=0 前置条件满足 |
| 4 | 配置 CPOL、CPHA | SPI_CR1.CPOL, CPHA | ll | SPE=0 前置条件满足 |
| 5 | 配置 DFF（数据帧格式） | SPI_CR1.DFF | ll | SPE=0 前置条件满足 |
| 6 | 配置 LSBFIRST | SPI_CR1.LSBFIRST | ll | 通信前配置 |
| 7 | 配置 SSM/SSI 或 SSOE | SPI_CR1.SSM, SSI / SPI_CR2.SSOE | ll | SSM=1+SSI=1 用于软件NSS主机 |
| 8 | 设置 MSTR=1 | SPI_CR1.MSTR | ll | **INV_SPI_001**: SSM=1时须SSI=1 |
| 9 | 配置中断/DMA（可选） | SPI_CR2.TXEIE/RXNEIE/ERRIE/TXDMAEN/RXDMAEN | ll | 按需配置 |
| 10 | 设置 SPE=1 | SPI_CR1.SPE | ll | 使能外设，BR/CPOL/CPHA 从此锁定 |

### 4.2 从机模式初始化

来源：RM0008 §25.3.2, p.678

| 步骤 | 操作 | 备注 |
|------|------|------|
| 1 | 使能 SPI 时钟 | 同主机 |
| 2 | 确保 SPE=0 | LOCK guard |
| 3 | 配置 DFF、CPOL、CPHA、LSBFIRST | 必须与主机一致 |
| 4 | 清除 MSTR=0（从机） | 默认复位值已为0 |
| 5 | 配置 NSS：硬件模式清SSM，软件模式置SSM=1清SSI=0 | 从机软件模式SSI=0允许MODF检测 |
| 6 | 设置 SPE=1 | 从机须先于主机使能 |

### 4.3 去初始化序列（DeInit）

来源：RM0008 §25.3.8, p.691（全双工模式）

| 步骤 | 操作 |
|------|------|
| 1 | 等待 RXNE=1，读取最后一帧数据 |
| 2 | 等待 TXE=1 |
| 3 | 等待 BSY=0（**INV_SPI_003**） |
| 4 | 清除 SPE=0 |
| 5 | 禁用 SPI 时钟（RCC_APBxENR.SPIxEN=0） |

---

## 5. 关键时序约束（LOCK 字段）

来源：RM0008 §25.5.1 Notes, p.715

| 字段 | 约束条件 | 违反效果 |
|------|---------|---------|
| BR[2:0] | SPE=1 时不得修改 | 未定义行为（手册: "not allowed"） |
| CPOL | SPE=1 时不得修改 | 未定义行为 |
| CPHA | SPE=1 时不得修改 | 未定义行为 |
| MSTR | 通信进行中不得修改 | 未定义行为 |
| LSBFIRST | 通信进行中不得修改 | 未定义行为 |
| DFF | SPE=0 时配置（CRCEN同） | 未定义行为 |
| SSM/SSI | 主机模式：SSM=1时SSI必须=1 | MODF置位，MSTR被硬件清零，SPI退化为从机 |

---

## 6. 中断配置

来源：RM0008 §25.3.11 Table 182, p.694；§10.1.2 Table 63, p.196-197

| 中断事件 | 事件标志 | 使能控制位 | IRQn（medium-density） |
|---------|---------|----------|----------------------|
| 发送缓冲区空 | SR.TXE | CR2.TXEIE | SPI1=35, SPI2=36 |
| 接收缓冲区非空 | SR.RXNE | CR2.RXNEIE | SPI1=35, SPI2=36 |
| 模式故障 | SR.MODF | CR2.ERRIE | SPI1=35, SPI2=36 |
| 溢出错误 | SR.OVR | CR2.ERRIE | SPI1=35, SPI2=36 |
| CRC错误 | SR.CRCERR | CR2.ERRIE | SPI1=35, SPI2=36 |

> SPI1 和 SPI2 各有一个共享中断向量（所有事件共用一个IRQ）。

---

## 7. DMA 通道配置

来源：RM0008 §13.3.7 Table 78（DMA1 request mapping）

| 实例 | 方向 | DMA控制器 | 通道 |
|------|------|-----------|------|
| SPI1 | TX | DMA1 | Ch3 |
| SPI1 | RX | DMA1 | Ch2 |
| SPI2 | TX | DMA1 | Ch5 |
| SPI2 | RX | DMA1 | Ch4 |

---

## 8. GPIO 引脚配置

来源：RM0008 §9.1.11 Table 25（GPIO configurations for device peripherals）；§9.3.10（SPI1 alternate function remapping）

### SPI1（默认引脚）

| 信号 | 引脚 | GPIO 模式 | 备注 |
|------|------|----------|------|
| SCK | PA5 | 复用推挽输出（AF_PP） | 主机：输出；从机：输入（浮空或上拉） |
| MISO | PA6 | 输入浮空 / 复用推挽 | 主机：输入；从机：复用推挽输出 |
| MOSI | PA7 | 复用推挽输出 | 主机：输出；从机：输入浮空 |
| NSS | PA4 | 硬件：输入浮空 / 软件：GPIO | 软件管理时可作普通GPIO |

### SPI1（重映射引脚，AFIO_MAPR.SPI1_REMAP=1）

| 信号 | 引脚 |
|------|------|
| SCK | PB3 |
| MISO | PB4 |
| MOSI | PB5 |
| NSS | PA15 |

### SPI2（无重映射）

| 信号 | 引脚 | GPIO 模式 |
|------|------|----------|
| SCK | PB13 | 复用推挽输出（主机）/ 输入浮空（从机） |
| MISO | PB14 | 输入浮空（主机）/ 复用推挽（从机） |
| MOSI | PB15 | 复用推挽输出（主机）/ 输入浮空（从机） |
| NSS | PB12 | 硬件：输入浮空 / 软件：GPIO |

---

## 9. 错误状态及清除方式

来源：RM0008 §25.3.10, p.693-694

| 错误类型 | 标志 | access | 触发条件 | 清除方式 | 恢复操作 |
|---------|------|--------|---------|---------|---------|
| 模式故障（MODF） | SR.MODF | RC_SEQ | 主机模式下NSS引脚拉低（硬件NSS）或SSI=0（软件NSS） | 读/写SR（MODF置位时）再写CR1 | 恢复SPE=1和MSTR=1 |
| 溢出（OVR） | SR.OVR | RC_SEQ | 从机未及时读取DR，新数据覆盖旧数据 | 读DR后读SR | 检查RXNE处理速度 |
| CRC错误（CRCERR） | SR.CRCERR | W0C | 接收CRC与RXCRCR不匹配 | 写0到SR.CRCERR | 重置CRC：清CRCEN再置CRCEN |

---

## 10. 不变式（Invariants）

来源：RM0008 §25.3.1-25.3.3, §25.5.1

| ID | 表达式 | 说明 | 违反效果 |
|----|--------|------|---------|
| INV_SPI_001 | `CR1.MSTR==1 && CR1.SSM==1 implies CR1.SSI==1` | 主机+软件NSS时，SSI必须=1 | SR.MODF置位，CR1.MSTR被硬件清零，SPI退化为从机 |
| INV_SPI_002 | `CR1.SPE==1 implies !writable(CR1.BR) && !writable(CR1.CPOL) && !writable(CR1.CPHA) && !writable(CR1.DFF) && !writable(CR1.MSTR)` | SPE=1时配置字段不可修改 | 未定义行为 |
| INV_SPI_003 | `SR.BSY==1 implies !writable(CR1.SPE)` | 禁用SPI必须先等BSY=0且TXE=1 | 最后一帧数据可能损坏或丢失 |
| INV_SPI_004 | `SR.CRCERR==1 implies access_type(SR.CRCERR) == W0C` | CRCERR须写0清除，禁止写1操作 | 写1无效或写入错误值到其他位 |

---

## 11. 已知 Errata

STM32F103C8T6 的 errata（ES0306 / ES0182）暂未在本项目 `docs/` 目录中提供，无法自动提取。  
建议人工核查以下已知 STM32F1 SPI errata：

- **2.9.1**（如存在）：高频 SPI 通信中，NSS 脉冲与 SCK 时序约束
- **DFF 更改**：必须在 CRCEN=0 且 SPE=0 时进行，否则 CRC 寄存器不会正确复位

> **REVIEW_REQUEST**: Errata 文件不在项目 docs/ 中，confidence=0.70，需人工确认是否有已知 SPI errata 影响 STM32F103C8T6。

---

## 12. Atomic Sequences（原子操作序列）

### SEQ_OVR_CLEAR — 清除 OVR 标志

来源：RM0008 §25.3.10, p.694（"Clearing the OVR bit is done by a read from the SPI_DR register followed by a read access to the SPI_SR register"）

1. READ SPI_DR（丢弃数据，副作用：开始清除序列）
2. READ SPI_SR（完成清除序列）

### SEQ_MODF_CLEAR — 清除 MODF 标志

来源：RM0008 §25.3.10, p.693

1. READ 或 WRITE SPI_SR（MODF置位时）
2. WRITE SPI_CR1（写入任意值，通常是恢复 SPE=1 和 MSTR=1）
