# 外设中间表示规范 (Peripheral IR Specification)

**版本**: 4.0
**用途**: 定义 `doc-analyst` Agent 输出的结构化外设描述格式。**ir/<module>_ir_summary.json** 是唯一机器可消费真值源。
**消费者**: `code-gen` Agent（从 IR 生成驱动代码）、`reviewer-agent`（验证代码与 IR 的一致性）、`check-invariants.py`（静态检查）

---

## 1. 概述

中间表示（Intermediate Representation, IR）是**芯片手册**与**驱动代码**之间的桥梁。

```
芯片手册 (PDF/SVD/XML)
        │
        ▼
   ┌─────────────┐
   │ doc-analyst  │  ← 解析手册，提取结构化信息
   │  Agent       │
   └──────┬──────┘
          │ 输出
          ▼
   ┌─────────────┐
   │  IR JSON     │  ← 本规范定义的格式
   │  <module>    │
   │  _ir_summary.json    │
   └──────┬──────┘
          │ 输入
          ▼
   ┌─────────────┐
   │  code-gen   │  ← 从 IR 映射生成代码
   │  Agent       │
   └─────────────┘
```

**核心原则**：
1. **IR 是唯一真值源**：code-gen 禁止自行假设寄存器属性，必须从 IR 读取
2. **每个字段都有来源**：所有信息必须标注手册引用（章节、页码、表格编号）
3. **不确定即标注**：confidence < 0.85 的字段必须触发人工审核（R9 规则）
4. **access type 决定代码模式**：W1C → 直接赋值，RW → 可用 RMW，RO → 只读访问

---

## 2. 文件约定

- **存放位置**: `ir/<module>_ir_summary.json`（如 `ir/can_ir_summary.json`、`ir/spi_ir_summary.json`）
- **编码**: UTF-8
- **格式**: JSON（可通过 `jq .` 验证语法）
- **生命周期**: doc-analyst 生成 → 人工审核确认 → code-gen 消费 → 随手册版本更新

---

## 3. 顶层结构

```json
{
  "ir_version": "4.0",
  "peripheral": {
    "name": "CAN",
    "full_name": "Controller Area Network",
    "chip_family": "STM32F1",
    "chip_model": "STM32F103C8T6",
    "source_document": "RM0008 Rev 21",
    "source_sections": ["32.7 bxCAN registers", "32.4 Operating modes"],

    "instances": [ ... ],
    "instance_model": { ... },

    "registers": [ ... ],
    "interrupts": [ ... ],
    "clock": [ ... ],
    "dma_channels": [ ... ],
    "gpio_config": [ ... ],
    "atomic_sequences": [ ... ],
    "errors": [ ... ],
    "functional_model": { ... },
    "configuration_strategies": [ ... ],
    "cross_field_constraints": [ ... ],
    "errata": [ ... ],

    "signal_graph": [ ... ],
    "shared_resources": [ ... ],
    "structural_relations": [ ... ],

    "pending_reviews": [ ... ],
    "generation_metadata": { ... }
  }
}
```

---

## 4. 字段详细定义

### 4.1 instances — 外设实例

```json
"instances": [
  {
    "index": 0,
    "name": "CAN1",
    "base_address": "0x40006400",
    "source": "RM0090 §2.3 Table 1"
  },
  {
    "index": 1,
    "name": "CAN2",
    "base_address": "0x40006800",
    "source": "RM0090 §2.3 Table 1"
  }
]
```

### 4.2 registers — 寄存器定义（核心）

**v4.0 新增字段**：`register_set_id`（可选）。当 `instance_model` 使用 `topology: tree/dag` 时，每条寄存器记录**必须**填写此字段，以声明该寄存器属于哪个实例层级的寄存器布局。详见 §9.4。

```json
"registers": [
  {
    "name": "MCR",
    "full_name": "Master Control Register",
    "offset": "0x000",
    "width": 32,
    "access": "RW",
    "reset_value": "0x00010002",
    "description": "CAN master control register",
    "source": "RM0090 §32.9.1 p.1070",
    "confidence": 0.98,
    "endinit": "none",
    "safety_relevant": false,
    "register_set_id": "CAN_GLOBAL_REGS",
    "bitfields": [
      {
        "name": "INRQ",
        "bits": "0",
        "bit_offset": 0,
        "bit_width": 1,
        "access": "RW",
        "reset_value": 0,
        "description": "Initialization request",
        "source": "RM0090 §32.9.1 Table 186",
        "confidence": 0.99
      },
      {
        "name": "SLEEP",
        "bits": "1",
        "bit_offset": 1,
        "bit_width": 1,
        "access": "RW",
        "reset_value": 1,
        "description": "Sleep mode request",
        "source": "RM0090 §32.9.1 Table 186",
        "confidence": 0.99
      }
    ]
  },
  {
    "name": "RF0R",
    "full_name": "Receive FIFO 0 Register",
    "offset": "0x00C",
    "width": 32,
    "access": "mixed",
    "reset_value": "0x00000000",
    "description": "CAN receive FIFO 0 register",
    "source": "RM0090 §32.9.4 p.1074",
    "confidence": 0.98,
    "endinit": "none",
    "safety_relevant": false,
    "register_set_id": "CAN_GLOBAL_REGS",

    "bitfields": [
      {
        "name": "FMP0",
        "bits": "1:0",
        "bit_offset": 0,
        "bit_width": 2,
        "access": "RO",
        "reset_value": 0,
        "description": "FIFO 0 message pending (read-only count)",
        "source": "RM0090 §32.9.4",
        "confidence": 0.98
      },
      {
        "name": "FULL0",
        "bits": "3",
        "bit_offset": 3,
        "bit_width": 1,
        "access": "W1C",
        "reset_value": 0,
        "description": "FIFO 0 full. Set by hardware, cleared by writing 1.",
        "source": "RM0090 §32.9.4",
        "confidence": 0.98
      },
      {
        "name": "FOVR0",
        "bits": "4",
        "bit_offset": 4,
        "bit_width": 1,
        "access": "W1C",
        "reset_value": 0,
        "description": "FIFO 0 overrun. Set by hardware, cleared by writing 1.",
        "source": "RM0090 §32.9.4",
        "confidence": 0.98
      },
      {
        "name": "RFOM0",
        "bits": "5",
        "bit_offset": 5,
        "bit_width": 1,
        "access": "W1C",
        "reset_value": 0,
        "description": "Release FIFO 0 output mailbox. Set by software to release.",
        "source": "RM0090 §32.9.4",
        "confidence": 0.97
      }
    ]
  }
]
```

### 4.3 access type 枚举与代码生成规则

**核心原则**：access type 必须忠实反映芯片手册的标注（如手册标注 `rc_w0` -> IR 填 `W0C`，标注 `rc_w1` -> IR 填 `W1C`）。doc-analyst **不得**将不识别的类型强行归类为已有类型，应如实记录并标注 `confidence < 0.85` 触发人工审核。

| access type | 含义 | code-gen 操作规则 | 示例 |
|-------------|------|------------------|------|
| `RW` | 可读可写 | 允许 RMW（`\|=`, `&=`），允许直接赋值 | `CANx->MCR \|= INRQ_Msk;` |
| `RO` | 只读 | 只允许读取，禁止任何写操作 | `val = CANx->MSR;` |
| `WO` | 只写 | 只允许写入，读取值无意义 | `CANx->TDR = data;` |
| `W1C` | 写1清零 | **禁止 `\|=`**，必须直接赋值（`=`），避免意外清除其他标志 | `CANx->RF0R = RFOM0_Msk;` |
| `W0C` | 写0清零 | 清除时对目标位写 0。若同寄存器无其他特殊位可用 `&= ~Msk`；若含 W1C 位则需安全赋值 | `SPIx->SR &= ~CRCERR_Msk;` |
| `W1S` | 写1置位 | 直接赋值目标位 | `CANx->BSRR = pin_msk;` |
| `RC` | 读清零（单步） | 读取即清除，需注意读取顺序和副作用 | `tmp = I2Cx->SR1;` |
| `RC_SEQ` | 读清零（多步序列） | 需执行特定多步操作序列才能清除，序列定义在 `atomic_sequences[]` 中，bitfield 通过 `clear_sequence` 字段引用 | 见 4.8 节 |
| `WO_TRIGGER` | 只写触发位（写入后硬件自动清零） | 只生成 Trigger 函数，直接赋值，无需读回 | `CANx->TSR = ABRQ0_Msk;` |
| `mixed` | 寄存器内不同位域有不同 access type | 以各 bitfield 的 access 为准，**禁止对整个寄存器生成 RMW 函数**，必须按 bitfield 粒度独立生成 | 见 RF0R 示例 |

**`mixed` 寄存器安全规则**：
1. **禁止**对 `mixed` 寄存器生成整体的 Set/Clear/Toggle 函数
2. 只允许按 bitfield 粒度生成独立函数，每个 bitfield 根据自己的 access type 走对应规则
3. 若需要 RMW 操作修改其中某个 `RW` 位，写回值中所有 `W1C` 位必须置 0、所有 `W0C` 位必须置 1（安全值，不触发意外清除）

### 4.4 interrupts — 中断源

```json
"interrupts": [
  {
    "name": "CAN1_TX",
    "irq_number": 19,
    "description": "CAN1 TX interrupt",
    "trigger_registers": ["TSR"],
    "trigger_flags": ["RQCP0", "RQCP1", "RQCP2"],
    "clear_method": "W1C",
    "source": "RM0090 §32.8 Table 185"
  },
  {
    "name": "CAN1_RX0",
    "irq_number": 20,
    "description": "CAN1 RX FIFO 0 interrupt",
    "trigger_registers": ["RF0R"],
    "trigger_flags": ["FMP0"],
    "clear_method": "read_fifo",
    "source": "RM0090 §32.8"
  }
]
```

### 4.5 clock — 时钟配置（按实例索引）

多实例外设的不同实例可能挂载在不同总线上，因此 clock 统一为**数组**，每个元素对应一个实例：

```json
"clock": [
  {
    "instance": "CAN1",
    "bus": "APB1",
    "enable_register": "RCC_APB1ENR",
    "enable_bit": "CAN1EN",
    "reset_register": "RCC_APB1RSTR",
    "reset_bit": "CAN1RST",
    "max_frequency_mhz": 42,
    "source": "RM0090 §7.3.14",
    "confidence": 0.95
  }
]
```

