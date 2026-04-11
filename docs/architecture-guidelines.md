# 嵌入式驱动开发架构分层规范 (Architecture Layering Guideline) v2.3

本规范定义了 AI 在从"芯片手册 (Specification)"转化为"驱动代码 (Driver Implementation)"时的强制分层结构。严禁 AI 将底层寄存器操作与高层业务逻辑混写在同一个文件中。

---

## 总体文件结构与层次概览

```
┌─────────────────────────────────────────────────────┐
│             *_api.h   （公共接口层）                  │  ← 对外唯一暴露
├─────────────────────────────────────────────────────┤
│         *_drv.c / *_isr.c  （驱动逻辑实现层）         │  ← 状态机 / 数据流
├─────────────────────────────────────────────────────┤
│         *_ll.h / *_ll.c    （硬件抽象底端层）          │  ← 原子级硬件操作
├─────────────────────────────────────────────────────┤
│              *_reg.h       （寄存器映射层）            │  ← 物理地址 / 位域
├─────────────────────────────────────────────────────┤
│   *_types.h（类型定义）  *_cfg.h（编译期配置）         │  ← 跨层共用基础
└─────────────────────────────────────────────────────┘
```

### 合法 `#include` 依赖方向（严格单向，禁止循环依赖）

```
*_drv.c   →  *_api.h, *_ll.h, *_types.h, *_cfg.h
*_ll.h    →  *_reg.h, *_types.h
*_reg.h   →  <stdint.h>（只允许 C 标准库头文件）
*_types.h →  <stdint.h>, <stdbool.h>（只允许 C 标准库）
*_cfg.h   →  （不 include 任何项目头文件）
```

**红线约束：**
- `*_ll.h` **不得** include `*_drv.h`（向上依赖禁止）
- `*_reg.h` **不得** include 任何项目自定义头文件
- `*_api.h` **不得** include `*_ll.h` 或 `*_reg.h`（防止实现细节泄漏到接口层）

**跨模块调用规范（Cross-Module Access）：**

在实际场景中，一个外设驱动的初始化往往需要操作其他模块的硬件资源（如 SPI 需要使能 RCC 时钟、配置 GPIO 复用功能）。此类跨模块操作必须遵守以下规定：

- `*_drv.c` **允许且仅允许** include 其他模块的 `*_api.h`（例如 `port_api.h` 配置引脚、`clock_api.h` 开启外设时钟、或板级 `*_bsp.h`）。
- **严禁** `spi_drv.c` 直接 include `gpio_ll.h` 或 `gpio_reg.h` 越权操作其他模块的底层寄存器。跨模块只能经由对方的公共接口层（`*_api.h`）进行，不得绕过其封装层直接强控。

```
✅ 合法：spi_drv.c  →  gpio_api.h  →  GPIO 模块内部处理
❌ 违规：spi_drv.c  →  gpio_ll.h   （越权，绕过 GPIO 封装）
❌ 违规：spi_drv.c  →  gpio_reg.h  （越权，直接操作 GPIO 寄存器）
```

---

## 基础层定义（所有驱动层共同依赖）

### 0a. 类型定义层 (Type Definition)

- **文件后缀**：`*_types.h`（例如 `can_types.h`）
- **职责**：集中定义跨层共用的数据结构、枚举、错误码，不含任何逻辑代码或寄存器引用。
- **约束**：
  - 只允许 include C 标准库（`<stdint.h>`, `<stdbool.h>`）。
  - 不得包含任何函数声明或实现。
  - **禁止动态内存**：所有结构体成员必须使用静态或栈上分配，禁止包含指向堆内存的指针作为运行时数据容器。
  - 错误码必须使用枚举定义，不得使用裸整数魔法值。

```c
// can_types.h 示例
typedef enum {
    CAN_OK        = 0,
    CAN_ERR_BUSY  = 1,
    CAN_ERR_PARAM = 2,
    CAN_ERR_TIMEOUT = 3,
} Can_ReturnType;

typedef struct {
    uint32_t id;
    uint8_t  dlc;
    uint8_t  data[8];
} Can_PduType;
```

### 0b. 编译期配置层 (Compile-Time Configuration)

- **文件后缀**：`*_cfg.h`（例如 `can_cfg.h`）
- **职责**：定义所有编译期可调参数，由项目配置工具（如 EB Tresos）或手动维护。
- **约束**：
  - 只允许使用 `#define`，不得包含类型定义或函数声明。
  - 不得 include 任何项目头文件。
  - 所有宏命名必须带模块前缀（如 `CAN_CFG_`）。

