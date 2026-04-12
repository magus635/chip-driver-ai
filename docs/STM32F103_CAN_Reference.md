# STM32F103 bxCAN 外设参考手册摘要
# 来源：RM0008 Reference Manual (STM32F101xx, STM32F102xx, STM32F103xx, STM32F105xx, STM32F107xx)
# 章节：Chapter 24 - Controller area network (bxCAN)

---

## 1. 外设概述

| 项目 | 值 |
|---|---|
| 外设名称 | bxCAN (Basic Extended CAN) |
| CAN1 基地址 | 0x40006400 |
| 总线 | APB1 |
| APB1 时钟 | 36 MHz (SYSCLK=72MHz, APB1 prescaler=2) |
| 支持协议 | CAN 2.0A / CAN 2.0B |
| 发送邮箱 | 3 个 |
| 接收 FIFO | 2 个（各 3 级深度） |
| 过滤器组 | 14 个（组 0-13） |

---

## 2. RCC 时钟配置

RCC 基地址: 0x40021000

### 2.1 APB1 外设时钟使能寄存器 (RCC_APB1ENR)
- 地址：RCC 基地址 + 0x1C = 0x4002101C
- CAN1EN: bit 25 — CAN1 时钟使能

### 2.2 APB2 外设时钟使能寄存器 (RCC_APB2ENR)
- 地址：RCC 基地址 + 0x18 = 0x40021018
- IOPAEN: bit 2 — GPIOA 时钟使能
- IOPBEN: bit 3 — GPIOB 时钟使能
- IOPCEN: bit 4 — GPIOC 时钟使能
- IOPDEN: bit 5 — GPIOD 时钟使能
- AFIOEN: bit 0 — AFIO 时钟使能（使用引脚重映射时必须使能）

---

## 3. GPIO 引脚映射与配置

### 3.1 CAN1 引脚映射（通过 AFIO_MAPR 重映射）

| AFIO_MAPR CAN_REMAP[1:0] | CAN1_RX | CAN1_TX | 说明 |
|---|---|---|---|
| 00 (默认) | PA11 | PA12 | 无重映射 |
| 10 (部分重映射) | PB8 | PB9 | 重映射到 PB8/PB9 |
| 11 (完全重映射) | PD0 | PD1 | 重映射到 PD0/PD1 |

AFIO_MAPR 寄存器地址: 0x40010004
CAN_REMAP 位域: bits [14:13]

### 3.2 GPIO 配置要求（F103 CRL/CRH 寄存器模型）

CAN_TX 引脚 (输出):
- CNF: 复用推挽输出 (0b10)
- MODE: 50MHz 输出 (0b11)
- 对应 CRL/CRH 4位配置值: 0b1011 (0xB)

CAN_RX 引脚 (输入):
- CNF: 上拉/下拉输入 (0b10)
- MODE: 输入模式 (0b00)
- 对应 CRL/CRH 4位配置值: 0b1000 (0x8)
- 同时设置 ODR 对应位 = 1（选择上拉）

### 3.3 GPIO CRL/CRH 寄存器说明

每个 GPIO 引脚占 4 位配置空间:
- 引脚 0-7: GPIOx_CRL (偏移 0x00)
- 引脚 8-15: GPIOx_CRH (偏移 0x04)

每 4 位格式: CNF[1:0] : MODE[1:0]

---

## 4. CAN 寄存器详细定义

### 4.1 CAN_MCR — 主控制寄存器 (偏移 0x00)
复位值: 0x0001 0002