**必填字段**：`instance`, `bus`, `enable_register`, `enable_bit`, `source`
**可选字段**：`reset_register`, `reset_bit`, `max_frequency_mhz`, `clock_source_mux`, `confidence`

### 4.5b dma_channels — DMA 通道映射

定义外设各实例使用的 DMA 通道及请求映射，供 code-gen 在 DMA 传输模式下生成正确的通道配置。

```json
"dma_channels": [
  {
    "instance": "SPI1",
    "direction": "TX",
    "dma_controller": "DMA1",
    "channel": 3,
    "request": "SPI1_TX",
    "priority": "medium",
    "source": "RM0008 §13.3.7 Table 78 p.282"
  },
  {
    "instance": "SPI1",
    "direction": "RX",
    "dma_controller": "DMA1",
    "channel": 2,
    "request": "SPI1_RX",
    "priority": "medium",
    "source": "RM0008 §13.3.7 Table 78 p.282"
  }
]
```

**字段说明**：

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `instance` | string | 是 | 外设实例名（如 `SPI1`） |
| `direction` | enum | 是 | `TX` / `RX` / `MEM2MEM` |
| `dma_controller` | string | 是 | DMA 控制器名（如 `DMA1`） |
| `channel` | int | 是 | 通道编号（STM32F1 为固定映射，无 request mux） |
| `request` | string | 否 | 请求源标识（STM32F1 由通道号隐含，F4/H7 需显式指定） |
| `priority` | enum | 否 | 默认优先级：`low` / `medium` / `high` / `very_high` |
| `source` | string | 是 | 手册引用 |
| `confidence` | float | 否 | 置信度 (0.0-1.0) |

**code-gen 消费规则**：为支持 DMA 的外设生成 `Xxx_TransferDMA()` 函数时，从此数组读取通道号和控制器，避免硬编码。

### 4.6 functional_model — 功能语义（AI 提取）

```json
"functional_model": {
  "init_sequence": [
    {
      "order": 1,
      "description": "Enable peripheral clock via RCC",
      "register": "RCC_APB1ENR",
      "field": "CAN1EN",
      "value": "1",
      "layer": "drv",
      "source": "RM0090 §32.4.1"
    },
    {
      "order": 2,
      "description": "Request initialization mode",
      "register": "MCR",
      "field": "INRQ",
      "value": "1",
      "layer": "ll",
      "source": "RM0090 §32.4.1"
    },
    {
      "order": 3,
      "description": "Wait for INAK acknowledgement",
      "register": "MSR",
      "field": "INAK",
      "condition": "== 1",
      "timeout_required": true,
      "layer": "drv",
      "source": "RM0090 §32.4.1"
    },
    {
      "order": 4,
      "description": "Configure bit timing (BRP, TS1, TS2, SJW)",
      "register": "BTR",
      "layer": "ll",
      "source": "RM0090 §32.7.7"
    },
    {
      "order": 5,
      "description": "Leave initialization mode",
      "register": "MCR",
      "field": "INRQ",
      "value": "0",
      "layer": "ll",
      "source": "RM0090 §32.4.1"
    }
  ],

  "deinit_sequence": [
    {
      "order": 1,
      "description": "Enter initialization mode to stop all bus activity",
      "register": "MCR",
      "field": "INRQ",
      "value": "1",
      "layer": "ll",
      "source": "RM0090 §32.4.1"
    },
    {
      "order": 2,
      "description": "Disable peripheral clock",
      "register": "RCC_APB1ENR",
      "field": "CAN1EN",
      "value": "0",
      "layer": "drv",
      "source": "RM0090 §7.3.14"
    }
  ],

  "operation_modes": [
    {
      "name": "normal",
      "description": "Standard CAN communication",
      "btr_config": {"LBKM": 0, "SILM": 0}
    },
    {
      "name": "loopback",
      "description": "Internal loopback for self-test",
      "btr_config": {"LBKM": 1, "SILM": 0}
    },
    {
      "name": "silent",
      "description": "Listen-only mode, no TX",
      "btr_config": {"LBKM": 0, "SILM": 1}
    }
  ],

  "error_handling": [
    "LEC field in ESR indicates last error code",
    "TEC/REC counters for fault confinement state",
    "ABOM bit enables automatic bus-off recovery"
  ],

  "invariants": [
    {
      "id": "INV_CAN_001",
      "expr": "MCR.INRQ==0 implies !writable(BTR)",
      "description": "BTR can only be modified when CAN is in initialization mode (INRQ=1 and INAK=1)",
      "scope": "always",
      "violation_effect": "Write to BTR is silently ignored",
      "enforced_by": ["code-gen:precondition", "reviewer-agent:static"],
      "source": "RM0090 §32.7.7",
      "confidence": 0.97
    }
  ],

  "confidence": 0.88,
  "source": "RM0090 §32.4"
}
```

#### 4.6.1 invariants — 硬件强制不变式（核心新增）

**定位**：硬件层面的"必须为真"的约束。区别于 `init_sequence`（操作步骤）和 `modification_constraints`（自然语言），invariants 是**机器可解析的布尔表达式**，供 reviewer-agent 静态校验代码、code-gen 自动插入前置检查。

**字段定义**：

| 字段 | 类型 | 说明 |
|---|---|---|
| `id` | string | 唯一标识，格式 `INV_<外设>_<序号>` |
| `expr` | string | 布尔表达式，见下方语法 |
| `description` | string | 人类可读说明 |
| `scope` | enum | `always` / `during_init` / `during_transfer` / `before_disable` |
| `violation_effect` | string | 违反时硬件实际行为（"写入被忽略"/"MODF 置位"/"未定义行为"…） |
| `enforced_by` | string[] | 谁来保证：`code-gen:precondition` / `reviewer-agent:static` / `runtime:assert` |
| `source` | string | 手册引用 |
| `confidence` | float | 0.0–1.0 |

**表达式语法**（最小可解析子集）：

```ebnf
expr         := term (logic_op term)*
term         := atom | "!" atom | "(" expr ")"
atom         := field_ref cmp_op value
              | "writable(" reg_or_field ")"
              | "readable(" reg_or_field ")"
field_ref    := REG_NAME "." FIELD_NAME
reg_or_field := REG_NAME | field_ref
cmp_op       := "==" | "!=" | ">" | "<" | ">=" | "<="
logic_op     := "&&" | "||" | "implies"
value        := INT | HEX | field_ref
```

> **注**：`writable(REG_NAME)` 是寄存器级简写，含义为"REG_NAME 中任意字段均可写入"。`writable(REG_NAME.FIELD_NAME)` 则精确到位域级别。reviewer-agent 实现时两种形式都须支持。

**示例**（SPI）：

```json
"invariants": [
  {
    "id": "INV_SPI_001",
    "expr": "CR1.MSTR==1 && CR1.SSM==1 implies CR1.SSI==1",
    "description": "主模式 + 软件 NSS 时 SSI 必须为 1，否则硬件检测到 NSS 拉低会触发 MODF 并清除 MSTR",
    "scope": "always",
    "violation_effect": "MSTR 被硬件清零，SR.MODF 置位，SPI 退化为从机",
    "enforced_by": ["code-gen:precondition", "reviewer-agent:static"],
    "source": "RM0008 §25.3.1 p.676, §25.5.1 p.715",
    "confidence": 0.98
  },
  {
    "id": "INV_SPI_002",
    "expr": "CR1.SPE==1 implies !writable(CR1.BR) && !writable(CR1.CPOL) && !writable(CR1.CPHA)",
    "description": "BR/CPOL/CPHA 必须在 SPE=0 时配置，启用后改这些位行为未定义",
    "scope": "always",
    "violation_effect": "未定义行为（手册：not allowed）",
    "enforced_by": ["code-gen:precondition", "reviewer-agent:static"],
    "source": "RM0008 §25.5.1 p.714-715 Notes",
    "confidence": 0.99
  },
  {
    "id": "INV_SPI_003",
    "expr": "SR.BSY==1 implies !writable(CR1.SPE)",
    "description": "禁用 SPI 必须先等 BSY=0 且 TXE=1，否则可能丢失正在传输的帧",
    "scope": "before_disable",
    "violation_effect": "最后一帧数据可能损坏或丢失",
    "enforced_by": ["code-gen:precondition"],
    "source": "RM0008 §25.3.8 p.691",
    "confidence": 0.99
  }
]
```

**消费规则**：
- code-gen：每次生成对 `field_ref` 的写操作前，若该字段出现在某 invariant 的 `expr` 左侧，必须插入前置检查代码或确保上下文已满足。
- reviewer-agent：解析所有 invariants，对生成的 `*_drv.c` / `*_ll.c` 做数据流分析，检测违反 `enforced_by:reviewer-agent:static` 的写入。
- 失败时进入 `pending_reviews`，code-gen 拒绝放行。

### 4.7 errata — 勘误表影响

```json
"errata": [
  {
    "id": "ES0306_2.3.7",
    "title": "Limitation on I2C N=2 byte reception",
    "affected_registers": ["CR1", "SR1"],
    "workaround": "Must set POS bit before clearing ADDR for 2-byte reception",
    "impact_on_code": "I2C_MasterReceive must special-case Size==2",
    "source": "ES0306 Rev 7, §2.3.7",
    "confidence": 0.95
  }
]
```

### 4.8 atomic_sequences — 原子操作序列

当标志位的清除方式不是简单的读或写（如 SPI OVR 需要"先读 DR 再读 SR"），必须在 `atomic_sequences` 中定义精确的操作步骤，并在相应 bitfield 中通过 `clear_sequence` 引用：

```json
"atomic_sequences": [
  {
    "id": "SEQ_OVR_CLEAR",
    "name": "Overrun Clear Sequence",
    "source": "RM0090 §28.3.10",
    "steps": [
      {"order": 1, "operation": "READ", "target": "DR", "comment": "Read DR to discard data"},
      {"order": 2, "operation": "READ", "target": "SR", "comment": "Read SR to complete OVR clear"}
    ],
    "constraints": ["Order must not be swapped", "No other operations between steps"],
    "barrier_required": true
  }
]
```

**bitfield 引用方式**：
```json
{"name": "OVR", "access": "RC_SEQ", "clear_sequence": "SEQ_OVR_CLEAR", ...}
```

**code-gen 消费规则**：遇到 `clear_sequence` 引用时，在 `*_ll.h` 中生成对应的封装函数（如 `SPI_LL_ClearOVR()`），函数体严格按 `steps[]` 顺序生成读写操作。若 `barrier_required == true`，在序列末尾插入 `__DSB()`。