```c
// can_cfg.h 示例
#define CAN_CFG_CHANNEL_COUNT     (2U)
#define CAN_CFG_TX_FIFO_DEPTH     (8U)
#define CAN_CFG_RX_FIFO_DEPTH     (16U)
#define CAN_CFG_TX_TIMEOUT_MS     (100U)
#define CAN_CFG_ISR_PRIORITY      (5U)
```

---

## 驱动必须严格遵守以下三层架构

### 1. 寄存器映射层 (Register Mapping)

- **文件后缀**：`*_reg.h`（例如 `can_reg.h`）
- **职责**：将外设物理地址、寄存器偏移、位域（Bitfields）使用结构体和宏定义完整映射。
- **约束**：
  - 绝对**不允许**包含任何逻辑代码或 C 函数实现。
  - 寄存器结构体成员必须使用 CMSIS 标准访问宏修饰，以便静态分析工具识别读写属性：
    - `__IO`：可读写寄存器（等价于 `volatile`）
    - `__I`：只读寄存器（等价于 `volatile const`）
    - `__O`：只写寄存器（等价于 `volatile`，读行为未定义）
  - **禁止使用裸 `volatile`** 修饰结构体成员，统一使用 CMSIS 宏。
  - 位段操作使用 `_Pos` / `_Msk` 宏对，禁止在此层外散落魔法数字。
  - 外设基地址宏定义统一在此层管理。

```c
// can_reg.h 示例
#include <stdint.h>

/* 位域宏定义（Pos / Msk 成对出现） */
#define CAN_MCR_INRQ_Pos   (0U)
#define CAN_MCR_INRQ_Msk   (0x1UL << CAN_MCR_INRQ_Pos)

#define CAN_MCR_SLEEP_Pos  (1U)
#define CAN_MCR_SLEEP_Msk  (0x1UL << CAN_MCR_SLEEP_Pos)

#define CAN_MSR_INAK_Pos   (0U)
#define CAN_MSR_INAK_Msk   (0x1UL << CAN_MSR_INAK_Pos)

/* 寄存器结构体（使用 CMSIS 访问宏） */
typedef struct {
    __IO uint32_t MCR;   /* 主控制寄存器         偏移: 0x000 */
    __I  uint32_t MSR;   /* 主状态寄存器（只读） 偏移: 0x004 */
    __IO uint32_t TSR;   /* 发送状态寄存器        偏移: 0x008 */
    __IO uint32_t RF0R;  /* 接收 FIFO0 寄存器    偏移: 0x00C */
    __IO uint32_t IER;   /* 中断使能寄存器        偏移: 0x014 */
    __IO uint32_t ESR;   /* 错误状态寄存器        偏移: 0x018 */
} CAN_TypeDef;

/* 外设基地址（仅此层允许出现物理地址） */
#define CAN1_BASE   (0x40006400UL)
#define CAN1        ((CAN_TypeDef *)CAN1_BASE)
```

---

### 2. 硬件抽象底端层 (Low-Level Hardware Abstraction)

- **文件后缀**：`*_ll.h` / `*_ll.c`（例如 `can_ll.h`）
  - > **命名说明**：统一使用 `_ll` 后缀（Low-Level），代表芯片外设级操作，与具体板卡无关。
  - > `_bsp`（Board Support Package）语义为板级配置（时钟树、引脚复用），属于不同抽象概念，**不得混用**。如项目存在板级配置，应独立为 `*_bsp.h`，不得与 `_ll` 合并。
- **职责**：对上层提供最基础的"原子级硬件操作"，例如"置位标志"、"获取时钟使能状态"、"读写 FIFO"。此层不关心任何协议流程或状态机逻辑。
- **约束**：
  - 推荐使用 `static inline` 实现在头文件中；复杂操作可在 `_ll.c` 中实现。
  - **W1C（写 1 清零）等所有寄存器副作用必须在此层完全封装**，绝不能透出到上层。
  - **内存屏障（Memory Barrier）**：DMA 相关寄存器操作后，必须在此层显式插入 `__DSB()` / `__DMB()`，不得将屏障职责遗漏到 Driver 层。
  - **原子性声明**：所有非原子 RMW（Read-Modify-Write）操作，函数注释中必须标注"调用方须保证临界区"；或提供带 `_Atomic` 后缀的安全封装版本。
  - 入口参数必须做 NULL 指针校验（若涉及指针参数）。