| 位 | 名称 | 读写 | 复位值 | 说明 |
|---|---|---|---|---|
| 31:17 | - | - | - | 保留 |
| 16 | DBF | rw | 1 | 调试冻结：1=调试时冻结CAN |
| 15 | RESET | rw | 0 | 软件复位：1=强制复位，硬件自动清零 |
| 14:8 | - | - | - | 保留 |
| 7 | TTCM | rw | 0 | 时间触发通信模式 |
| 6 | ABOM | rw | 0 | 自动离线管理：1=自动恢复 |
| 5 | AWUM | rw | 0 | 自动唤醒模式 |
| 4 | NART | rw | 0 | 禁止自动重传：1=不重传 |
| 3 | RFLM | rw | 0 | 接收FIFO锁定模式 |
| 2 | TXFP | rw | 0 | 发送FIFO优先级：0=ID优先，1=请求顺序 |
| 1 | SLEEP | rw | 1 | 睡眠模式请求 |
| 0 | INRQ | rw | 0 | 初始化请求：1=请求进入初始化模式 |

### 4.2 CAN_MSR — 主状态寄存器 (偏移 0x04)
复位值: 0x0000 0C02

| 位 | 名称 | 读写 | 复位值 | 说明 |
|---|---|---|---|---|
| 31:12 | - | - | - | 保留 |
| 11 | RX | r | 1 | CAN Rx 引脚当前电平 |
| 10 | SAMP | r | 1 | 上次采样点值 |
| 9 | RXM | r | 0 | 接收模式：1=CAN正在接收 |
| 8 | TXM | r | 0 | 发送模式：1=CAN正在发送 |
| 7:5 | - | - | - | 保留 |
| 4 | SLAKI | rc_w1 | 0 | 睡眠确认中断标志 |
| 3 | WKUI | rc_w1 | 0 | 唤醒中断标志 |
| 2 | ERRI | rc_w1 | 0 | 错误中断标志 |
| 1 | SLAK | r | 1 | 睡眠确认：1=处于睡眠模式 |
| 0 | INAK | r | 0 | 初始化确认：1=处于初始化模式 |

### 4.3 CAN_TSR — 发送状态寄存器 (偏移 0x08)
复位值: 0x1C00 0000

| 位 | 名称 | 读写 | 说明 |
|---|---|---|---|
| 31 | LOW2 | r | 邮箱2最低优先级 |
| 30 | LOW1 | r | 邮箱1最低优先级 |
| 29 | LOW0 | r | 邮箱0最低优先级 |
| 28 | TME2 | r | 发送邮箱2空：1=空闲 |
| 27 | TME1 | r | 发送邮箱1空：1=空闲 |
| 26 | TME0 | r | 发送邮箱0空：1=空闲 |
| 25:24 | CODE | r | 邮箱代码（下一个空闲邮箱编号） |
| 23 | ABRQ2 | rs | 中止邮箱2请求 |
| 22:20 | - | - | 保留 |
| 19 | TERR2 | r | 邮箱2发送错误 |
| 18 | ALST2 | r | 邮箱2仲裁丢失 |
| 17 | TXOK2 | r | 邮箱2发送成功 |
| 16 | RQCP2 | rc_w1 | 邮箱2请求完成 |
| 15 | ABRQ1 | rs | 中止邮箱1请求 |
| 14:12 | - | - | 保留 |
| 11 | TERR1 | r | 邮箱1发送错误 |
| 10 | ALST1 | r | 邮箱1仲裁丢失 |
| 9 | TXOK1 | r | 邮箱1发送成功 |
| 8 | RQCP1 | rc_w1 | 邮箱1请求完成 |
| 7 | ABRQ0 | rs | 中止邮箱0请求 |
| 6:4 | - | - | 保留 |
| 3 | TERR0 | r | 邮箱0发送错误 |
| 2 | ALST0 | r | 邮箱0仲裁丢失 |
| 1 | TXOK0 | r | 邮箱0发送成功 |
| 0 | RQCP0 | rc_w1 | 邮箱0请求完成 |

### 4.4 CAN_RF0R — 接收 FIFO 0 寄存器 (偏移 0x0C)
复位值: 0x0000 0000