**支持的 operation 类型**：
| operation | 含义 |
|-----------|------|
| `READ` | 读取寄存器 |
| `WRITE` | 写入寄存器（`value` 字段指定值，`"current"` 表示写回当前值） |
| `WAIT_FLAG` | 等待标志位（需 `timeout_required: true`） |
| `CLEAR_BIT` | 清除指定位 |
| `SET_BIT` | 置位指定位 |
| `CALL` | 调用另一个 atomic_sequence（`target` 为被调用序列 ID） |

### 4.9 gpio_config — GPIO 引脚配置

定义外设各实例所需的 GPIO 引脚复用配置，供 code-gen 在初始化序列中生成跨模块调用：

```json
"gpio_config": [
  {
    "instance": "SPI1",
    "pins": [
      {"function": "SCK",  "default_pin": "PA5", "alt_pin": "PB3", "af": 5, "mode": "AF_PP", "pull": "NONE", "speed": "HIGH"},
      {"function": "MISO", "default_pin": "PA6", "alt_pin": "PB4", "af": 5, "mode": "INPUT",  "pull": "PULL_UP"},
      {"function": "MOSI", "default_pin": "PA7", "alt_pin": "PB5", "af": 5, "mode": "AF_PP", "pull": "NONE", "speed": "HIGH"}
    ],
    "source": "DS9716 Table 9"
  }
]
```

### 4.10 errors — 错误类型与恢复策略

```json
"errors": [
  {
    "type": "Overrun",
    "flag": "SR.OVR",
    "flag_access": "RC_SEQ",
    "effect": "New data overwrites unread data, subsequent reception blocked",
    "recovery_sequence": "SEQ_OVR_CLEAR",
    "prevention": "Ensure DR is read in time"
  }
]
```

**code-gen 消费规则**：为每个 error 在 `*_drv.c` 中生成处理分支，`recovery_sequence` 引用 `atomic_sequences` 中的 LL 封装函数。

### 4.11 pending_reviews — 待人工审核项

当 doc-analyst 对某个字段的 confidence < 0.85，或遇到手册矛盾时，必须填写此数组：

```json
"pending_reviews": [
  {
    "review_id": "rev_001",
    "category": "ambiguous_register",
    "question": "RF0R.RFOM0 的 access type 手册文字描述为 'rs' (read/set)，但表格标注为 'rc_w1'，应按 W1C 还是 RS 处理？",
    "context": "RM0090 §32.9.4 Table 188 vs 正文第3段",
    "options": ["W1C (write-1-to-clear)", "RS (read/set, hardware auto-clear)"],
    "ai_recommendation": "W1C",
    "confidence": 0.72,
    "resolved": false,
    "human_decision": null
  }
]
```

**规则**：存在未解决的 `pending_reviews` 时，code-gen 禁止生成涉及该寄存器/位域的代码，直到人工审核完成。

### 4.12 configuration_strategies — 配置策略

定义外设的不同**初始化和运行策略**（如单主机、多主机、特定应用场景），供 code-gen 生成对应的配置函数。

```json
"configuration_strategies": [
  {
    "id": "single_master_mode",
    "name": "Single-Master Mode",
    "description": "标准的单主机通信模式。主机控制 NSS，从机被动响应。",
    "applies_to": ["master"],
    "register_config": {
      "CR1": {"MSTR": 1, "SSM": 1, "SSI": 1},
      "CR2": {"SSOE": 0}
    },
    "prerequisites": [
      "NSS pin configured as GPIO output",
      "Master controls NSS signal timing"
    ],
    "constraints": [
      "SSI must be 1 (INV_SPI_001)",
      "SSOE must be 0 in single-master mode"
    ],
    "source": "RM0008 §25.3.3 p.680",
    "confidence": 0.99
  },
  {
    "id": "multimaster_mode",
    "name": "Multimaster Mode (Arbitration)",
    "description": "多主机竞争模式。每个节点通过 NSS 脚信号和 MODF 监听实现仲裁。",
    "applies_to": ["master"],
    "register_config": {
      "CR1": {"MSTR": 1, "SSM": 0},
      "CR2": {"SSOE": 0, "ERRIE": 1}
    },
    "prerequisites": [
      "NSS configured as input (floating or pulled-up)",
      "MODF interrupt enabled for collision detection",
      "Application implements arbitration protocol"
    ],
    "constraints": [
      "Cannot enable SSOE (CR2.SSOE=0)",
      "Each master must implement arbitration protocol",
      "MODF flag indicates collision; master must release bus and retry"
    ],
    "source": "RM0008 §25.3.1 p.676",
    "confidence": 0.98
  },
  {
    "id": "hardware_nss_slave_mode",
    "name": "Hardware NSS Slave Mode",
    "description": "从机使用硬件 NSS 管理。NSS 拉低时使能，拉高时禁用接收。",
    "applies_to": ["slave"],
    "register_config": {
      "CR1": {"MSTR": 0, "SSM": 0},
      "CR2": {"SSOE": 0}
    },
    "prerequisites": [
      "NSS pin configured as input (floating)",
      "Master controls NSS signal"
    ],
    "constraints": [
      "Slave becomes active only when NSS is pulled low"
    ],
    "source": "RM0008 §25.3.2 p.678",
    "confidence": 0.98
  },
  {
    "id": "software_nss_slave_mode",
    "name": "Software NSS Slave Mode",
    "description": "从机使用软件 NSS 管理。SSI 位决定是否响应。",
    "applies_to": ["slave"],
    "register_config": {
      "CR1": {"MSTR": 0, "SSM": 1, "SSI": 0},
      "CR2": {"SSOE": 0}
    },
    "prerequisites": [
      "NSS pin can be used for other purposes"
    ],
    "constraints": [
      "SSI=0 allows MODF detection for collision handling"
    ],
    "source": "RM0008 §25.3.2 p.678",
    "confidence": 0.98
  }
]
```

**字段说明**：

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | string | 唯一标识，格式 `<mode>_<variant>` |
| `name` | string | 可读名称 |
| `description` | string | 详细功能说明 |
| `applies_to` | string[] | 应用范围：`master` / `slave` / `both` |
| `register_config` | object | 寄存器字段配置字典，按寄存器名组织 |
| `prerequisites` | string[] | 前置条件（GPIO 配置、中断使能等） |
| `constraints` | string[] | 运行约束和限制 |
| `source` | string | 手册引用 |
| `confidence` | float | 置信度 (0.0-1.0) |

**code-gen 消费规则**：
- 从 `configuration_strategies[]` 提取所有策略
- 为每个策略生成一个高级配置函数（如 `SPI_ConfigMultimaster()`）
- 函数内部调用 LL 层函数完成寄存器配置
- 在函数文档中标注 `prerequisites` 和 `constraints`

### 4.13 cross_field_constraints — 跨字段约束

记录**两个或多个字段之间的依赖关系**，包括前置条件、相互作用、不变式、初始化顺序等。

```json
"cross_field_constraints": [
  {
    "id": "CONSTRAINT_SSI_MODF",
    "type": "invariant_violation",
    "title": "Master + Software NSS 时 SSI 必须=1（INV_SPI_001）",
    "fields": ["CR1.MSTR", "CR1.SSM", "CR1.SSI"],
    "description": "主机模式 + 软件 NSS 管理时，SSI 必须为 1。若 SSI=0，硬件会检测 NSS 为低电平（因为软件强制到管脚），触发 MODF 并自动清除 MSTR，导致 SPI 退化为从机。",
    "precondition": "MSTR == 1 && SSM == 1 implies SSI == 1",
    "violation_effect": "MODF flag set by hardware; MSTR automatically cleared; SPI forced to slave mode",
    "enforced_by": ["code-gen:precondition", "reviewer-agent:static"],
    "source": "RM0008 §25.3.1 p.676, §25.5.1 p.715",
    "confidence": 0.99
  },
  {
    "id": "CONSTRAINT_BR_SPE_LOCK",
    "type": "precondition",
    "title": "BR[2:0] 不可在 SPE=1 时修改",
    "fields": ["CR1.BR", "CR1.SPE"],
    "description": "BR (baud rate) 是 LOCK 字段，仅可在 SPE=0 时配置。SPE=1 时写入无效（未定义行为）。",
    "precondition": "write_to(CR1.BR) requires CR1.SPE == 0",
    "procedure": [
      "Check if SPE == 1",
      "If true, clear SPE (CR1.SPE = 0)",
      "Wait for any pending transmission to complete",
      "Modify BR[2:0]"
    ],
    "violation_effect": "Write to BR is silently ignored; actual baud rate does not change",
    "enforced_by": ["code-gen:guard"],
    "source": "RM0008 §25.5.1 p.715 Notes",
    "confidence": 0.99
  },
  {
    "id": "CONSTRAINT_DFF_CRC",
    "type": "interaction",
    "title": "DFF 修改时 CRC 寄存器复位",
    "fields": ["CR1.DFF", "CR1.CRCEN", "RXCRCR", "TXCRCR"],
    "description": "当改变 DFF（8位↔16位）时，CRC 寄存器（RXCRCR/TXCRCR）需通过切换 CRCEN 进行复位。否则可能残留前一次传输的 CRC 值，导致 CRC 校验错误。",
    "precondition": "SPE == 0 && (DFF change detected)",
    "procedure": [
      "Ensure SPE == 0",
      "Clear CRCEN (CR1.CRCEN = 0)",
      "Change DFF (CR1.DFF = new_value)",
      "Set CRCEN = 1 (if CRC is used)"
    ],
    "violation_effect": "CRC mismatch detection fails; stale CRC values cause spurious CRCERR",
    "enforced_by": ["code-gen:guard", "reviewer-agent:static"],
    "source": "RM0008 §25.5.1 Notes, Errata ES0306 (pending)",
    "confidence": 0.85
  },
  {
    "id": "CONSTRAINT_AFIO_GPIO_ORDER",
    "type": "initialization_order",
    "title": "AFIO 重映射必须在 GPIO 配置前完成",
    "fields": ["AFIO_MAPR", "GPIO_CRL/CRH"],
    "description": "对于支持重映射的外设（如 SPI1），AFIO_MAPR 重映射位必须在 GPIO 模式/复用配置之前设置。否则 GPIO 可能在错误的引脚上使能复用功能。",
    "precondition": "if_remap_needed() then AFIO_MAPR_set before GPIO_config",
    "procedure": [
      "1. Check if SPI remapping is needed",
      "2. If yes, set AFIO_MAPR.<peripheral>_REMAP bit",
      "3. Then configure GPIO pins (CNF+MODE)"
    ],
    "applies_to": ["SPI1"],
    "violation_effect": "GPIO复用配置作用到错误的物理引脚，导致通信无法建立",
    "enforced_by": ["code-gen:order"],
    "source": "RM0008 §9.3.10 p.176",
    "confidence": 0.97
  }
]
```