```c
// can_ll.h 示例

#include "can_reg.h"
#include "can_types.h"

/* 进入初始化模式（非原子 RMW，调用方须关中断或加锁） */
static inline void CAN_LL_EnableInit(CAN_TypeDef *CANx)
{
    CANx->MCR |= CAN_MCR_INRQ_Msk;
}

/* 查询初始化应答位（只读，天然原子） */
static inline bool CAN_LL_IsInitAcknowledged(const CAN_TypeDef *CANx)
{
    return ((CANx->MSR & CAN_MSR_INAK_Msk) != 0U);
}

/* 清除 FIFO0 消息挂起标志（W1C，副作用封装在此层） */
static inline void CAN_LL_ClearRxFIFO0MsgPending(CAN_TypeDef *CANx)
{
    /* W1C：写 1 清零，勿用 |= 操作 */
    CANx->RF0R = CAN_RF0R_RFOM0_Msk;
}

/* DMA 触发后同步屏障（此层负责，Driver 层不得遗漏） */
static inline void CAN_LL_DmaSyncBarrier(void)
{
    __DSB();
    __DMB();
}
```

---

### 3. 驱动逻辑实现层 (Driver Implementation & ISR)

- **文件后缀**：`*_drv.c`, `*_isr.c`（例如 `can_drv.c`, `can_isr.c`）
- **职责**：实现状态机控制、数据流处理、缓冲队列、等待超时机制，以及对操作系统的 API 调用。
- **约束**：
  - **红线**：此层**严禁**直接出现十六进制外设物理地址（如 `0x40006400`）和对底层寄存器的直接位运算。
  - 所有硬件操作必须调用第二层（`_ll`）提供的接口，不得跨层调用。
  - **禁止动态内存**：所有缓冲区和数据结构必须静态分配，禁止 `malloc` / `free`。
  - 核心思维是"状态机驱动（State-Driven）"或"事件驱动（Event-Driven）"，使用 `*_types.h` 中定义的标准数据结构传递数据。
  - 所有函数必须有明确错误码返回（使用 `Can_ReturnType` 类枚举），禁止返回裸 `void`（影响功能安全可追溯性）。
  - 入口参数必须做 NULL 指针校验和范围检查（防御性编程）。
  - **临界区纪律（Critical Section Discipline）**：保护非原子 RMW 操作时，必须调用操作系统或调度器提供的受控临界区 API（FreeRTOS 使用 `taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()`；AUTOSAR OS 使用 `SchM_Enter_<Module>_<Resource>()` / `SchM_Exit_...()`）。**严禁**在 Driver 层直接使用全局硬件级关中断指令（`__disable_irq()` / `__enable_irq()`）——此类指令会屏蔽系统全部中断，破坏实时性，只允许在最底层 Kernel/BSP 源码中使用。

```c
// ✅ 合法：使用受控临界区（FreeRTOS 示例）
taskENTER_CRITICAL();
CAN_LL_EnableInit(CAN1);      /* 非原子 RMW */
taskEXIT_CRITICAL();

// ✅ 合法：AUTOSAR OS 示例
SchM_Enter_Can_CAN_EXCLUSIVE_AREA_0();
CAN_LL_EnableInit(CAN1);
SchM_Exit_Can_CAN_EXCLUSIVE_AREA_0();

// ❌ 违规：暴力全局关中断，破坏系统实时性
__disable_irq();
CAN_LL_EnableInit(CAN1);
__enable_irq();
```

#### ISR 职责边界（强制规定，"尽量简短"不是约束）

ISR 函数须严格遵守以下职责边界，每一条均为强制要求：

| 步骤 | 职责 | 说明 |
|:---:|---|---|
| 1 | 通过 LL API 读取并清除硬件中断标志 | W1C 操作必须第一时间完成 |
| 2 | 读取状态/数据寄存器，写入共享数据结构 | 须使用临界区或无锁设计（如环形缓冲） |
| 3 | 发送信号量 / 事件标志给 Task | 使用 OS API（如 `xSemaphoreGiveFromISR`） |
| ❌ | **禁止**在 ISR 中调用任何阻塞 OS API | 不得调用 `osDelay`、`osMutexAcquire` 等 |
| ❌ | **禁止**在 ISR 中直接操作 `_drv` 层状态机 | 状态机推进由 Task 负责，ISR 只做最小化处理 |