| 位 | 名称 | 读写 | 说明 |
|---|---|---|---|
| 31:6 | - | - | 保留 |
| 5 | RFOM0 | rs | 释放FIFO 0输出邮箱：写1释放 |
| 4 | FOVR0 | rc_w1 | FIFO 0溢出 |
| 3 | FULL0 | rc_w1 | FIFO 0满 |
| 1:0 | FMP0 | r | FIFO 0中待读报文数 (0-3) |

### 4.5 CAN_RF1R — 接收 FIFO 1 寄存器 (偏移 0x10)
复位值: 0x0000 0000

| 位 | 名称 | 读写 | 说明 |
|---|---|---|---|
| 31:6 | - | - | 保留 |
| 5 | RFOM1 | rs | 释放FIFO 1输出邮箱 |
| 4 | FOVR1 | rc_w1 | FIFO 1溢出 |
| 3 | FULL1 | rc_w1 | FIFO 1满 |
| 1:0 | FMP1 | r | FIFO 1中待读报文数 (0-3) |

### 4.6 CAN_IER — 中断使能寄存器 (偏移 0x14)
复位值: 0x0000 0000

| 位 | 名称 | 说明 |
|---|---|---|
| 17 | SLKIE | 睡眠中断使能 |
| 16 | WKUIE | 唤醒中断使能 |
| 15 | ERRIE | 错误中断使能 |
| 11 | LECIE | 上次错误码中断使能 |
| 10 | BOFIE | 离线中断使能 |
| 9 | EPVIE | 错误被动中断使能 |
| 8 | EWGIE | 错误警告中断使能 |
| 6 | FOVIE1 | FIFO 1溢出中断使能 |
| 5 | FFIE1 | FIFO 1满中断使能 |
| 4 | FMPIE1 | FIFO 1消息挂起中断使能 |
| 3 | FOVIE0 | FIFO 0溢出中断使能 |
| 2 | FFIE0 | FIFO 0满中断使能 |
| 1 | FMPIE0 | FIFO 0消息挂起中断使能 |
| 0 | TMEIE | 发送邮箱空中断使能 |

### 4.7 CAN_ESR — 错误状态寄存器 (偏移 0x18)
复位值: 0x0000 0000

| 位 | 名称 | 读写 | 说明 |
|---|---|---|---|
| 31:24 | REC | r | 接收错误计数器 |
| 23:16 | TEC | r | 发送错误计数器 |
| 15:7 | - | - | 保留 |
| 6:4 | LEC | rw | 上次错误代码：000=无错误，001=填充错误，010=格式错误，011=确认错误，100=隐性位错误，101=显性位错误，110=CRC错误，111=由软件设置 |
| 3 | - | - | 保留 |
| 2 | BOFF | r | 离线标志 |
| 1 | EPVF | r | 错误被动标志 |
| 0 | EWGF | r | 错误警告标志 |

### 4.8 CAN_BTR — 位时序寄存器 (偏移 0x1C)
复位值: 0x0123 0000
**注意：仅在初始化模式下可写**

| 位 | 名称 | 读写 | 说明 |
|---|---|---|---|
| 31 | SILM | rw | 静默模式：1=静默 |
| 30 | LBKM | rw | 回环模式：1=回环 |
| 29:26 | - | - | 保留 |
| 25:24 | SJW | rw | 重同步跳跃宽度：实际值 = SJW+1 (1-4 Tq) |
| 23 | - | - | 保留 |
| 22:20 | TS2 | rw | 时间段2：实际值 = TS2+1 (1-8 Tq) |
| 19:16 | TS1 | rw | 时间段1：实际值 = TS1+1 (1-16 Tq) |
| 15:10 | - | - | 保留 |
| 9:0 | BRP | rw | 波特率预分频：实际值 = BRP+1 (1-1024) |

---

## 5. 发送邮箱寄存器 (x = 0, 1, 2)

### 5.1 CAN_TIxR — 发送邮箱标识符寄存器
偏移: 0x180 + x*0x10

