# 外设中间表示规范 (Peripheral IR Specification)

**版本**: 2.0
**用途**: 定义 `doc-analyst` Agent 输出的结构化外设描述格式。**IR JSON 是唯一机器可消费真值源。**
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
   │  _ir.json    │
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

- **存放位置**: `docs/<module>_ir.json`（如 `docs/can_ir.json`、`docs/spi_ir.json`）
- **编码**: UTF-8
- **格式**: JSON（可通过 `jq .` 验证语法）
- **生命周期**: doc-analyst 生成 → 人工审核确认 → code-gen 消费 → 随手册版本更新

---

## 3. 顶层结构

```json
{
  "ir_version": "2.0",
  "peripheral": {
    "name": "CAN",
    "full_name": "Controller Area Network",
    "chip_family": "STM32F4",
    "chip_model": "STM32F407VGT6",
    "source_document": "RM0090 Rev 19",
    "source_sections": ["32.7 bxCAN registers", "32.4 Operating modes"],

    "instances": [ ... ],
    "registers": [ ... ],
    "interrupts": [ ... ],
    "clock": [ ... ],
    "dma_channels": [ ... ],
    "gpio_config": [ ... ],
    "atomic_sequences": [ ... ],
    "errors": [ ... ],
    "functional_model": { ... },
    "errata": [ ... ],

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

    "bitfields": [
      {
        "name": "FMP0",
        "bits": "1:0",
        "bit_offset": 0,
        "bit_width": 2,
        "access": "RO",
        "reset_value": 0,
        "description": "FIFO 0 message pending (read-only count)",
        "source": "RM0090 §32.9.4"
      },
      {
        "name": "FULL0",
        "bits": "3",
        "bit_offset": 3,
        "bit_width": 1,
        "access": "W1C",
        "reset_value": 0,
        "description": "FIFO 0 full. Set by hardware, cleared by writing 1.",
        "source": "RM0090 §32.9.4"
      },
      {
        "name": "FOVR0",
        "bits": "4",
        "bit_offset": 4,
        "bit_width": 1,
        "access": "W1C",
        "reset_value": 0,
        "description": "FIFO 0 overrun. Set by hardware, cleared by writing 1.",
        "source": "RM0090 §32.9.4"
      },
      {
        "name": "RFOM0",
        "bits": "5",
        "bit_offset": 5,
        "bit_width": 1,
        "access": "W1C",
        "reset_value": 0,
        "description": "Release FIFO 0 output mailbox. Set by software to release.",
        "source": "RM0090 §32.9.4"
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
    "source": "RM0090 SS7.3.14",
    "confidence": 0.95
  }
]
```