```c
// can_isr.c 示例（遵循 ISR 职责边界）
void CAN1_RX0_IRQHandler(void)
{
    /* 步骤 1：通过 LL API 清除中断标志（W1C） */
    CAN_LL_ClearRxFIFO0MsgPending(CAN1);

    /* 步骤 2：读取数据写入环形缓冲（无锁设计，天然线程安全） */
    Can_PduType msg;
    CAN_LL_ReadRxFIFO0(CAN1, &msg);
    RingBuf_Push(&g_can1_rx_buf, &msg);   /* 无锁环形缓冲 */

    /* 步骤 3：通知 Task 处理 */
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(g_can1_rx_sem, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
```

---

### 4. 公共 API 接口层 (Public Interface)

- **文件后缀**：`*_api.h`（例如 `can_api.h`）
- **职责**：对上层（BSW / SWC / Application）暴露唯一稳定接口，与实现细节完全解耦。上层模块**只允许 include 此文件**，不得直接 include `_ll.h` 或 `_reg.h`。
- **约束**：
  - 只声明函数原型，不包含实现。
  - 只 include `*_types.h` 和 `*_cfg.h`，不得 include `*_ll.h` 或 `*_reg.h`。
  - 接口一旦发布，须保持向后兼容，变更需走 ASPICE 变更管理流程。

```c
// can_api.h 示例
#ifndef CAN_API_H
#define CAN_API_H

#include "can_types.h"
#include "can_cfg.h"

Can_ReturnType Can_Init(const Can_ConfigType *Config);
Can_ReturnType Can_DeInit(void);
Can_ReturnType Can_Write(Can_HwHandleType Hth, const Can_PduType *PduInfo);
Can_ReturnType Can_Read(Can_HwHandleType Hrh, Can_PduType *PduInfo);
Can_ReturnType Can_GetStatus(Can_StatusType *Status);

#endif /* CAN_API_H */
```

---

## 功能安全与代码质量约束 (Functional Safety)

> 本节针对 ASIL-B 及以上等级项目为强制要求；ASIL-QM 项目建议遵守。

| 约束类别 | 强制要求 |
|---|---|
| **MISRA-C 合规** | 所有层代码须符合 MISRA-C:2012。Mandatory 规则无条件遵守；Required 规则违例须填写偏差申请（Deviation）并经安全审查；Advisory 规则须在团队内统一约定。 |
| **禁止动态内存** | 全项目禁止使用 `malloc` / `free` / `realloc`。所有缓冲区、队列、状态结构体必须静态分配（`static` 或全局），大小由 `*_cfg.h` 编译期确定。 |
| **错误返回约定** | 每个对外函数必须返回错误码（枚举类型，定义于 `*_types.h`）。禁止返回裸 `void`。调用方必须检查返回值，禁止静默丢弃。 |
| **防御性编程** | 函数入口必须校验指针参数（NULL 检查）和数值参数（范围检查）。第一层（`_reg.h`）无函数，豁免；第二层（`_ll.h`）的 `inline` 函数可豁免（性能敏感，改由调用方保证）；第三层和 API 层必须全面校验。 |
| **禁止递归** | 全项目禁止递归函数调用（影响栈深度分析，WCET 不可确定）。 |
| **可测试性接口** | 每层须预留单元测试桩接口（Testability Hook），方便 SIL/HIL 注入故障；`_ll` 层推荐通过函数指针或条件编译提供 Mock 替换点。 |
| **数据封装** | **严禁**使用 `extern` 关键字在头文件中暴露任何变量。模块内所有状态数据对象（缓冲区数组、状态标志、句柄结构体）必须声明为文件作用域的 `static`，对外不可见。跨模块的数据交换只能通过 `*_api.h` 中声明的 Getter/Setter 函数接口，或通过句柄指针（Handle/Context Pointer）以 OOP 方式传递，不得裸暴露变量地址。 |
| **静态分析** | 代码须通过 MISRA 静态分析工具扫描（如 PC-lint Plus、Polyspace、Helix QAC），并配置与 ASIL 等级匹配的规则集。零未解决的 Mandatory 违例。 |

---

## 目录结构约束 (Directory Layout)

> 文件命名规范必须与物理目录结构配合执行，否则分层约束仅存在于纸面。**严禁将所有模块的所有文件平铺在同一目录下。**

### 标准目录结构