| 位 | 名称 | 说明 |
|---|---|---|
| 31:21 | STID | 标准标识符 (11位) |
| 20:3 | EXID | 扩展标识符 (18位，与STID共同组成29位) |
| 2 | IDE | 标识符扩展：0=标准帧，1=扩展帧 |
| 1 | RTR | 远程发送请求：0=数据帧，1=远程帧 |
| 0 | TXRQ | 发送请求：写1启动发送 |

### 5.2 CAN_TDTxR — 发送邮箱数据长度和时间戳寄存器
偏移: 0x184 + x*0x10

| 位 | 名称 | 说明 |
|---|---|---|
| 31:16 | TIME | 时间戳 |
| 8 | TGT | 发送全局时间：1=在DATA6/DATA7发送时间戳 |
| 3:0 | DLC | 数据长度码 (0-8) |

### 5.3 CAN_TDLxR — 发送邮箱低位数据寄存器
偏移: 0x188 + x*0x10

| 位 | 名称 | 说明 |
|---|---|---|
| 31:24 | DATA3 | 数据字节3 |
| 23:16 | DATA2 | 数据字节2 |
| 15:8 | DATA1 | 数据字节1 |
| 7:0 | DATA0 | 数据字节0 |

### 5.4 CAN_TDHxR — 发送邮箱高位数据寄存器
偏移: 0x18C + x*0x10

| 位 | 名称 | 说明 |
|---|---|---|
| 31:24 | DATA7 | 数据字节7 |
| 23:16 | DATA6 | 数据字节6 |
| 15:8 | DATA5 | 数据字节5 |
| 7:0 | DATA4 | 数据字节4 |

---

## 6. 接收 FIFO 邮箱寄存器 (x = 0, 1)

### 6.1 CAN_RIxR — 接收 FIFO 邮箱标识符寄存器
偏移: 0x1B0 + x*0x10

| 位 | 名称 | 说明 |
|---|---|---|
| 31:21 | STID | 标准标识符 |
| 20:3 | EXID | 扩展标识符 |
| 2 | IDE | 标识符扩展 |
| 1 | RTR | 远程发送请求 |
| 0 | - | 保留 |

### 6.2 CAN_RDTxR — 接收 FIFO 邮箱数据长度和时间戳寄存器
偏移: 0x1B4 + x*0x10

| 位 | 名称 | 说明 |
|---|---|---|
| 31:16 | TIME | 时间戳 |
| 15:8 | FMI | 过滤器匹配序号 |
| 3:0 | DLC | 数据长度码 |

### 6.3 CAN_RDLxR — 接收 FIFO 邮箱低位数据寄存器
偏移: 0x1B8 + x*0x10 (同 TDLxR 格式)

### 6.4 CAN_RDHxR — 接收 FIFO 邮箱高位数据寄存器
偏移: 0x1BC + x*0x10 (同 TDHxR 格式)

---

## 7. 过滤器寄存器

### 7.1 CAN_FMR — 过滤器主寄存器 (偏移 0x200)

| 位 | 名称 | 说明 |
|---|---|---|
| 0 | FINIT | 过滤器初始化模式：1=初始化模式 |

注意: F103 只有 CAN1，无 CAN2SB 字段的实际意义（无需分配过滤器给 CAN2）。

### 7.2 CAN_FM1R — 过滤器模式寄存器 (偏移 0x204)
- 位 x (x=0-13): FBMx — 0=掩码模式，1=列表模式

### 7.3 CAN_FS1R — 过滤器比例寄存器 (偏移 0x20C)
- 位 x (x=0-13): FSCx — 0=双16位，1=单32位

### 7.4 CAN_FFA1R — 过滤器 FIFO 分配寄存器 (偏移 0x214)
- 位 x (x=0-13): FFAx — 0=分配到FIFO 0，1=分配到FIFO 1

### 7.5 CAN_FA1R — 过滤器激活寄存器 (偏移 0x21C)
- 位 x (x=0-13): FACTx — 0=未激活，1=激活