**字段说明**：

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `id` | string | 是 | 唯一标识，格式 `CONSTRAINT_<field1>_<field2>` |
| `type` | enum | 是 | 约束类型：`interaction` / `precondition` / `invariant_violation` / `initialization_order` |
| `fields` | string[] | 是 | 涉及的寄存器字段（全路径，如 `CR1.BR`） |
| `description` | string | 是 | 详细说明为什么存在这个约束 |
| `violation_effect` | string | 是 | 违反约束时硬件的实际行为 |
| `source` | string | 是 | 手册引用 |
| `title` | string | 否 | 简述（可选，用于人类快速浏览） |
| `precondition` | string | 否 | 前置条件表达式或自然语言说明（`precondition` 类型建议提供） |
| `procedure` | string[] | 否 | 正确处理步骤（`interaction` 和 `initialization_order` 类型建议提供） |
| `applies_to` | string[] | 否 | 适用的外设实例（如 `["SPI1"]`，省略表示所有实例） |
| `enforced_by` | string[] | 否 | 谁来保证：`code-gen:guard` / `code-gen:order` / `reviewer-agent:static` / `runtime:assert` |
| `confidence` | float | 否 | 置信度 (0.0-1.0) |

**约束类型说明**：

| 类型 | 含义 | code-gen 处理 |
|------|------|--------------|
| `precondition` | 字段修改需要满足的前置条件 | 生成 guard 检查代码 |
| `interaction` | 修改一个字段会影响其他字段 | 生成级联操作序列 |
| `invariant_violation` | 字段组合违反硬件不变式 | 插入断言和合规检查 |
| `initialization_order` | 初始化步骤的依赖顺序 | 确保 init_sequence 的执行顺序 |

**code-gen 消费规则**：
1. 遍历 `cross_field_constraints[]`
2. 对于 `enforced_by: code-gen:guard` 的约束，在对应字段写入前生成条件检查
3. 对于 `enforced_by: code-gen:order` 的约束，验证 `init_sequence` 中的步骤顺序
4. 在生成的代码中插入注释标注约束 ID（如 `/* CONSTRAINT_BR_SPE_LOCK */`）

示例（INV_SPI_002）：
```c
// Guard: CONSTRAINT_BR_SPE_LOCK
// BR[2:0] must be configured when SPE=0
if (SPIx->CR1 & SPI_CR1_SPE_Msk) {
    SPIx->CR1 &= ~SPI_CR1_SPE_Msk;  // Clear SPE first
}
// Now safe to modify BR
SPIx->CR1 = (SPIx->CR1 & ~SPI_CR1_BR_Msk) | ((br_value << SPI_CR1_BR_Pos) & SPI_CR1_BR_Msk);
```

### 4.14 generation_metadata — 生成元信息

```json
"generation_metadata": {
  "generated_by": "doc-analyst",
  "generation_date": "2026-04-11",
  "source_documents": [
    {"name": "RM0008", "version": "Rev 21", "path": "docs/stm32f103_reference_manual.pdf"},
    {"name": "ES0306", "version": "Rev 7",  "path": "docs/stm32f103_errata.pdf"}
  ],
  "total_registers": 24,
  "total_bitfields": 87,
  "average_confidence": 0.93,
  "fields_below_threshold": 2,
  "validation_passed": true
}
```

---

## 5. doc-analyst 工作流程

### 5.1 核心提取步骤

```
1. 读取芯片手册（PDF -> 文本/表格提取，或 SVD -> 直接解析）
2. 逐个寄存器提取：name, offset, width, access, reset_value, bitfields
3. 对每个 bitfield 确定 access type
   - 必须忠实反映手册标注（如 rc_w0 -> W0C, rc_w1 -> W1C）
   - 重点关注 W1C、W0C、RC、RC_SEQ、WO_TRIGGER 等特殊类型
   - 多步清除的标志位必须定义 atomic_sequence 并在 bitfield 中通过 clear_sequence 引用
4. 提取功能描述：init_sequence（每步含 layer 和 requires 标注）, operation_modes, error_handling
5. 跨章节交叉提取：RCC 时钟(clock[])、GPIO 复用(gpio_config[])、NVIC 中断(interrupts[])、DMA 通道(dma_channels[])
6. 提取配置策略(configuration_strategies[])：识别不同初始化/运行场景（如单主机、多主机、从机模式），标注 prerequisites 和 constraints
7. 提取跨字段约束(cross_field_constraints[])：识别字段间的前置条件(precondition)、交互作用(interaction)、不变式违反(invariant_violation)、初始化顺序(initialization_order)
8. 检查勘误表（若 docs/ 下存在 errata 文件）
9. 对 confidence < 0.85 的字段，创建 pending_review
10. 执行 §5.2~5.3 完整性检查，运行 `python3 scripts/validate-ir.py` 验证
11. 输出 IR JSON 到 ir/<module>_ir_summary.json（唯一机器消费格式，遵循 §3 顶层结构）
```

### 5.2 完整性检查 — 模式穷举与约束提取

**核心目标**：防止生成的 IR 遗漏操作模式、配置策略和跨字段约束。

#### 5.2.1 配置矩阵穷举（Configuration Mode Matrix）

对多位配置字段的所有**合法组合**进行穷举，并明确标注在 `operation_modes[]` 中。

**方法**：
1. 识别所有**配置主导字段**（影响硬件行为的寄存器位，如 BIDIMODE、RXONLY、MSTR）
2. 枚举所有可能的位值组合
3. 查阅手册确定哪些组合**合法**、哪些**保留**、哪些**非法**
4. 对每个合法组合，创建一个 `operation_modes[]` 条目

**SPI 示例**（应覆盖但原 IR 缺失的模式）：

```json
"operation_modes": [
  // 全双工模式（BIDIMODE=0, RXONLY=0）
  {"name": "full_duplex_master", "cr1": {"BIDIMODE": 0, "RXONLY": 0, "MSTR": 1}},
  {"name": "full_duplex_slave", "cr1": {"BIDIMODE": 0, "RXONLY": 0, "MSTR": 0}},
  
  // 仅接收模式（BIDIMODE=0, RXONLY=1）— 缺失 slave variant
  {"name": "receive_only_master", "cr1": {"BIDIMODE": 0, "RXONLY": 1, "MSTR": 1}},
  {"name": "receive_only_slave", "cr1": {"BIDIMODE": 0, "RXONLY": 1, "MSTR": 0}},  // ← 原IR缺失
  
  // 单线双向模式（BIDIMODE=1, RXONLY=ignored, BIDIOE控制方向）
  {"name": "bidirectional_tx_master", "cr1": {"BIDIMODE": 1, "BIDIOE": 1, "MSTR": 1}},
  {"name": "bidirectional_rx_master", "cr1": {"BIDIMODE": 1, "BIDIOE": 0, "MSTR": 1}},
  {"name": "bidirectional_tx_slave", "cr1": {"BIDIMODE": 1, "BIDIOE": 1, "MSTR": 0}},    // ← 原IR缺失
  {"name": "bidirectional_rx_slave", "cr1": {"BIDIMODE": 1, "BIDIOE": 0, "MSTR": 0}}     // ← 原IR缺失
]
```

**检查清单**：
- [ ] 对每个多位字段组合（如 BIDIMODE × RXONLY × MSTR），是否列举了所有 2^n 种理论组合？
- [ ] 是否明确标注了哪些组合保留或非法（在 description 中或 `pending_reviews` 中）？
- [ ] master/slave 的对称模式是否都出现了？

#### 5.2.2 配置策略提取（Configuration Strategies）

识别**不同的初始化和运行策略**，这些策略可能不是单纯的寄存器位组合，而是**场景级别的配置**。

**SPI 示例**（原 IR 未明确列出）：

```json
"configuration_strategies": [
  {
    "id": "single_master_mode",
    "name": "Single-Master Mode",
    "description": "标准的单主机通信模式。主机控制 NSS，从机被动响应。",
    "applies_to": ["master"],
    "register_config": {
      "CR1": {"MSTR": 1, "SSM": 1, "SSI": 1},  // ← INV_SPI_001: SSI必须=1
      "CR2": {"SSOE": 0}                        // 禁止 SSOE 在 single-master
    },
    "prerequisites": ["NSS pin configured as GPIO output"],
    "constraints": [
      "SSI must be 1 (INV_SPI_001)",
      "SSOE must be 0 in single-master mode"
    ],
    "source": "RM0008 §25.3.3",
    "confidence": 0.99
  },
  {
    "id": "multimaster_mode",
    "name": "Multimaster Mode (Arbitration)",
    "description": "多主机竞争模式。每个节点通过 NSS 脚信号和 MODF 监听实现仲裁。",
    "applies_to": ["master"],
    "register_config": {
      "CR1": {"MSTR": 1, "SSM": 0},
      "CR2": {"SSOE": 0, "ERRIE": 1}            // CRITICAL: SSOE禁止; ERRIE必须使能以检测碰撞
    },
    "prerequisites": [
      "NSS configured as input (floating or pulled up)",
      "MODF interrupt enabled (CR2.ERRIE=1) for collision detection",
      "Each master drives NSS low only during its own transmission"
    ],
    "constraints": [
      "Cannot use SSOE in multimaster environment",
      "MODF flag indicates collision; master must release bus and retry"
    ],
    "source": "RM0008 §25.3.1",
    "confidence": 0.98
  },
  {
    "id": "hardware_nss_slave_mode",
    "name": "Hardware NSS Slave Mode",
    "description": "从机使用硬件 NSS 管理。NSS 拉低时使能，拉高时禁用接收。",
    "applies_to": ["slave"],
    "register_config": {
      "CR1": {"MSTR": 0, "SSM": 0},
      "CR2": {"SSOE": 0}
    },
    "prerequisites": [
      "NSS pin configured as input (floating)",
      "Master controls NSS signal"
    ],
    "constraints": [
      "Slave becomes active only when NSS is pulled low"
    ],
    "source": "RM0008 §25.3.2",
    "confidence": 0.98
  },
  {
    "id": "software_nss_slave_mode",
    "name": "Software NSS Slave Mode",
    "description": "从机使用软件 NSS 管理。SSI 位决定是否响应。",
    "applies_to": ["slave"],
    "register_config": {
      "CR1": {"MSTR": 0, "SSM": 1, "SSI": 0},
      "CR2": {"SSOE": 0}
    },
    "prerequisites": [
      "NSS pin can be used for other purposes"
    ],
    "constraints": [
      "SSI=0 allows MODF detection for collision handling"
    ],
    "source": "RM0008 §25.3.2",
    "confidence": 0.98
  }
]
```