```
project_root/
│
├── chip/                              # ── 芯片级共有文件（见下节详解）
│   ├── Device/                        # 芯片系统级文件（CMSIS Device Layer）
│   │   ├── include/
│   │   │   ├── <chip>.h               # 全芯片外设入口（include 各 *_reg.h）
│   │   │   ├── <chip>_irq.h           # 全芯片 IRQn_Type 枚举
│   │   │   └── system_<chip>.h        # SystemInit / SystemCoreClock 声明
│   │   └── src/
│   │       ├── system_<chip>.c        # SystemInit 实现
│   │       └── startup_<chip>.s       # 向量表 / 堆栈汇编启动文件
│   │
│   ├── Core/                          # ARM CMSIS-Core（只读，来自 ARM 官方）
│   │   └── include/
│   │       ├── cmsis_compiler.h       # __IO / __I / __O / __DSB 等 Core 宏
│   │       ├── core_cm33.h            # （或 core_cr5.h，按芯片内核选择）
│   │       └── ...
│   │
│   ├── Platform/                      # AUTOSAR 平台抽象层（Platform-specific）
│   │   └── include/
│   │       ├── Platform_Types.h       # uint8/sint32 等 AUTOSAR 平台类型
│   │       ├── Compiler.h             # INLINE / STATIC / FUNC() 编译器抽象宏
│   │       └── Std_Types.h            # Std_ReturnType / E_OK / E_NOT_OK
│   │
│   └── Common/                        # 跨模块共用工具（项目内部封装）
│       ├── include/
│       │   ├── drv_assert.h           # 断言宏（DRV_ASSERT / DET_REPORT）
│       │   ├── drv_err.h              # 全项目统一错误码聚合（按模块前缀分段）
│       │   └── ring_buf.h             # 通用无锁环形缓冲接口
│       └── src/
│           ├── ring_buf.c             # 通用无锁环形缓冲实现
│           └── crc.c                  # 共用 CRC-8/16/32 实现
│
├── drivers/                              # ── 各外设驱动模块
│   ├── Can/
│   │   ├── include/
│   │   │   ├── can_reg.h              # 寄存器映射层
│   │   │   ├── can_ll.h               # 硬件抽象底端层
│   │   │   ├── can_types.h            # 类型定义层
│   │   │   ├── can_cfg.h              # 编译期配置层
│   │   │   └── can_api.h              # 公共接口层（唯一对外头文件）
│   │   └── src/
│   │       ├── can_drv.c              # 驱动逻辑实现层
│   │       ├── can_isr.c              # 中断服务函数
│   │       └── can_ll.c               # LL 层非 inline 实现（如有）
│   │
│   ├── Spi/
│   │   ├── include/
│   │   │   ├── spi_reg.h
│   │   │   ├── spi_ll.h
│   │   │   ├── spi_types.h
│   │   │   ├── spi_cfg.h
│   │   │   └── spi_api.h
│   │   └── src/
│   │       ├── spi_drv.c
│   │       └── spi_isr.c
│   │
│   ├── Gpio/
│   ├── Adc/
│   └── ...                            # 其他外设模块（结构一致）
│
├── config/                            # ── 项目级全局配置（非模块级）
│   ├── nvic_priorities.h              # 全局中断优先级分配表（唯一真值源）
│   └── memory_layout.h               # 运行时内存分区常量（栈/堆/共享内存地址）
│
├── linker/                            # ── 链接脚本（分两层管理）
│   ├── memory_tha6xxx.ld              # 芯片存储器映射（来自厂商，只读）
│   └── link_app.ld                    # 项目级链接脚本（INCLUDE memory_tha6xxx.ld）
│
├── BSP/                               # ── 板级支持包（板卡相关，非芯片通用）
│   ├── include/
│   │   └── bsp_board.h                # 板级引脚分配、时钟配置声明
│   └── src/
│       └── bsp_board.c                # 板级初始化实现
│
├── OS/                                # ── 操作系统适配层
│   └── ...
│
└── test/                              # ── 单元测试（与源码目录完全镜像）
    ├── chip/
    │   └── Common/
    │       ├── test_ring_buf.c
    │       └── test_crc.c
    └── drivers/
        ├── Can/
        │   ├── test_can_ll.c
        │   └── test_can_drv.c
        └── Spi/
```

### 芯片共有文件的三类归属与约束

芯片级共有文件按性质归入 `chip/` 下四个子目录，各有独立约束：

#### chip/Device/ — 芯片系统级文件

| 文件 | 职责 | 约束 |
|---|---|---|
| `<chip>.h` | 全芯片唯一入口头文件，汇总所有外设 `*_reg.h` | 只做 include 汇总，禁止新增定义 |
| `<chip>_irq.h` | 全芯片 `IRQn_Type` 枚举，所有 ISR 的中断号来源 | 各模块 `*_isr.c` 必须从此处引用中断号，禁止硬编码数字 |
| `system_<chip>.h/c` | `SystemInit()`、`SystemCoreClock` 全局变量声明与实现 | 仅在系统启动时调用一次，禁止在驱动层直接调用 |
| `startup_<chip>.s` | 向量表、堆栈大小、复位向量 | 向量表中的 ISR 函数名必须与各模块 `*_isr.c` 中定义的弱符号名一致 |