### 7.6 CAN_FiRx — 过滤器组 i 寄存器 x (偏移 0x240 + i*0x08 + x*0x04)
- i = 0-13 (过滤器组号)
- x = 0-1 (每组两个寄存器)

---

## 8. 波特率计算

### 8.1 公式
```
BaudRate = APB1_CLK / ((BRP + 1) * (1 + (TS1 + 1) + (TS2 + 1)))
         = APB1_CLK / ((BRP + 1) * Tq_count)

其中：
  Tq_count = 1 + (TS1+1) + (TS2+1) = SYNC_SEG + PROP_SEG+PHASE1 + PHASE2
  采样点 = (1 + (TS1+1)) / Tq_count * 100%
```

### 8.2 约束条件
- BRP: 0-1023 (实际分频 1-1024)
- TS1: 0-15 (实际 1-16 Tq)
- TS2: 0-7 (实际 1-8 Tq)
- SJW: 0-3 (实际 1-4 Tq)
- SJW ≤ TS2
- 推荐采样点: 75% ~ 87.5%

### 8.3 常用波特率配置表 (APB1 = 36 MHz, Tq_count = 12)

| 波特率 | BRP | TS1 | TS2 | Tq数 | 采样点 | 实际波特率 |
|---|---|---|---|---|---|---|
| 1000 kbps | 2 | 8 | 1 | 12 | 83.3% | 1000.0 kbps |
| 500 kbps | 5 | 8 | 1 | 12 | 83.3% | 500.0 kbps |
| 250 kbps | 11 | 8 | 1 | 12 | 83.3% | 250.0 kbps |
| 125 kbps | 23 | 8 | 1 | 12 | 83.3% | 125.0 kbps |
| 100 kbps | 29 | 8 | 1 | 12 | 83.3% | 100.0 kbps |
| 50 kbps | 59 | 8 | 1 | 12 | 83.3% | 50.0 kbps |

### 8.4 验算示例 (500 kbps)
```
BaudRate = 36,000,000 / ((5+1) * (1 + (8+1) + (1+1)))
         = 36,000,000 / (6 * 12)
         = 36,000,000 / 72
         = 500,000 = 500 kbps ✓

采样点 = (1 + 9) / 12 = 83.3% ✓
```

---

## 9. 初始化序列 (RM0008 §24.4.1)

```
步骤 1: 使能时钟
  RCC->APB2ENR |= RCC_APB2ENR_IOPxEN;    // GPIO 端口时钟（如 IOPAEN for PA）
  RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;    // AFIO 时钟（引脚重映射需要）
  RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;    // CAN1 时钟

步骤 2: 配置 GPIO 引脚（以默认 PA11/PA12 为例）
  // PA12 (CAN_TX): 复用推挽输出, 50MHz
  GPIOA->CRH &= ~(0xF << 16);            // 清除 PA12 配置位 (pin12 在 CRH, 偏移 (12-8)*4=16)
  GPIOA->CRH |=  (0xB << 16);            // CNF=10(复用推挽), MODE=11(50MHz)

  // PA11 (CAN_RX): 上拉输入
  GPIOA->CRH &= ~(0xF << 12);            // 清除 PA11 配置位 (pin11 在 CRH, 偏移 (11-8)*4=12)
  GPIOA->CRH |=  (0x8 << 12);            // CNF=10(上拉/下拉输入), MODE=00(输入)
  GPIOA->ODR |=  (1 << 11);              // 选择上拉

  // 可选: 重映射到 PB8/PB9 (部分重映射)
  // AFIO->MAPR &= ~(0x3 << 13);
  // AFIO->MAPR |=  (0x2 << 13);         // CAN_REMAP = 10

步骤 3: 请求进入初始化模式
  CAN1->MCR |= CAN_MCR_INRQ;

步骤 4: 等待进入初始化模式确认
  while (!(CAN1->MSR & CAN_MSR_INAK)) { /* 带超时 */ }

步骤 5: 退出睡眠模式
  CAN1->MCR &= ~CAN_MCR_SLEEP;

步骤 6: 配置 MCR（可选功能）
  CAN1->MCR |= CAN_MCR_ABOM;   // 自动离线恢复
  CAN1->MCR |= CAN_MCR_NART;   // 禁止自动重传（可选）

步骤 7: 配置波特率和模式 (BTR)
  CAN1->BTR = (SJW << 24) | (TS2 << 20) | (TS1 << 16) | BRP;
  // 回环模式(调试): CAN1->BTR |= CAN_BTR_LBKM;

步骤 8: 退出初始化模式
  CAN1->MCR &= ~CAN_MCR_INRQ;

步骤 9: 等待退出初始化模式确认
  while (CAN1->MSR & CAN_MSR_INAK) { /* 带超时 */ }

步骤 10: 配置过滤器（至少一个，否则无法接收）
  CAN1->FMR |= CAN_FMR_FINIT;        // 进入过滤器初始化
  CAN1->FA1R |= (1 << 0);             // 激活过滤器0
  CAN1->sFilterRegister[0].FR1 = 0;   // 接受所有ID
  CAN1->sFilterRegister[0].FR2 = 0;
  CAN1->FMR &= ~CAN_FMR_FINIT;       // 退出过滤器初始化
```