#### 5.2.3 跨字段约束识别（Cross-field Constraints）

识别**两个或多个字段之间的依赖关系**，包括：
- **字段修改的前置条件**（如 DFF 必须在 SPE=0 时修改）
- **字段间的相互作用**（如修改 DFF 时需重置 CRC）
- **字段的互斥关系**（如某些组合是非法的）

**格式**：

```json
"cross_field_constraints": [
  {
    "id": "CONSTRAINT_DFF_CRC",
    "type": "interaction",
    "title": "DFF 修改时 CRC 寄存器复位",
    "fields": ["CR1.DFF", "CR1.CRCEN", "RXCRCR", "TXCRCR"],
    "description": "当改变 DFF（8位↔16位）时，CRC 寄存器（RXCRCR/TXCRCR）需通过切换 CRCEN 进行复位。否则可能残留前一次传输的 CRC 值。",
    "precondition": "SPE == 0 && (DFF change detected)",
    "procedure": [
      "Clear CRCEN (CR1.CRCEN = 0)",
      "Change DFF",
      "Set CRCEN = 1 (if CRC is used)"
    ],
    "violation_effect": "CRC mismatch detection fails; stale CRC values cause spurious CRCERR",
    "source": "RM0008 §25.5.1 Notes, Errata ES0306 (pending confirmation)"
  },
  {
    "id": "CONSTRAINT_BR_SPE_LOCK",
    "type": "precondition",
    "title": "BR[2:0] 不可在 SPE=1 时修改",
    "fields": ["CR1.BR", "CR1.SPE"],
    "description": "BR (baud rate) 是 LOCK 字段，仅可在 SPE=0 时配置。SPE=1 时写入无效。",
    "precondition": "write_to(CR1.BR) requires CR1.SPE == 0",
    "violation_effect": "Write to BR is silently ignored; actual baud rate does not change",
    "source": "RM0008 §25.5.1 p.715 Notes"
  },
  {
    "id": "CONSTRAINT_SSI_MODF",
    "type": "invariant_violation",
    "title": "Master + Software NSS 时 SSI 必须=1（INV_SPI_001）",
    "fields": ["CR1.MSTR", "CR1.SSM", "CR1.SSI"],
    "description": "主机模式 + 软件 NSS 管理时，SSI 必须为 1。若 SSI=0，硬件会检测 NSS 为低电平（因为软件强制到管脚），触发 MODF 并自动清除 MSTR，导致 SPI 退化为从机。",
    "precondition": "MSTR == 1 && SSM == 1 implies SSI == 1",
    "violation_effect": "MODF flag set by hardware; MSTR automatically cleared; SPI forced to slave mode",
    "enforced_by": ["code-gen:precondition", "reviewer-agent:static"],
    "source": "RM0008 §25.3.1, §25.5.1"
  },
  {
    "id": "CONSTRAINT_AFIO_GPIO_ORDER",
    "type": "initialization_order",
    "title": "AFIO 重映射必须在 GPIO 配置前完成",
    "fields": ["AFIO_MAPR", "GPIO_CRL/CRH"],
    "description": "对于支持重映射的外设（如 SPI1），AFIO_MAPR 重映射位必须在 GPIO 模式/复用配置之前设置。否则 GPIO 可能在错误的引脚上使能复用功能。",
    "procedure": [
      "1. Set AFIO_MAPR.SPI1_REMAP (if needed)",
      "2. Then configure GPIO pins (CNF+MODE)"
    ],
    "applies_to": ["SPI1"],
    "violation_effect": "GPIO复用配置作用到错误的物理引脚，导致通信无法建立",
    "source": "RM0008 §9.3.10"
  }
]
```

#### 5.2.4 初始化序列的前置条件标注

在 `functional_model.init_sequence` 的每一步中，若某字段依赖前置条件，显式标注 `requires` 字段：

```json
"init_sequence": [
  {
    "order": 1,
    "description": "Enable SPI peripheral clock via RCC",
    "register": "RCC_APB2ENR",
    "field": "SPI1EN",
    "layer": "drv",
    "source": "RM0008 §7.3.7"
  },
  {
    "order": 2,
    "description": "Configure AFIO remapping (if needed)",
    "register": "AFIO_MAPR",
    "field": "SPI1_REMAP",
    "value": "0 or 1",
    "condition": "depends on desired pin mapping",
    "layer": "drv",
    "source": "RM0008 §9.3.10",
    "note": "Must be done BEFORE GPIO configuration"
  },
  {
    "order": 3,
    "description": "Configure GPIO pins for SPI alternate function",
    "register": "GPIOx_CRL/CRH",
    "field": "CNFy, MODEy",
    "layer": "drv",
    "requires": "step 2 completed (if AFIO remap needed)",
    "source": "RM0008 §9.1.11"
  },
  {
    "order": 4,
    "description": "Guard: ensure SPE=0 before modifying LOCK fields",
    "register": "CR1",
    "field": "SPE",
    "value": "0",
    "layer": "ll",
    "guard": "INV_SPI_002",
    "requires": "must be checked before any CR1 field modification",
    "source": "RM0008 §25.5.1 Notes"
  }
  // ...more steps
]
```

### 5.3 完整性检查流程

完成上述 5.2 的提取后，运行以下自验证：

```
A. 模式完整性
  - [ ] 配置矩阵穷举：所有主导字段的组合都有对应 operation_modes 条目或 pending_review
  - [ ] master/slave 对称性：slave 版本是否都列出了？
  
B. 策略完整性
  - [ ] configuration_strategies[] 是否覆盖了所有重要的使用场景？（single-master, multimaster, etc.）
  - [ ] 每个策略是否标注了 prerequisites 和 constraints？
  
C. 约束完整性
  - [ ] 是否识别了所有 LOCK 字段及其前置条件？
  - [ ] 是否识别了所有跨字段依赖？
  - [ ] 初始化序列中的每一步是否有前置条件标注（requires）？
  
D. 初始化顺序
  - [ ] init_sequence 中依赖其他步骤的项是否显式标注了 requires？
  - [ ] AFIO/GPIO/外设时钟的顺序是否正确？
  
E. Errata
  - [ ] 是否存在来自 docs/ 的 errata 文件？
  - [ ] 若缺失，是否在 pending_reviews 中标注？
```

---

## 6. code-gen 消费规则

code-gen 从 IR JSON 生成代码时，**必须遵守**以下映射关系：

### 6.1 寄存器层（`*_reg.h`）— 确定性映射

| IR 字段 | 生成目标 |
|---------|---------|
| `register.name` + `_Pos` / `_Msk` | 每个 bitfield 生成 `_Pos` 和 `_Msk` 宏对 |
| `register.offset` | 结构体成员偏移注释 |
| `register.access` | CMSIS 访问宏（`__IO` / `__I` / `__O`） |
| `instances[].base_address` | 外设基地址宏 `#define CANx ((CAN_TypeDef *)addr)` |

### 6.2 LL 层（`*_ll.h`）— access type 驱动

| bitfield.access | LL 函数生成模式 |
|-----------------|----------------|
| `RW` | 生成 Set/Clear/Toggle 函数，允许 `\|=` 和 `&=` |
| `RO` | 只生成 Get/Is 函数，无写入函数 |
| `WO` | 只生成 Write/Set 函数，无读取函数 |
| `W1C` | 生成 Clear 函数，**必须用直接赋值（`=`），禁止 `\|=`** |
| `W0C` | 生成 Clear 函数，通过 `&= ~Msk` 或安全赋值清除目标位。若同一寄存器含 W1C 位，写回值中 W1C 位必须为 0 |
| `W1S` | 生成 Set 函数，直接赋值目标位 |
| `RC` | 生成 Read 函数，注释标注"读即清除"副作用 |
| `RC_SEQ` | 生成 Clear 函数，函数体**严格按 `atomic_sequences` 中引用的步骤顺序**生成读写操作 |
| `WO_TRIGGER` | 只生成 Trigger 函数，直接赋值，无需读回确认 |
| `mixed`（寄存器级） | **禁止**生成整体的 Set/Clear/Toggle。只逐 bitfield 生成独立函数，各走各的 access 规则 |

### 6.3 Driver 层（`*_drv.c`）— 功能语义驱动

| IR 字段 | 生成目标 |
|---------|---------|
| `functional_model.init_sequence` | `Xxx_Init()` 函数体中的操作顺序。按 `layer` 字段决定：`ll` -> 调用 LL 函数，`drv` -> 在 drv 层组装逻辑（含等待循环、跨模块调用）。根据 `requires` 字段验证步骤间依赖关系，确保前置步骤先执行（如 RCC 时钟使能 → GPIO 配置 → SPI 配置） |
| `functional_model.deinit_sequence` | `Xxx_DeInit()` 函数体 |
| `functional_model.operation_modes` | 模式枚举和配置逻辑 |
| `configuration_strategies` | 为每个策略生成高级配置函数（如 `SPI_ConfigMultimaster()`），内部调用 LL 层函数。在函数文档中标注 prerequisites 和 constraints |
| `cross_field_constraints` | 在涉及约束字段的初始化代码中插入 guard 检查和级联操作。使用 `enforced_by` 字段决定放置位置。在代码中插入注释标注约束 ID（如 `/* CONSTRAINT_BR_SPE_LOCK */`） |
| `errors[]` | 错误处理函数，按 `recovery_sequence` 引用调用对应 LL 封装 |
| `errata[].workaround` | 在对应代码位置插入 workaround 并注释 errata ID |

---

## 7. 验证清单

doc-analyst 输出 IR 后，必须通过以下自验证：