**`<chip>.h` 是寄存器层的唯一聚合点：**

```c
// <chip>.h 内容示例（只做汇总，无新增定义）
#ifndef CHIP_XYZ_H
#define CHIP_XYZ_H

#include "chip/Device/include/<chip>_irq.h"
#include "drivers/Can/include/can_reg.h"
#include "drivers/Spi/include/spi_reg.h"
#include "drivers/Gpio/include/gpio_reg.h"
// ... 各模块 reg.h 汇总

#endif
```

> **注意**：`<chip>.h` 作为汇总入口，其 include 方向是**向下聚合**（唯一合法的跨层引用），但使用方（`*_ll.h`）仍应直接 include 本模块的 `*_reg.h`，不必依赖 `<chip>.h` 入口，防止不必要的编译依赖扩大。

#### chip/Core/ — ARM CMSIS-Core

- **来源**：直接使用 ARM 官方 CMSIS 发布包，**禁止项目内自行修改**。
- **只读约束**：纳入版本管理后，设置为只读目录；任何修改须走 CMSIS 版本升级流程。
- 所有 `__IO`、`__DSB()`、`__disable_irq()` 等 Core 原语必须从此目录引用，禁止在项目内部重复定义。

#### chip/Platform/ — AUTOSAR 平台抽象

- `Platform_Types.h` 和 `Compiler.h` 是 AUTOSAR 规范定义文件，内容须严格符合 AUTOSAR 标准，**不得自行扩展**。
- `Std_Types.h` 中的 `E_OK` / `E_NOT_OK` 是全项目统一返回值基础，各模块 `*_types.h` 中的错误枚举须与之兼容（可扩展，不可矛盾）。
- 所有模块的 `*_types.h` 必须 include `Std_Types.h` 作为基础类型来源，不得自行重新定义 `uint8_t` 等基本类型的别名。

#### chip/Common/ — 跨模块共用工具

- **准入条件**：只有被 **3 个及以上**外设模块使用的工具代码才允许进入 `Common/`。仅被 1~2 个模块使用的工具留在对应模块内部。
- `drv_assert.h`：封装 DET（Default Error Tracer）上报接口，统一断言行为；各模块 `_drv.c` 的防御性编程必须通过此宏实现，不得各自自定义断言。
- `drv_err.h`：按模块前缀分段聚合所有错误码，避免不同模块错误码数值冲突。
- `ring_buf.h/c`：无锁环形缓冲实现，供多个模块 ISR 使用；接口须满足 MISRA-C:2012 且通过独立单元测试覆盖。

### 目录约束规则

**模块独立性：**
- 每个外设模块**必须**独占一个以大写模块名命名的子目录（`Can/`、`Spi/`、`Gpio/` 等）。
- **严禁**将多个模块的文件混放在同一目录（如 `drivers/src/can_drv.c` 与 `drivers/src/spi_drv.c` 并列），这会使模块边界在文件系统层面消失。

**头文件与源文件分离：**
- 每个模块目录内必须有独立的 `include/` 和 `src/` 子目录。
- **严禁** `.h` 与 `.c` 文件混放在同一目录。
- 构建系统（CMake / Make）的 include path 只允许指向 `include/` 目录，不得将 `src/` 加入搜索路径（防止内部实现头文件被外部模块意外引用）。

**跨模块 include 路径约束：**
- 跨模块 include 只能引用对方模块 `include/` 目录下的 `*_api.h`，不得使用相对路径 `../../Can/src/` 等方式绕过构建系统访问其他模块内部文件。
- 推荐在构建系统中为每个模块显式声明其对外暴露的 include 路径（仅 `include/`），内部 `src/` 目录不对外暴露。

**测试目录镜像原则：**
- `test/` 目录结构必须与 `drivers/` 目录结构完全镜像，一一对应，便于 CI 扫描和覆盖率统计。
- 测试文件命名规则：`test_<被测模块文件名>.c`（如 `test_can_ll.c`）。

**违规示例（红线）：**