---

## 10. 发送流程 (RM0008 §24.4.2)

```
1. 查找空闲邮箱
   if (CAN1->TSR & CAN_TSR_TME0)      mailbox = 0;
   else if (CAN1->TSR & CAN_TSR_TME1) mailbox = 1;
   else if (CAN1->TSR & CAN_TSR_TME2) mailbox = 2;
   else return -1;  // 全忙

2. 设置帧 ID
   标准帧: CAN1->sTxMailBox[mb].TIR = (id << 21);
   扩展帧: CAN1->sTxMailBox[mb].TIR = (id << 3) | CAN_TI0R_IDE;

3. 设置 RTR 和 DLC
   CAN1->sTxMailBox[mb].TIR |= (rtr << 1);
   CAN1->sTxMailBox[mb].TDTR = dlc & 0x0F;

4. 填写数据
   CAN1->sTxMailBox[mb].TDLR = data[0]|(data[1]<<8)|(data[2]<<16)|(data[3]<<24);
   CAN1->sTxMailBox[mb].TDHR = data[4]|(data[5]<<8)|(data[6]<<16)|(data[7]<<24);

5. 请求发送
   CAN1->sTxMailBox[mb].TIR |= CAN_TI0R_TXRQ;
```

---

## 11. 接收流程 (RM0008 §24.4.3)

```
1. 检查 FIFO 中是否有报文
   if ((CAN1->RF0R & CAN_RF0R_FMP0) == 0) return -1;  // FIFO 空

2. 读取帧 ID
   ide = (CAN1->sFIFOMailBox[fifo].RIR >> 2) & 1;
   if (ide == 0)  // 标准帧
       id = CAN1->sFIFOMailBox[fifo].RIR >> 21;
   else           // 扩展帧
       id = CAN1->sFIFOMailBox[fifo].RIR >> 3;

3. 读取 RTR 和 DLC
   rtr = (CAN1->sFIFOMailBox[fifo].RIR >> 1) & 1;
   dlc = CAN1->sFIFOMailBox[fifo].RDTR & 0x0F;

4. 读取数据
   data[0..3] = CAN1->sFIFOMailBox[fifo].RDLR (逐字节提取);
   data[4..7] = CAN1->sFIFOMailBox[fifo].RDHR (逐字节提取);

5. 释放 FIFO
   CAN1->RF0R |= CAN_RF0R_RFOM0;  // FIFO 0
   // 或 CAN1->RF1R |= CAN_RF1R_RFOM1;  // FIFO 1
```