**必填字段**：`instance`, `bus`, `enable_register`, `enable_bit`, `source`
**可选字段**：`reset_register`, `reset_bit`, `max_frequency_mhz`, `clock_source_mux`, `confidence`

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
      "source": "RM0090 SS32.4.1"
    },
    {
      "order": 2,
      "description": "Request initialization mode",
      "register": "MCR",
      "field": "INRQ",
      "value": "1",
      "layer": "ll",
      "source": "RM0090 SS32.4.1"
    },
    {
      "order": 3,
      "description": "Wait for INAK acknowledgement",
      "register": "MSR",
      "field": "INAK",
      "condition": "== 1",
      "timeout_required": true,
      "layer": "drv",
      "source": "RM0090 SS32.4.1"
    },
    {
      "order": 4,
      "description": "Configure bit timing (BRP, TS1, TS2, SJW)",
      "register": "BTR",
      "layer": "ll",
      "source": "RM0090 SS32.7.7"
    },
    {
      "order": 5,
      "description": "Leave initialization mode",
      "register": "MCR",
      "field": "INRQ",
      "value": "0",
      "layer": "ll",
      "source": "RM0090 SS32.4.1"
    }
  ],

  "deinit_sequence": [
    {
      "order": 1,
      "description": "Enter initialization mode to stop all bus activity",
      "register": "MCR",
      "field": "INRQ",
      "value": "1"
    },
    {
      "order": 2,
      "description": "Disable peripheral clock",
      "register": "RCC_APB1ENR",
      "field": "CAN1EN",
      "value": "0"
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
expr      := term (logic_op term)*
term      := atom | "!" atom | "(" expr ")"
atom      := field_ref cmp_op value
           | "writable(" field_ref ")"
           | "readable(" field_ref ")"
field_ref := REG_NAME "." FIELD_NAME
cmp_op    := "==" | "!=" | ">" | "<" | ">=" | "<="
logic_op  := "&&" | "||" | "implies"
value     := INT | HEX | field_ref
```

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
    "source": "RM0090 §28.3.3",
    "confidence": 0.98
  },
  {
    "id": "INV_SPI_002",
    "expr": "CR1.SPE==1 implies !writable(CR1.BR) && !writable(CR1.CPOL) && !writable(CR1.CPHA)",
    "description": "BR/CPOL/CPHA 必须在 SPE=0 时配置，启用后改这些位行为未定义",
    "scope": "always",
    "violation_effect": "未定义行为（手册：not allowed）",
    "enforced_by": ["code-gen:precondition", "reviewer-agent:static"],
    "source": "RM0090 §28.3.3",
    "confidence": 0.99
  },
  {
    "id": "INV_SPI_003",
    "expr": "SR.BSY==1 implies !writable(CR1.SPE)",
    "description": "禁用 SPI 必须先等 BSY=0 且 TXE=1，否则可能丢失正在传输的帧",
    "scope": "before_disable",
    "violation_effect": "最后一帧数据可能损坏或丢失",
    "enforced_by": ["code-gen:precondition"],
    "source": "RM0090 §28.3.8",
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
    "id": "ES0182_2.1.8",
    "title": "Limitation on I2C N=2 byte reception",
    "affected_registers": ["CR1", "SR1"],
    "workaround": "Must set POS bit before clearing ADDR for 2-byte reception",
    "impact_on_code": "I2C_MasterReceive must special-case Size==2",
    "source": "ES0182 Rev 13, §2.1.8",
    "confidence": 0.95
  }
]
### 4.8 atomic_sequences — 原子操作序列

当标志位的清除方式不是简单的读或写（如 SPI OVR 需要"先读 DR 再读 SR"），必须在 `atomic_sequences` 中定义精确的操作步骤，并在相应 bitfield 中通过 `clear_sequence` 引用：

```json
"atomic_sequences": [
  {
    "id": "SEQ_OVR_CLEAR",
    "name": "Overrun Clear Sequence",
    "source": "RM0090 SS28.3.10",
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

### 4.12 generation_metadata — 生成元信息

```json
"generation_metadata": {
  "generated_by": "doc-analyst",
  "generation_date": "2026-04-11",
  "source_documents": [
    {"name": "RM0090", "version": "Rev 19", "path": "docs/stm32f103_reference_manual.pdf"},
    {"name": "ES0182", "version": "Rev 13", "path": "docs/stm32f4_errata.pdf"}
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

```
1. 读取芯片手册（PDF -> 文本/表格提取，或 SVD -> 直接解析）
2. 逐个寄存器提取：name, offset, width, access, reset_value, bitfields
3. 对每个 bitfield 确定 access type
   - 必须忠实反映手册标注（如 rc_w0 -> W0C, rc_w1 -> W1C）
   - 重点关注 W1C、W0C、RC、RC_SEQ、WO_TRIGGER 等特殊类型
   - 多步清除的标志位必须定义 atomic_sequence 并在 bitfield 中通过 clear_sequence 引用
4. 提取功能描述：init_sequence（每步含 layer 标注）, operation_modes, error_handling
5. 跨章节交叉提取：RCC 时钟(clock[])、GPIO 复用(gpio_config[])、NVIC 中断(interrupts[])、DMA 通道(dma_channels[])
6. 检查勘误表（若 docs/ 下存在 errata 文件）
7. 对 confidence < 0.85 的字段，创建 pending_review
8. 输出 IR JSON 到 docs/<module>_ir.json（唯一机器消费格式，遵循 SS3 顶层结构）
9. 运行自验证：
   - 寄存器偏移无重叠
   - 位域无重叠且不超出寄存器宽度
   - 所有字段都有 source 引用
   - 所有 RC_SEQ 位域的 clear_sequence 引用存在于 atomic_sequences[]
   - clock[] 与 instances[] 个数匹配
   - JSON 语法合法（jq 验证）
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
| `functional_model.init_sequence` | `Xxx_Init()` 函数体中的操作顺序。按 `layer` 字段决定：`ll` -> 调用 LL 函数，`drv` -> 在 drv 层组装逻辑（含等待循环、跨模块调用） |
| `functional_model.deinit_sequence` | `Xxx_DeInit()` 函数体 |
| `functional_model.operation_modes` | 模式枚举和配置逻辑 |
| `errors[]` | 错误处理函数，按 `recovery_sequence` 引用调用对应 LL 封装 |
| `errata[].workaround` | 在对应代码位置插入 workaround 并注释 errata ID |

---

## 7. 验证清单

doc-analyst 输出 IR 后，必须通过以下自验证：

| # | 检查项 | 方法 |
|---|--------|------|
| 1 | JSON 语法合法 | `jq . docs/<module>_ir.json` |
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