```
❌ 平铺结构（禁止）：
drivers/
├── can_reg.h
├── can_ll.h
├── can_drv.c
├── spi_reg.h
├── spi_drv.c
└── gpio_ll.h       ← 所有模块混在一起，分层约束形同虚设

❌ 头文件与源文件混放（禁止）：
Can/
├── can_reg.h
├── can_ll.h
├── can_drv.c       ← .h 与 .c 混在同一目录
└── can_isr.c

❌ 跨模块相对路径（禁止）：
// spi_drv.c 中
#include "../../Gpio/include/gpio_ll.h"   ← 越权跨模块访问底层
```

---

## 快速对照表

| 文件 | 层级 | 允许的内容 | 禁止的内容 |
|---|---|---|---|
| `*_reg.h` | 寄存器映射层 | 物理地址宏、寄存器结构体（`__IO`）、位域宏 | 任何函数、逻辑代码 |
| `*_ll.h/c` | 硬件抽象底端层 | `static inline` 原子操作、W1C 封装、内存屏障 | 状态机、协议逻辑、OS API |
| `*_drv.c` | 驱动逻辑实现层 | 状态机、缓冲队列、超时、OS API 调用 | 物理地址、寄存器直接位运算、动态内存 |
| `*_isr.c` | 驱动逻辑实现层 | LL API 调用、无锁缓冲写入、OS 信号量发送 | 阻塞 OS API、状态机直接操作 |
| `*_api.h` | 公共接口层 | 函数原型声明 | 实现代码、`_ll.h`/`_reg.h` include |
| `*_types.h` | 基础类型层 | 枚举、结构体、错误码 | 函数、寄存器引用、动态内存 |
| `*_cfg.h` | 编译期配置层 | `#define` 配置宏 | 类型定义、函数、项目 include |

---

## AI 代码生成常见违规速查表

> 本节汇总 AI 在自动生成驱动代码时最容易犯的错误。`reviewer-agent` 和 `scripts/check-arch.sh` 会自动检查这些项目。所有条目均来自已发生的实际违规案例。

### 硬件操作类违规

| ID | 违规描述 | 为什么危险 | 正确做法 | 自动检查 |
|:--:|---------|-----------|---------|:--------:|
| HW-01 | **W1C 寄存器用 `\|=` 操作** | `\|=` 先读后写，读回的已 pending 标志位会被写回 1，导致意外清除 | 直接赋值 `REG = FLAG_Msk;` | check #4 |
| HW-02 | **`_Pos` 宏误用为位掩码** | `_Pos` 是位偏移量（如 2），`_Msk` 才是掩码（如 `1<<2`），`cr1 \|= XXX_Pos` 会写入错误的位 | 位操作统一用 `_Msk`，移位操作用 `_Pos` | check #5 |
| HW-03 | **空循环延时** | 依赖编译器优化等级，`-O0` 和 `-O2` 下延时差异巨大，不可靠 | 使用硬件定时器或 LL 封装延时 | check #7 |
| HW-04 | **魔法数字操作寄存器位** | `REG \|= (1UL << 11UL)` 可读性差，无法追溯含义 | 使用 `_Msk` 宏并注释手册来源 | 人工审查 |

### 架构分层类违规

| ID | 违规描述 | 为什么危险 | 正确做法 | 自动检查 |
|:--:|---------|-----------|---------|:--------:|
| AR-01 | **Driver 层直接访问寄存器 (`->REG`)** | 绕过 LL 层的 W1C 封装、内存屏障、原子性保护 | 所有硬件操作通过 `_LL_xxx()` 函数 | check #1 |
| AR-02 | **`_api.h` include `_ll.h` 或 `_reg.h`** | 实现细节泄漏到接口层，上层模块可能绕过封装直接操作硬件 | `_api.h` 只 include `_types.h` 和 `_cfg.h` | check #2 |
| AR-03 | **`_ll.h` include `_api.h` 或 `_drv.h`** | 向上依赖，破坏单向依赖链，导致循环依赖 | `_ll.h` 只 include `_reg.h` 和 `_types.h` | check #2 |
| AR-04 | **跨模块直接访问底层** | `spi_drv.c` 直接 include `gpio_ll.h` 操作 GPIO 寄存器 | 跨模块只能通过对方的 `_api.h` | check #2 |
| AR-05 | **缺少 DeInit 函数** | 功能安全要求模块可安全关闭和重启，无 DeInit 则资源无法释放 | 每个模块 Init/DeInit 配对 | check #8 |
| AR-06 | **LL 函数依赖调用者满足硬件前置条件** | LL 层是硬件抽象底端，可能被任何上层路径触达；依赖隐式契约则调用路径上任何一处失守即 UB。例：`SPI_LL_Configure()` 写 BR/CPOL/CPHA 前不清 SPE，若先 Enable 后 Configure 即违反 RM0090 §28.3.3 | IR 中 LOCK 型不变式 (`<guard> implies !writable(REG.FIELD)`) 必须由 code-gen 消费，在 LL 函数入口注入守卫：`scope=always` → 自清 guard 字段；`scope=before_disable` → 入口 wait-for-ready；生成后运行 `check-invariants.py` 自验证 | `scripts/check-invariants.py` |