| # | 检查项 | 方法 |
|---|--------|------|
| 1 | JSON 语法合法 | `jq . ir/<module>_ir_summary.json` |
| 2 | 所有寄存器偏移无重叠 | 脚本检查 offset + width/8 不交叉 |
| 3 | 所有位域无重叠 | 同一寄存器内 bit_offset + bit_width 不交叉 |
| 4 | 位域不超出寄存器宽度 | bit_offset + bit_width <= register.width |
| 5 | 所有字段都有 `source` | 遍历检查 source 非空 |
| 6 | W1C/W0C/RC/RC_SEQ 等特殊 access type 已标注 | 检查含状态/标志的寄存器 |
| 7 | 所有 `RC_SEQ` 位域的 `clear_sequence` 引用存在 | 检查 atomic_sequences 中有对应 ID |
| 8 | `clock[]` 与 `instances[]` 数量匹配 | 每个实例都有时钟配置 |
| 9 | `interrupts[]` 非空且每项有 `irq_number` | 无中断的外设需显式标注 |
| 10 | `pending_reviews` 中无未解决项（仅 code-gen 前检查） | `resolved == true` |
| 11 | `average_confidence >= 0.85` | 否则整体需人工审核 |
| 12 | `configuration_strategies[]` 非空 | 至少包含 1 种策略；多主机/单主机等重要场景都应列出 |
| 13 | `cross_field_constraints[]` 非空 | 至少包含 1 个约束；所有 LOCK 字段、字段交互、初始化顺序都应列出 |
| 14 | 约束的 `violation_effect` 清晰 | 每个约束都明确描述违反后果（"未定义行为" / "硬件自动修复" / "数据丢失" 等） |
| 15 | `init_sequence` 中关键步骤有 `requires` | 初始化顺序依赖（如 AFIO → GPIO）都应标注 |
| 16 | `instance_model` 存在且 `topology != flat` 时，所有 `registers[]` 记录须有 `register_set_id` | 遍历检查 |
| 17 | `instance_model.instances[]` 中不存在环（tree topology） | DFS 检测 |
| 18 | `signal_graph[].source.instance_id` / `sink.instance_id` 均在 `instance_model` 或 `instances[]` 中存在 | ID 查找 |
| 19 | `signal_graph[].routing_nodes[].select_register` 在对应外设 `registers[]` 中存在 | 跨外设引用标注 `peripheral` 字段 |
| 20 | `signal_graph[type=event]` 与 `interrupts[]` 的 `irq_number` 无重复定义冲突 | 对照检查 |
| 21 | `shared_resources[].requestors` ≥ 2，且 `fixed_priority` 时 `priority_level` 无重复 | 值唯一性检查 |
| 22 | `structural_relations[].participants[].instance_id` 均在 `instance_model` 或 `instances[]` 中存在 | ID 查找 |

---

## 8. ASPICE CL2 追溯扩展（swr_refs）

本章定义为满足 **ASPICE CL2 SWE.1 双向追溯**要求而引入的扩展字段。

### 8.1 追溯链设计

```
docs/srs/<module>_srs.md        ← SWE.1 工作产品（SWR 权威来源）
    │  (SWR ID 锚点)
    ▼
ir/<module>_feature_matrix.json  ← swr_refs 字段：SWR → Feature 映射
    │  (feature.ir_refs)
    ▼
ir/<module>_ir_summary.json      ← 寄存器/位域/序列（硬件规格权威来源）
    │
    ▼
src/<module>_*.c / *.h           ← 代码（注释中标注 SWR ID）
    │
    ▼
tests/<module>_test.c            ← SWE.4 测试用例（引用 SWR ID 和 TC ID）
```

### 8.2 feature_matrix.json 中的 swr_refs 字段

在 `ir/<module>_feature_matrix.json` 的每个 feature 对象中新增 `swr_refs` 字段：

```json
{
  "feature_id": "CAN-INIT",
  "category": "生命周期",
  "name": "Init / DeInit",
  "status": "todo",
  "target_apis": ["Can_Init", "Can_DeInit"],
  "depends_on": [],
  "ir_refs": [
    "functional_model.init_sequence",
    "registers.MCR",
    "registers.MSR"
  ],
  "swr_refs": [
    "SWR-CAN-001",
    "SWR-CAN-002"
  ],
  "acceptance": "Can_Init 完成初始化序列；Can_DeInit 释放资源",
  "notes": ""
}
```

**字段规则**：

| 规则 | 说明 |
|---|---|
| `swr_refs` 非空 | 每个 feature 至少关联 1 条 SWR；无对应 SWR 的 feature 须在 notes 中注明豁免原因 |
| SWR 必须存在 | `swr_refs` 中列出的每个 ID 必须在 `docs/srs/<module>_srs.md` 中有对应的 `### SWR-XXX-NNN` 标题 |
| 双向覆盖 | SRS 追溯矩阵（第 11 节）中的每条 SWR 必须对应一个 feature；`scripts/validate-srs-traceability.py` 自动检查 |
| schema_version | 引入 `swr_refs` 的 feature_matrix 文件须将 `schema_version` 升至 `"1.1"` |

### 8.3 代码层注释规范

code-gen 在生成或修改函数时，若函数直接实现某条 SWR，应在函数头注释中标注：

```c
/* SWR: SWR-CAN-001 — CAN 外设初始化
 * IR:  functional_model.init_sequence (can_ir_summary.json)
 * TC:  TC-CAN-001
 */
Can_StatusType Can_Init(CAN_TypeDef *CANx, const Can_ConfigType *config)
{
    ...
}
```

**注释字段说明**：

| 字段 | 含义 | 是否必填 |
|---|---|---|
| `SWR:` | 对应的软件需求 ID | 必填（若有对应 SWR） |
| `IR:` | 消费的 IR JSON 元素路径 | 必填（R1 要求） |
| `TC:` | 验证该函数的测试用例 ID | 必填（有测试时） |

### 8.4 SRS 文件约定

- **存放位置**：`docs/srs/<module>_srs.md`（如 `docs/srs/can_srs.md`）
- **SWR ID 格式**：`SWR-<MODULE>-<NNN>`（三位数字，从 001 开始连续编号，不分子类）
- **优先级枚举**：`M`（必须/Must）、`S`（应当/Should）、`O`（可选/Optional）
- **状态枚举**：`草稿` → `评审中` → `已批准`（须在 frontmatter `review_status` 字段维护）
- **模板**：`docs/srs/srs-template.md`（新模块复制此模板填写）
- **CI 检查**：`scripts/validate-srs-traceability.py` 在 Gate 1（IR Gate）后自动运行

### 8.5 CI 集成点

| 阶段 | 检查 | 失败退出码 |
|---|---|---|
| Gate 1（IR Gate）之后 | `validate-srs-traceability.py --module <module>` | 1（覆盖缺口）/ 2（需人工审核） |
| Gate 2（Review Gate）之前 | reviewer-agent 检查 swr_refs 与 SRS 的一致性 | 拒绝签发 token |
| 代码生成后 | 检查 `/* SWR: */` 注释覆盖率 | 警告（非阻断） |

### 8.6 SRS 评审记录集成

SRS 评审结论记录到 `.claude/review-decisions.md`，格式与 R9 决策记录一致，类别使用 `srs_review`：

```
| 日期 | reviewer-agent | srs_review | SRS-CAN-001 需求完整性 | 批准 | 0.95 | QM | docs/srs/can_srs.md | R9@v3.1 |
```

---

## 9. `instance_model` — 层次化实例模型（v4.0 新增）

### 9.1 设计动机

`instances[]`（v3.x）仅支持平级多实例（同一外设的多个独立副本）。当外设内部具有**嵌套子单元**（如 ADC Kernel → Group → Channel、GTM Cluster → Channel、DMA Controller → Channel Group → Channel）时，平级结构无法表达地址层次和寄存器归属。`instance_model` 用有向树（或有向无环图）替代平级数组，描述任意深度的实例层次。

**向后兼容原则**：`instance_model` 为可选字段。不存在时 code-gen 退回到 `instances[]` 处理逻辑；`topology: "flat"` 时两者等价。

### 9.2 顶层字段

```json
"instance_model": {
  "topology": "flat | tree | dag",
  "instances": [ <InstanceNode>, ... ]
}
```

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `topology` | enum | 是 | `flat`：平级，等价于旧 `instances[]`；`tree`：有根树；`dag`：有向无环图（共享子节点） |
| `instances` | array | 是 | 所有实例节点的扁平列表，通过 `parent` / `children` 构造树形 |

### 9.3 InstanceNode 字段

```json
{
  "id": "VADC_K0_G0",
  "level": 2,
  "base_address": "0xF0200420",
  "base_address_mode": "absolute",
  "register_set_id": "VADC_GROUP_REGS",
  "parent": "VADC_K0",
  "children": ["VADC_K0_G0_CH0", "VADC_K0_G0_CH1"],
  "multiplicity": {
    "count": 2,
    "stride": "0x0400",
    "index_start": 0,
    "index_macro": "VADC_GROUP_BASE(k, g)"
  },
  "source": "TC39xB_UM §29.3.1 Table 29-1"
}
```

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `id` | string | 是 | 全局唯一标识符，供 `signal_graph` / `structural_relations` 引用 |
| `level` | integer | 是 | 树的深度，根节点为 0 |
| `base_address` | string (hex) | 是（level 0）；可选（其他） | level 0 用绝对地址；深层节点若 `base_address_mode: relative` 则为相对父节点的偏移 |
| `base_address_mode` | enum | 否 | `absolute`（默认）或 `relative_to_parent` |
| `register_set_id` | string | 是 | 关联 `registers[]` 中同一 `register_set_id` 的寄存器组；同一 set 的寄存器由该节点的 `base_address` + 各寄存器 `offset` 定位 |
| `parent` | string \| null | 是 | 父节点 `id`；根节点填 `null` |
| `children` | string[] | 否 | 子节点 `id` 列表；叶节点省略或填 `[]` |
| `multiplicity` | object | 否 | 当该类型节点在父节点下存在多份相同布局的副本时使用 |
| `multiplicity.count` | integer | 是（若有） | 副本数量 |
| `multiplicity.stride` | string (hex) | 是（若有） | 相邻副本间的地址间距 |
| `multiplicity.index_start` | integer | 否 | 第一个副本的索引，默认 0 |
| `multiplicity.index_macro` | string | 否 | 推荐生成的基地址宏名称模板，code-gen 使用 |
| `source` | string | 是 | 手册章节引用 |

### 9.4 对 `registers[]` 的变化

每条寄存器记录增加可选字段 `register_set_id`，标明其归属的实例层级：