**关于 AR-06 的设计原则**：LL 函数可能被 `*_drv.c`、ISR、其它模块的调用链触达。若依赖"调用者已进入某硬件状态"的隐式契约，任一失守即 UB。正确姿势按 **(a) `functional_model.invariants[].scope`** 和 **(b) 守卫代码本身是否含 busy-wait** 两个维度决定：

1. **LL 自守 — 单次原子操作**（`scope=always`/`during_init`，守卫是单次清/置位）：LL 函数入口主动清除/判断 guard 字段。例 `SPI_LL_Configure()` 首行 `SPIx->CR1 &= ~SPI_CR1_SPE_Msk;`
2. **Driver 层外守 — 含 busy-wait**（`scope=before_disable`，或守卫需要等待某状态）：**守卫代码必须放在 driver 层**的包装函数（如 `Spi_Disable()` / `Spi_DeInit()`）中，组合 LL 的查询原语（`Xxx_LL_IsBusy`、`Xxx_LL_IsTxEmpty`）实现等待循环，再调用 `Xxx_LL_Disable`。**严禁在 `*_ll.h` 的 `static inline` 函数中出现 `while` 等待循环**——这违反 LL 层"原子操作 only"的职责边界（line 501 规范）
3. **显式契约 + 断言**（少数 `scope=during_init` 场景）：函数文档标注前置条件，debug 版本加 `assert(...)`，reviewer-agent 验证所有调用点均在守卫之后
4. **命名标注**：无法自守且无 driver 包装的函数加后缀 `_Unsafe`（如 `SPI_LL_ConfigureUnsafe`），强制调用者显式承担责任

**决策表**（code-gen 生成时必须遵循）：

| invariant.scope | 守卫是否含 busy-wait | 采用姿势 | 注入层 |
|---|:--:|---|---|
| `always` | 否 | #1 LL 自守 | `_ll.h` |
| `during_init` | 否 | #1 LL 自守 或 #3 契约+断言 | `_ll.h` / `_drv.c` 初始化序列 |
| `before_disable` | **是** | **#2 Driver 外守** | **`_drv.c`** |
| `before_disable` | 否（罕见） | #1 LL 自守 | `_ll.h` |
| 任意（复杂依赖） | 是 | #4 `_Unsafe` 后缀 + 文档 | `_ll.h` + 调用点校验 |

### 编码规范类违规

| ID | 违规描述 | 为什么危险 | 正确做法 | 自动检查 |
|:--:|---------|-----------|---------|:--------:|
| CS-01 | **使用裸 `int`/`short`/`long`** | 不同平台宽度不同（ILP32 vs LP64），行为不可预测 | `<stdint.h>` 定长类型（`uint32_t`） | check #3 |
| CS-02 | **`(void*)0` 替代 `NULL`** | 编码规范一致性，隐式类型转换风险 | 统一使用 `NULL` 宏 | check #6 |
| CS-03 | **寄存器操作无手册来源注释** | 无法追溯配置依据，维护时无法判断是否正确 | `/* RM0090 §32.7.7 */` | 人工审查 |
| CS-04 | **全局变量缺少 `g_` 前缀** | 命名规范不一致，难以区分局部/全局作用域 | `g_can_txCount` | 人工审查 |

### 功能安全类违规

| ID | 违规描述 | 为什么危险 | 正确做法 | 自动检查 |
|:--:|---------|-----------|---------|:--------:|
| FS-01 | **ISR 中调用阻塞 OS API** | 导致死锁或 WCET 不可确定 | ISR 只用 `FromISR` 变体 | 人工审查 |
| FS-02 | **Driver 层用 `__disable_irq()` 关全局中断** | 破坏系统实时性，屏蔽所有中断 | 使用 OS 临界区 API | 人工审查 |
| FS-03 | **函数返回裸 `void`（公共 API）** | 调用方无法判断操作是否成功，影响安全可追溯性 | 返回错误码枚举 | 人工审查 |
| FS-04 | **公共 API 缺少 NULL 指针校验** | 空指针解引用导致 HardFault | 函数入口 `if (ptr == NULL) return ERR` | 人工审查 |