```json
{
  "name": "ARBCFG",
  "register_set_id": "VADC_GROUP_REGS",
  "offset": "0x0080",
  "width": 32,
  ...
}
```

**规则**：
- 若 `instance_model` 存在且 `topology != "flat"`，所有寄存器**必须**填写 `register_set_id`
- 同一 `register_set_id` 的寄存器布局在所有同级实例中完全一致（stride 寻址前提）
- `register_set_id` 命名约定：`<PERIPHERAL>_<LEVEL_NAME>_REGS`，如 `VADC_KERNEL_REGS`、`VADC_GROUP_REGS`

### 9.5 code-gen 消费规则

```
1. 读取 instance_model.topology
   - flat → 退回旧 instances[] 逻辑
   - tree / dag → 执行以下步骤

2. 拓扑排序：BFS 遍历 instances，按 level 分组

3. 对每个 level > 0 且有 multiplicity 的节点类型：
   生成基地址宏：
     #define <INDEX_MACRO>  (PARENT_BASE + (n) * stride)
   生成实例类型定义（复用同一 typedef，无需重复）

4. 对每个 register_set_id：
   收集所有 registers[register_set_id == x] 的寄存器
   生成结构体 typedef <PERIPHERAL>_<LEVEL>_TypeDef

5. 生成实例指针宏：
   对非 multiplicity 节点：#define INST_ID  ((<TypeDef>*)base_address)
   对 multiplicity 节点：  #define INST_ID(n) ((<TypeDef>*)<INDEX_MACRO>(n))
```

### 9.6 验证规则

| # | 检查项 |
|---|---|
| V9.1 | 所有 `id` 在 `instances[]` 内唯一 |
| V9.2 | `parent` 引用的 `id` 必须存在（根节点除外） |
| V9.3 | `children` 引用的 `id` 必须存在 |
| V9.4 | `topology: tree` 时不得存在环（用 DFS 检测） |
| V9.5 | `instance_model` 存在且 `topology != flat` 时，`registers[]` 中每条记录须有 `register_set_id` |
| V9.6 | `register_set_id` 须在某个实例节点中被引用 |
| V9.7 | `multiplicity.stride` × `multiplicity.count` 不得超出父节点地址空间（若父节点有 size 字段） |

---

## 10. `signal_graph` — 信号图（v4.0 新增）

### 10.1 设计动机

外设间的所有流动——触发脉冲、时钟信号、数据通路、同步脉冲、事件/中断——本质上都是有向图中的边。`signal_graph` 用统一的有向边结构描述这些流动，通过 `type` 字段区分语义，通过 `routing_nodes[]` 描述中间路由节点（mux、总线、SRC、分频器等）。

**与现有字段的关系**：
- `interrupts[]` 保留，描述事件源的寄存器级属性；路由路径（如经 SRC 到 CPU）用 `signal_graph[type=event]` 补充
- `dma_channels[]` 保留，描述 DMA 通道静态映射；跨外设触发链用 `signal_graph[type=trigger]` 补充
- `clock[]` 保留，描述外设总线时钟使能；片内分频链用 `signal_graph[type=clock]` 补充

### 10.2 Signal 对象字段

```json
{
  "id": "SIG_GTM_TO_VADC_K0G0",
  "type": "trigger",
  "description": "GTM ATOM0 CH0 output triggers VADC K0 Group0 request source 0",
  "source": {
    "instance_id": "GTM_ATOM0_CH0",
    "peripheral": "GTM",
    "port": "OUT"
  },
  "routing_nodes": [
    {
      "node_type": "mux",
      "node_id": "OTGB0_LINE0",
      "peripheral": "GTM",
      "select_register": "OTGB0_CTRL",
      "select_field": "SEL0",
      "select_value": 5,
      "source": "TC39xB_UM §20.9.1"
    }
  ],
  "sink": {
    "instance_id": "VADC_K0_G0",
    "peripheral": "VADC",
    "port": "TRIG_IN_0",
    "connect_register": "K0_G0_REQTSEL",
    "connect_field": "TRSEL",
    "connect_value": 2,
    "edge_type": "rising",
    "source": "TC39xB_UM §29.7.3"
  },
  "config_sequence": "SEQ_GTM_VADC_TRIG_SETUP",
  "doc_source": "TC39xB_UM §29.7 & §20.9"
}
```

#### 顶层字段

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `id` | string | 是 | 全局唯一，供 `shared_resources` / `structural_relations` 引用 |
| `type` | enum | 是 | 见下表 |
| `description` | string | 否 | 可读说明 |
| `source` | object | 是 | 信号起点 |
| `routing_nodes` | array | 否 | 中间路由节点，空列表表示直接连接 |
| `sink` | object | 是 | 信号终点 |
| `config_sequence` | string | 否 | 建立此信号连接所需的 `atomic_sequences` ID |
| `doc_source` | string | 是 | 手册章节引用（注意与信号起点字段 `source` 区分） |

#### `type` 枚举

| 值 | 含义 | code-gen 生成 | 典型场景 |
|---|---|---|---|
| `trigger` | 外部硬件触发脉冲 | 触发源选择配置函数 + 使能 | GTM→ADC, TIM→ADC |
| `clock` | 时钟信号（含分频） | 时钟使能 + 分频系数配置 | 外设内部分频链 |
| `data` | 数据通路（结果、FIFO、DMA） | 数据读写函数 + DMA/FIFO 配置 | ADC结果→内存 |
| `sync` | 同步脉冲（同时启动多个单元） | 同步触发配置 | ADC kernel 同步 |
| `enable` | 使能信号（一个外设使能另一个） | 使能配置顺序约束 | 时钟门控 |
| `event` | 事件/中断信号（含路由节点） | 中断/SRC/NVIC 配置函数 | 外设→SRC→CPU |

#### `source` / `sink` 对象字段

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `instance_id` | string | 是 | 引用 `instance_model.instances[].id` 或 `instances[].name` |
| `peripheral` | string | 是 | 外设名（跨外设时尤其重要） |
| `port` | string | 否 | 逻辑端口名（同一实例可能有多个输入/输出端口） |
| `connect_register` | string | 否（sink 端推荐） | 终点配置寄存器名 |
| `connect_field` | string | 否（sink 端推荐） | 终点配置位域名 |
| `connect_value` | any | 否 | 写入值 |
| `edge_type` | enum | 否（trigger type） | `rising` \| `falling` \| `both` \| `level` |

#### `routing_nodes[]` 节点字段

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `node_type` | enum | 是 | 见下表 |
| `node_id` | string | 是 | 节点唯一标识 |
| `peripheral` | string | 是 | 该节点属于哪个外设 |
| `select_register` | string | 否 | 配置该节点的寄存器 |
| `select_field` | string | 否 | 配置该节点的位域 |
| `select_value` | any | 否 | 选择值 |
| `source` | string | 是 | 手册引用 |

#### `node_type` 枚举

| 值 | 含义 | code-gen 行为 |
|---|---|---|
| `mux` | 多路选择器（N 选 1） | 生成选择值写入代码 |
| `gate` | 信号门控（enable/disable） | 生成门控使能/禁用代码 |
| `prescaler` | 分频器 | 生成分频系数配置代码 |
| `src` | Service Request Controller（TC3xx） | 生成 SRC 节点配置代码（SRPN + TOS） |
| `bus` | 信号总线（多信号共享，只需选择） | 生成总线选择代码 |
| `fifo` | FIFO 缓冲（异步解耦） | 生成 FIFO 配置代码 |
| `buffer` | 单级缓冲寄存器 | 生成缓冲读写代码 |

### 10.3 简单外设的降级示例

STM32F1 CAN TX 中断（直连 NVIC，无路由节点）：

```json
{
  "id": "SIG_CAN1_TX_INT",
  "type": "event",
  "source": {
    "instance_id": "CAN1",
    "peripheral": "CAN",
    "port": "TX_COMPLETE_EVENT"
  },
  "routing_nodes": [],
  "sink": {
    "instance_id": "NVIC",
    "peripheral": "CORE",
    "port": "IRQ_19",
    "connect_register": "NVIC_ISER0",
    "connect_field": "bit19",
    "connect_value": 1
  },
  "doc_source": "RM0008 §24.8"
}
```

### 10.4 code-gen 消费规则

```
按 type 分派处理：

type=trigger：
  对 routing_nodes 中每个节点：
    生成：<Peripheral>_ConfigTriggerRoute(node_id, select_value)
  生成：<Peripheral>_EnableHardwareTrigger(instance_id, port, edge_type)

type=clock：
  对 routing_nodes 中 node_type=prescaler 的节点：
    生成：<Peripheral>_SetClockDivider(register, field, value)
  生成时钟使能调用（已有 clock[] 处理，此处补充分频配置）

type=event：
  对 routing_nodes 中 node_type=src 的节点：
    生成：SRC_Configure(node_id, priority, target_cpu_or_dma)
  无 routing_nodes 时：生成 NVIC_EnableIRQ(irq_number)

type=data：
  若 sink 端有 DMA 连接：生成 DMA 通道配置调用
  若无：生成结果读取函数（按 sink.connect_register）

type=sync / type=enable：
  生成对应的寄存器写操作序列，顺序依据 source → routing_nodes → sink

初始化顺序规则：
  同一外设的 signal_graph 节点按 source→sink 拓扑排序确定配置顺序
  跨外设时：source 外设的 signal_graph 节点先于 sink 外设配置
```

### 10.5 验证规则

| # | 检查项 |
|---|---|
| V10.1 | 所有 `id` 全局唯一 |
| V10.2 | `source.instance_id` 和 `sink.instance_id` 须在 `instance_model` 或 `instances[]` 中存在 |
| V10.3 | `routing_nodes[].select_register` 须在对应外设的 `registers[]` 中存在（或标注为跨外设引用） |
| V10.4 | `sink.connect_register` 须在 sink 外设的 `registers[]` 中存在 |
| V10.5 | `type=event` 的信号须与 `interrupts[]` 中的对应事件源保持一致（irq_number 不冲突） |
| V10.6 | `config_sequence` 引用的 ID 须在 `atomic_sequences[]` 中存在 |

---

## 11. `shared_resources` — 共享资源与仲裁模型（v4.0 新增）

### 11.1 设计动机

当多个请求方竞争同一硬件单元时（如多个 request source 竞争同一 ADC 核心，多个 DMA 通道竞争总线带宽），需要显式描述：**资源是什么、谁在竞争、按什么规则仲裁、抢占语义如何**。此模型不绑定到任何特定外设类型，适用于所有含仲裁逻辑的外设。

### 11.2 SharedResource 对象字段

```json
{
  "id": "RES_VADC_K0_CONV_CORE",
  "resource_name": "VADC Kernel 0 Conversion Core",
  "resource_instance": "VADC_K0",
  "arbiter": {
    "type": "fixed_priority",
    "config_register": "K0_ARBCFG",
    "config_field": "ASEN",
    "rounds_per_cycle": 8,
    "source": "TC39xB_UM §29.4.2"
  },
  "requestors": [
    {
      "id": "REQ_K0_QUEUE",
      "name": "Queue / Inject Request Source",
      "priority_level": 3,
      "priority_register": "K0_ARBPR",
      "priority_field": "PRIO0",
      "can_preempt_lower": true,
      "abort_behavior": "stop_and_restart",
      "enable_register": "K0_RQCTRL0",
      "enable_field": "ENGT",
      "source": "TC39xB_UM §29.4.3"
    },
    {
      "id": "REQ_K0_SCAN",
      "name": "Group Scan Request Source",
      "priority_level": 1,
      "priority_register": "K0_ARBPR",
      "priority_field": "PRIO1",
      "can_preempt_lower": false,
      "abort_behavior": "complete_then_switch",
      "enable_register": "K0_RSLEN",
      "enable_field": "ENGT",
      "source": "TC39xB_UM §29.4.4"
    }
  ],
  "source": "TC39xB_UM §29.4"
}
```

#### SharedResource 顶层字段

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `id` | string | 是 | 全局唯一 |
| `resource_name` | string | 是 | 可读名称，描述被共享的硬件单元 |
| `resource_instance` | string | 是 | 被共享单元的 `instance_id` |
| `arbiter` | object | 是 | 仲裁器描述 |
| `requestors` | array | 是 | 竞争请求方列表（≥ 2） |
| `source` | string | 是 | 手册引用 |

#### `arbiter` 字段

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `type` | enum | 是 | `fixed_priority` \| `round_robin` \| `weighted` \| `token_ring` \| `fcfs` |
| `config_register` | string | 否 | 配置仲裁行为的寄存器 |
| `config_field` | string | 否 | 仲裁使能/模式位域 |
| `rounds_per_cycle` | integer | 否 | 每个仲裁周期内的仲裁轮数（影响最坏等待时间计算） |
| `source` | string | 是 | 手册引用 |

#### `requestors[]` 字段

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `id` | string | 是 | 请求方唯一标识 |
| `name` | string | 是 | 可读名称 |
| `priority_level` | integer | 否 | 数值越大优先级越高（`fixed_priority` 时必填） |
| `priority_register` | string | 否 | 优先级配置寄存器 |
| `priority_field` | string | 否 | 优先级配置位域 |
| `can_preempt_lower` | boolean | 否 | 是否可打断低优先级正在进行的操作，默认 `false` |
| `abort_behavior` | enum | 否 | `stop_and_restart`：中止当前操作并重新开始；`complete_then_switch`：完成当前后切换；`discard`：丢弃当前请求 |
| `enable_register` | string | 否 | 使能该请求源的寄存器 |
| `enable_field` | string | 否 | 使能位域 |
| `source` | string | 是 | 手册引用 |

### 11.3 code-gen 消费规则

```
对每个 shared_resource：

  1. 生成仲裁器配置函数：
     <Module>_ConfigArbiter_<resource_id>(
         <Module>_ArbConfigType *config)
     内部写 arbiter.config_register.config_field

  2. 对每个 requestor：
     生成请求源配置函数：
       <Module>_ConfigRequestSource_<req_id>(...)
       内部写 priority_register.priority_field（若有）

     生成请求源使能/禁用函数：
       <Module>_EnableRequestSource_<req_id>(instance)
       <Module>_DisableRequestSource_<req_id>(instance)

  3. 若任意 requestor.can_preempt_lower = true：
     在被抢占时的中断/回调中生成 abort 处理逻辑
     按 abort_behavior 生成相应代码：
       stop_and_restart → 检查 active_flag，写 abort 位，等待完成
       complete_then_switch → 轮询完成标志后切换
       discard → 直接覆盖请求寄存器

  4. 生成优先级常量宏：
     #define <MODULE>_<REQ_ID>_PRIORITY  <priority_level>
```

### 11.4 验证规则

| # | 检查项 |
|---|---|
| V11.1 | `resource_instance` 须在 `instance_model` 或 `instances[]` 中存在 |
| V11.2 | `fixed_priority` 类型时，每个 requestor 的 `priority_level` 须填写且无重复 |
| V11.3 | `priority_register` / `enable_register` 须在对应外设的 `registers[]` 中存在 |
| V11.4 | 若 `can_preempt_lower: true`，`abort_behavior` 必须填写 |
| V11.5 | 每个 `resource_instance` 最多被一个 `shared_resource` 描述 |

---

## 12. `structural_relations` — 结构关系模型（v4.0 新增）

### 12.1 设计动机

`signal_graph` 描述"流动什么"，`structural_relations` 描述实例之间"是什么关系"。结构关系是**配置时的逻辑绑定**，而非运行时的信号流：主从同步、互斥独占、资源划分、级联链接等。这些关系影响初始化顺序、API 设计（是否需要 group-level 的聚合函数）和安全约束。

### 12.2 StructuralRelation 对象字段

```json
{
  "id": "REL_VADC_KERNEL_SYNC",
  "type": "master_slave",
  "description": "VADC Kernel 0 acts as sync master, driving kernels 1 and 3 to start conversion simultaneously",
  "participants": [
    {
      "role": "master",
      "instance_id": "VADC_K0",
      "config_register": "K0_SYNCTR",
      "config_field": "STSEL",
      "config_value_for_this_role": 0,
      "source": "TC39xB_UM §29.5.4"
    },
    {
      "role": "slave",
      "instance_id": "VADC_K1",
      "config_register": "K1_SYNCTR",
      "config_field": "STSEL",
      "config_value_for_this_role": 1,
      "source": "TC39xB_UM §29.5.4"
    },
    {
      "role": "slave",
      "instance_id": "VADC_K3",
      "config_register": "K3_SYNCTR",
      "config_field": "STSEL",
      "config_value_for_this_role": 1,
      "source": "TC39xB_UM §29.5.4"
    }
  ],
  "init_order": "master_first",
  "activation_sequence": "SEQ_VADC_SYNC_ACTIVATE",
  "constraint": "All slaves must complete channel configuration before master starts first conversion",
  "source": "TC39xB_UM §29.5"
}
```

#### StructuralRelation 字段

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `id` | string | 是 | 全局唯一 |
| `type` | enum | 是 | 见下表 |
| `description` | string | 否 | 关系的功能描述 |
| `participants` | array | 是 | 参与方列表（≥ 2） |
| `init_order` | enum | 否 | `master_first` \| `slaves_first` \| `any`；默认按 type 推导（见下表） |
| `activation_sequence` | string | 否 | 激活该关系所需的 `atomic_sequences` ID |
| `constraint` | string | 否 | 使该关系正确工作的自然语言约束描述 |
| `source` | string | 是 | 手册引用 |

#### `type` 枚举与默认 `init_order`

| type | 含义 | 默认 init_order | 典型场景 |
|---|---|---|---|
| `master_slave` | 主控一个或多个从属，主的动作驱动从 | `master_first` | ADC 同步、TIM 同步、I2C 主从 |
| `peer` | 平等协作，互相感知但独立运行 | `any` | 多核共享外设、双向通信 |
| `broadcast` | 一个配置源影响所有成员 | `source_first` | 全局寄存器影响所有 kernel |
| `chain` | 顺序级联，前一个输出驱动下一个 | `source_first` | DMA 描述符链、PWM 级联 |
| `mutex` | 互斥使用，运行时只有一个 active | `any` | 引脚复用、总线仲裁 |
| `partition` | 静态资源划分，各自独占一部分 | `any` | Filter bank 分配、内存分区 |

#### `participants[]` 字段

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `role` | enum | 是 | `master` \| `slave` \| `peer` \| `source` \| `sink` \| `member` \| `owner` \| `sharer` |
| `instance_id` | string | 是 | 引用 `instance_model` 或 `instances[]` 中的 id |
| `config_register` | string | 否 | 使该实例承担此 role 所需写的寄存器 |
| `config_field` | string | 否 | 对应位域 |
| `config_value_for_this_role` | any | 否 | 对应值 |
| `source` | string | 否 | 手册引用（精确到寄存器位域说明的章节） |

### 12.3 code-gen 消费规则

```
对每个 structural_relation：

  1. 根据 init_order 确定初始化顺序：
     master_first  → 排序时 role=master 的实例优先于 role=slave
     slaves_first  → 反之
     source_first  → role=source 先于 role=sink
     any           → 不约束顺序

  2. 对每个 participant（按排序后的顺序）：
     若 config_register 和 config_field 均有值：
       生成：<Module>_LL_SetField(<config_register>, <config_field>,
                                   <config_value_for_this_role>)
       注释：/* REL: <relation.id> — role: <participant.role> */

  3. 若 activation_sequence 有值：
     在所有 participant 配置完成后调用对应 atomic_sequence 函数

  4. 根据 type 生成高层 API：
     master_slave →
       <Module>_ConfigSyncMaster(master_instance, slave_list[])
       <Module>_ConfigSyncSlave(slave_instance, master_instance)
     partition →
       <Module>_PartitionResource(owner_instance, range_start, range_end)
     mutex →
       在注释中标注互斥约束，不生成运行时锁（驱动层不负责）

  5. constraint 非空时，在生成的函数头注释中写入：
     /* CONSTRAINT: <constraint> */
```

### 12.4 验证规则

| # | 检查项 |
|---|---|
| V12.1 | 所有 `id` 全局唯一 |
| V12.2 | `participants[].instance_id` 须在 `instance_model` 或 `instances[]` 中存在 |
| V12.3 | `master_slave` 类型须有且仅有一个 `role=master` 的参与方 |
| V12.4 | `participants[].config_register` 须在对应实例的 `register_set_id` 所归属的 `registers[]` 中存在 |
| V12.5 | `activation_sequence` 引用的 ID 须在 `atomic_sequences[]` 中存在 |
| V12.6 | `mutex` 类型时，运行时互斥保护须在代码生成注释中显式声明（驱动不提供锁） |
