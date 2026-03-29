# 嵌入式C编码规范

**版本**: 1.0
**适用平台**: ARM Cortex-M / Cortex-R52
**语言标准**: C99（允许 C11 `_Static_assert`、`_Noreturn`）
**基线标准**: MISRA C:2012
**功能安全**: ISO 26262 / IEC 61508

---

## 目录

1. [总则](#1-总则)
2. [命名规范](#2-命名规范)
3. [类型与内存安全](#3-类型与内存安全)
4. [函数设计](#4-函数设计)
5. [中断与并发](#5-中断与并发)
6. [寄存器与硬件抽象](#6-寄存器与硬件抽象)
7. [预处理器](#7-预处理器)
8. [错误处理](#8-错误处理)
9. [文件组织与模块化](#9-文件组织与模块化)
10. [编译与静态分析](#10-编译与静态分析)
11. [文档与注释](#11-文档与注释)

---

## 1. 总则

### 1.1 适用范围

本规范适用于以下硬件平台上的所有C语言嵌入式软件开发：

- ARM Cortex-M 系列（M0/M0+/M3/M4/M7/M33）
- ARM Cortex-R52（双核锁步/分离模式）
- 语言标准：C99 为主，允许使用 C11 的 `_Static_assert` 和 `_Noreturn`
- 编译器：GCC ARM、ARM Compiler 6 (armclang)、IAR EWARM

### 1.2 与 MISRA C:2012 的关系

本规范以 MISRA C:2012（含 Amendment 1/2/3）为基线。所有 MISRA 规则默认采纳，以下为明确偏离说明：

| MISRA 规则 | 级别 | 本规范态度 | 说明 |
|-----------|------|-----------|------|
| Dir 4.6 | Advisory | **采纳并升级为 Required** | 必须使用 `stdint.h` 固定宽度类型 |
| Dir 4.12 | Required | **采纳** | 禁止动态内存分配（ASIL-B 及以上） |
| Rule 8.13 | Advisory | **采纳** | 只读指针参数必须声明为 `const` |
| Rule 11.5 | Advisory | **采纳并升级为 Required** | 禁止 `void*` 到具体类型的隐式转换 |
| Rule 14.3 | Required | **偏离** | 允许防御性编程中的不可达检查（需文档化） |
| Rule 15.5 | Advisory | **采纳** | 功能安全代码优先单一出口 |
| Rule 17.2 | Required | **采纳** | 禁止递归 |
| Rule 21.3 | Required | **采纳** | 禁止 `malloc/free/calloc/realloc` |
| Rule 21.6 | Required | **采纳** | 禁止标准 I/O 库 |

### 1.3 功能安全等级与规范强度

| 安全等级 | 对应标准 | 规范强度 | 说明 |
|---------|---------|---------|------|
| QM (非安全) | — | Advisory 规则可偏离 | 需记录偏离理由 |
| ASIL-A / SIL-1 | ISO 26262 / IEC 61508 | Required 规则全部遵守 | 偏离需安全评审批准 |
| ASIL-B/C | ISO 26262 | 全部 Mandatory + Required | 静态分析零告警 |
| ASIL-D / SIL-3/4 | ISO 26262 / IEC 61508 | 全规范严格执行 | 独立工具验证，形式化方法辅助 |

**偏离流程**: 任何规则偏离必须填写《规则偏离申请表》，记录偏离理由、风险评估、替代措施，并经安全负责人签字。

---

## 2. 命名规范

### 2.1 模块前缀策略

采用分层前缀，支持多产品线共用代码库：

```
<产品/平台前缀>_<模块前缀>_<标识符>
```

| 层级 | 前缀示例 | 说明 |
|------|---------|------|
| 平台公共层 | `hal_`, `bsp_`, `drv_` | 硬件抽象、板级支持、驱动 |
| 中间件层 | `mid_`, `com_`, `diag_` | 通信栈、诊断服务 |
| 应用层 | `app_` | 应用逻辑 |
| 产品特定 | `prd_a_`, `prd_b_` | 特定产品线代码 |

### 2.2 命名模式

| 类别 | 模式 | 示例 |
|------|------|------|
| 类型 (`typedef struct`) | `<Prefix>_<PascalCase>_t` | `hal_GpioConfig_t` |
| 枚举类型 | `<Prefix>_<PascalCase>_e` | `drv_SpiMode_e` |
| 枚举值 | `<PREFIX>_<UPPER_SNAKE>` | `DRV_SPI_MODE_0` |
| 函数 | `<Prefix>_<PascalCase>` | `hal_Gpio_Init()` |
| 局部变量 | `camelCase` | `byteCount` |
| 全局变量 | `g_<prefix>_<camelCase>` | `g_drv_rxBuffer` |
| 静态模块变量 | `s_<camelCase>` | `s_initialized` |
| 宏 / 常量 | `<PREFIX>_<UPPER_SNAKE>` | `HAL_GPIO_PIN_MAX` |
| 函数式宏 | `<PREFIX>_<UPPER_SNAKE>(x)` | `DRV_REG_READ(addr)` |
| 布尔变量 | `is/has/can + PascalCase` | `isReady`, `hasError` |

### 2.3 硬件寄存器命名

```c
/* 寄存器基地址 */
#define UART0_BASE_ADDR          ((uint32_t)0x40004000U)

/* 寄存器偏移 */
#define UART_CR_OFFSET           ((uint32_t)0x00U)
#define UART_SR_OFFSET           ((uint32_t)0x04U)

/* 寄存器指针（volatile） */
#define UART0_CR                 (*(volatile uint32_t *)(UART0_BASE_ADDR + UART_CR_OFFSET))

/* 位域定义 */
#define UART_CR_TXEN_POS         ((uint32_t)0U)
#define UART_CR_TXEN_MSK         ((uint32_t)(1U << UART_CR_TXEN_POS))
#define UART_CR_BAUD_POS         ((uint32_t)8U)
#define UART_CR_BAUD_MSK         ((uint32_t)(0xFFU << UART_CR_BAUD_POS))
```

### 2.4 禁止事项

- 禁止单字母变量名（循环索引 `i`, `j`, `k` 除外，且循环体不超过10行）
- 禁止匈牙利记号（类型编码前缀如 `dwValue`）
- 禁止下划线开头的标识符（保留给编译器和标准库）
- 禁止仅靠大小写区分的标识符对

---

## 3. 类型与内存安全

> **本章为核心章节**，包含最高优先级的规则。

### 3.1 固定宽度整型（Mandatory）

**必须使用 `<stdint.h>` 定义的固定宽度类型**，禁止使用 `int`、`short`、`long`、`char`（仅字符串操作可用 `char`）。

```c
/* CORRECT */
uint8_t  sensorId;
int16_t  temperature;      /* 0.1°C 精度 */
uint32_t timestamp_ms;
int32_t  motorPosition;

/* WRONG */
int sensorId;
short temperature;
unsigned long timestamp;
```

布尔类型统一使用 `<stdbool.h>` 的 `bool`/`true`/`false`。

### 3.2 指针规则（Mandatory）

| 规则 | 说明 |
|------|------|
| P-01 | 指针声明时必须初始化（`= NULL` 或有效地址） |
| P-02 | 指针使用前必须判空 |
| P-03 | 释放/失效后立即置 `NULL` |
| P-04 | 禁止指针算术运算（数组遍历除外，需边界检查） |
| P-05 | `void*` 仅允许在通用接口层使用，调用端必须显式类型转换并注释 |
| P-06 | 函数指针必须通过 `typedef` 定义，使用前必须判空 |
| P-07 | 禁止从函数返回局部变量的地址 |

```c
/* P-01: 初始化 */
uint8_t *pBuffer = NULL;

/* P-02: 使用前判空 */
if (NULL != pBuffer)
{
    pBuffer[0] = 0U;
}

/* P-06: 函数指针 typedef */
typedef void (*hal_IsrCallback_t)(void);

static hal_IsrCallback_t s_callback = NULL;

void hal_Timer_SetCallback(hal_IsrCallback_t callback)
{
    s_callback = callback;  /* 允许 NULL 表示取消回调 */
}
```

### 3.3 数组边界检查（Required）

```c
/* 数组访问必须有边界保护 */
#define SENSOR_BUF_SIZE  ((uint32_t)64U)
static int16_t s_sensorBuf[SENSOR_BUF_SIZE];

bool drv_Sensor_Write(uint32_t index, int16_t value)
{
    bool result = false;

    if (index < SENSOR_BUF_SIZE)
    {
        s_sensorBuf[index] = value;
        result = true;
    }

    return result;
}
```

- 数组大小必须用 `#define` 或 `enum` 定义的常量表示
- 禁止可变长数组（VLA）
- `memcpy`/`memset` 等必须使用 `sizeof` 或已知常量作为长度参数

### 3.4 栈使用限制

| 规则 | 说明 |
|------|------|
| S-01 | 每个任务/线程的栈大小必须在设计阶段确定并文档化 |
| S-02 | 禁止在函数内定义超过 256 字节的局部数组（使用 `static` 或模块级分配） |
| S-03 | 使用静态分析工具计算最大栈深度（配合调用图分析） |
| S-04 | 实际栈使用不超过分配量的 75%（预留 25% 余量） |
| S-05 | ISR 栈与任务栈分离（Cortex-M 使用 MSP/PSP 双栈） |

### 3.5 动态内存分配策略

| 安全等级 | 策略 |
|---------|------|
| ASIL-B 及以上 | **完全禁止** `malloc/free` 及任何堆操作 |
| ASIL-A / QM | 仅允许在初始化阶段分配（启动后禁止），或使用静态内存池 |

**静态内存池模式**（适用于需要"动态"行为的场景）：

```c
/* 编译期分配的内存池 */
#define POOL_BLOCK_SIZE   ((uint32_t)64U)
#define POOL_BLOCK_COUNT  ((uint32_t)32U)

typedef struct
{
    uint8_t  data[POOL_BLOCK_SIZE];
    bool     inUse;
} mid_PoolBlock_t;

static mid_PoolBlock_t s_pool[POOL_BLOCK_COUNT];

/* 从池中分配 */
void *mid_Pool_Alloc(void)
{
    void *pBlock = NULL;
    uint32_t i;

    for (i = 0U; i < POOL_BLOCK_COUNT; i++)
    {
        if (!s_pool[i].inUse)
        {
            s_pool[i].inUse = true;
            pBlock = s_pool[i].data;
            break;
        }
    }

    return pBlock;
}
```

### 3.6 volatile 使用规则（Required）

`volatile` 必须用于以下场景，**其它场景禁止使用**：

| 场景 | 示例 |
|------|------|
| 内存映射寄存器 | `volatile uint32_t *pReg` |
| ISR 与主循环共享变量 | `static volatile bool s_dataReady` |
| 多核共享内存 | `volatile` + 内存屏障 |
| DMA 缓冲区 | DMA 目标/源 buffer |

```c
/* ISR 共享变量 */
static volatile uint32_t s_tickCount = 0U;

/* ISR 中写 */
void SysTick_Handler(void)
{
    s_tickCount++;
}

/* 主循环中读 */
uint32_t hal_GetTick(void)
{
    return s_tickCount;
}
```

**注意**：`volatile` 不提供原子性保证。多字节共享变量的读写必须配合临界区保护（见第5章）。

---

## 4. 函数设计

### 4.1 单一出口原则（Required for ASIL-B+, Advisory for QM/ASIL-A）

功能安全代码优先采用单一出口模式，便于追踪执行路径和资源清理：

```c
/* PREFERRED: 单一出口 */
hal_Status_e hal_Spi_Transfer(const uint8_t *pTxBuf, uint8_t *pRxBuf, uint32_t length)
{
    hal_Status_e status = HAL_STATUS_ERROR;

    if ((NULL != pTxBuf) && (NULL != pRxBuf) && (length > 0U) && (length <= SPI_MAX_LEN))
    {
        if (s_spiReady)
        {
            /* ... 传输逻辑 ... */
            status = HAL_STATUS_OK;
        }
        else
        {
            status = HAL_STATUS_BUSY;
        }
    }
    else
    {
        status = HAL_STATUS_INVALID_PARAM;
    }

    return status;
}
```

QM/ASIL-A 级别允许使用早返回，但需满足：
- 仅在参数校验阶段使用
- 函数无需资源清理
- 记录偏离理由

### 4.2 参数校验（Required）

所有公共接口函数（`extern` 函数）必须校验参数：

```c
hal_Status_e hal_Uart_Send(uint8_t port, const uint8_t *pData, uint32_t length)
{
    hal_Status_e status = HAL_STATUS_INVALID_PARAM;

    /* 防御性参数校验 */
    if ((port < HAL_UART_PORT_MAX) &&
        (NULL != pData) &&
        (length > 0U) &&
        (length <= HAL_UART_MAX_TX_LEN))
    {
        /* 执行逻辑 */
        status = drv_Uart_Transmit(port, pData, length);
    }

    return status;
}
```

模块内部的 `static` 函数可以使用断言替代运行时检查（见 8.2 节）。

### 4.3 函数复杂度限制

| 指标 | 限制 | 说明 |
|------|------|------|
| 函数体行数 | ≤ 75 行（不含注释和空行） | 超过需拆分 |
| 圈复杂度 | ≤ 15 | 使用静态分析工具检测 |
| 参数个数 | ≤ 6 | 超过需使用结构体封装 |
| 嵌套深度 | ≤ 4 | `if`/`for`/`while`/`switch` 嵌套 |
| 局部变量 | ≤ 10 | 函数作用域内的变量 |

### 4.4 递归禁止（Mandatory）

**严格禁止所有形式的递归**，包括直接递归和间接递归。

理由：
- 栈消耗无法静态分析
- 可能导致栈溢出
- 违反 MISRA C:2012 Rule 17.2

所有递归算法必须改写为迭代版本。

---

## 5. 中断与并发

### 5.1 ISR 编写规则（Mandatory）

| 规则 | 说明 |
|------|------|
| I-01 | ISR 必须尽可能短（目标：< 10μs，硬限制：< 50μs） |
| I-02 | ISR 中禁止调用阻塞函数（延时、等待信号量等） |
| I-03 | ISR 中禁止动态内存操作 |
| I-04 | ISR 中禁止使用浮点运算（除非硬件 FPU 且保存了 FP 上下文） |
| I-05 | ISR 共享变量必须声明为 `volatile` |
| I-06 | ISR 中仅做标志设置或缓冲区写入，逻辑处理延迟到主循环/任务 |
| I-07 | ISR 退出前必须清除中断标志位 |

```c
/* 典型 ISR 模式：最小化处理 */
static volatile bool s_rxFlag = false;
static volatile uint8_t s_rxData = 0U;

void UART0_IRQHandler(void)
{
    if (UART0->SR & UART_SR_RXNE_MSK)
    {
        s_rxData = (uint8_t)(UART0->DR & 0xFFU);
        s_rxFlag = true;
        UART0->SR &= ~UART_SR_RXNE_MSK;  /* 清除中断标志 */
    }
}
```

### 5.2 临界区保护模式

```c
/* Cortex-M 临界区：基于 PRIMASK */
#define CRITICAL_SECTION_ENTER()  \
    do {                          \
        uint32_t _primask = __get_PRIMASK(); \
        __disable_irq()

#define CRITICAL_SECTION_EXIT()   \
        __set_PRIMASK(_primask);  \
    } while (0)

/* 使用示例 */
void mid_Queue_Push(mid_Queue_t *pQueue, uint8_t data)
{
    CRITICAL_SECTION_ENTER();
    {
        if (pQueue->count < pQueue->capacity)
        {
            pQueue->buffer[pQueue->head] = data;
            pQueue->head = (pQueue->head + 1U) % pQueue->capacity;
            pQueue->count++;
        }
    }
    CRITICAL_SECTION_EXIT();
}
```

**临界区使用原则**：
- 临界区内代码必须尽量短（目标 < 20 条指令）
- 禁止在临界区内调用可能阻塞的函数
- 临界区必须成对使用（ENTER/EXIT）
- 支持嵌套（保存/恢复 PRIMASK）

### 5.3 volatile 与内存屏障

```c
/* 仅 volatile 不够：多核或 DMA 场景需要内存屏障 */

/* 写屏障：确保之前的写操作完成后才执行后续操作 */
__DSB();  /* Data Synchronization Barrier */
__ISB();  /* Instruction Synchronization Barrier */

/* DMA 传输前确保数据已写入内存 */
void drv_Dma_Start(const uint8_t *pSrc, uint32_t length)
{
    /* 确保所有数据写入完成 */
    __DSB();

    DMA->SRC_ADDR = (uint32_t)pSrc;
    DMA->LENGTH   = length;

    __DSB();
    DMA->CR |= DMA_CR_START_MSK;
}
```

### 5.4 Cortex-M NVIC 优先级规范

| 规则 | 说明 |
|------|------|
| N-01 | 采用4-bit抢占优先级分组（`NVIC_SetPriorityGrouping(3)`） |
| N-02 | 优先级 0-1：保留给安全关键中断（看门狗、故障检测） |
| N-03 | 优先级 2-3：时间关键外设（高速通信、电机控制 PWM） |
| N-04 | 优先级 4-7：普通外设（UART、SPI、I2C） |
| N-05 | 优先级 8-15：低优先级任务（定时器、后台处理） |
| N-06 | SysTick 固定使用最低抢占优先级 |
| N-07 | 使用 RTOS 时，`configMAX_SYSCALL_INTERRUPT_PRIORITY` 以下的中断禁止调用 RTOS API |

```c
/* 优先级配置示例 */
#define IRQ_PRIO_SAFETY_WDG      ((uint32_t)0U)   /* 最高 - 安全关键 */
#define IRQ_PRIO_MOTOR_PWM       ((uint32_t)2U)   /* 高 - 时间关键 */
#define IRQ_PRIO_CAN_RX          ((uint32_t)4U)   /* 中 - 通信 */
#define IRQ_PRIO_UART_RX         ((uint32_t)6U)   /* 中 - 调试 */
#define IRQ_PRIO_SYSTICK         ((uint32_t)15U)  /* 最低 */
```

### 5.5 Cortex-R52 特定规范

#### MPU 配置

| 规则 | 说明 |
|------|------|
| R-01 | 必须启用 MPU，默认背景区域设为不可访问 |
| R-02 | 代码区设为只读/可执行（XN=0），数据区设为不可执行（XN=1） |
| R-03 | 外设区必须设为 Device 内存类型（禁止推测访问） |
| R-04 | 每个 RTOS 任务配置独立的 MPU 区域 |

#### 双核操作

| 规则 | 说明 |
|------|------|
| R-05 | 锁步模式：启用比较逻辑，配置 TCMR 错误处理 |
| R-06 | 分离模式：核间通信使用硬件信号量 + 共享内存 |
| R-07 | 共享内存区域设为 Non-cacheable 或使用缓存维护操作 |
| R-08 | 核间共享数据结构必须自然对齐，使用内存屏障 |

```c
/* Cortex-R52 核间共享数据模板 */
typedef struct __attribute__((aligned(32)))
{
    volatile uint32_t sequence;    /* 序列号，用于检测更新 */
    volatile uint32_t data[8];     /* 共享数据 */
    volatile uint32_t crc;         /* 完整性校验 */
} ipc_SharedBlock_t;

/* 写入共享数据（Core 0） */
void ipc_Write(ipc_SharedBlock_t *pBlock, const uint32_t *pData, uint32_t count)
{
    pBlock->sequence++;
    __DSB();

    for (uint32_t i = 0U; i < count; i++)
    {
        pBlock->data[i] = pData[i];
    }

    pBlock->crc = ipc_CalcCrc(pData, count);
    __DSB();
    pBlock->sequence++;
    __DSB();
}
```

---

## 6. 寄存器与硬件抽象

### 6.1 寄存器访问模式

提供两种访问模式，按项目选择统一使用：

**模式A：宏定义（轻量级项目）**

```c
/* 读寄存器 */
#define REG_READ(reg)            (*(volatile uint32_t *)(reg))

/* 写寄存器 */
#define REG_WRITE(reg, val)      (*(volatile uint32_t *)(reg) = (uint32_t)(val))

/* 位操作：置位 */
#define REG_SET_BIT(reg, msk)    (*(volatile uint32_t *)(reg) |= (uint32_t)(msk))

/* 位操作：清位 */
#define REG_CLR_BIT(reg, msk)    (*(volatile uint32_t *)(reg) &= ~(uint32_t)(msk))

/* 位域写入 */
#define REG_WRITE_FIELD(reg, msk, pos, val) \
    do { \
        uint32_t _tmp = REG_READ(reg);      \
        _tmp &= ~(uint32_t)(msk);           \
        _tmp |= (((uint32_t)(val)) << (pos)) & (uint32_t)(msk); \
        REG_WRITE((reg), _tmp);             \
    } while (0)
```

**模式B：内联函数（推荐，类型安全）**

```c
static inline uint32_t hal_Reg_Read(volatile uint32_t *pReg)
{
    return *pReg;
}

static inline void hal_Reg_Write(volatile uint32_t *pReg, uint32_t value)
{
    *pReg = value;
}

static inline void hal_Reg_SetBit(volatile uint32_t *pReg, uint32_t mask)
{
    *pReg |= mask;
}

static inline void hal_Reg_ModifyField(volatile uint32_t *pReg,
                                       uint32_t mask, uint32_t pos,
                                       uint32_t value)
{
    uint32_t tmp = *pReg;
    tmp &= ~mask;
    tmp |= (value << pos) & mask;
    *pReg = tmp;
}
```

### 6.2 CMSIS 兼容层

- Cortex-M 项目**必须**使用 CMSIS Core 头文件（`core_cm4.h` 等）
- 中断控制使用 CMSIS API：`NVIC_EnableIRQ()`, `NVIC_SetPriority()`
- 系统控制使用 CMSIS API：`__enable_irq()`, `__disable_irq()`, `__DSB()`
- 禁止绕过 CMSIS 直接操作 SCB/NVIC 寄存器（调试除外）

### 6.3 位操作安全模式

```c
/* CORRECT: 使用无符号字面量，明确位宽 */
uint32_t mask = (1U << 4U);
uint32_t field = (value >> 8U) & 0xFFU;

/* WRONG: 有符号移位，未定义行为风险 */
int mask = (1 << 31);     /* UB: signed overflow */
int field = (value >> 8);  /* implementation-defined for negative */
```

**位操作规则**：
- 所有位操作必须使用无符号类型
- 移位量必须 < 操作数位宽
- 位字面量后缀 `U`
- 禁止对有符号数进行位操作

---

## 7. 预处理器

### 7.1 宏的限制与替代

| 场景 | 使用 | 替代方案 |
|------|------|---------|
| 常量定义 | 允许 | 优先使用 `static const` 或 `enum` |
| 简单值宏 | 允许 | `#define BUF_SIZE 64U` |
| 函数式宏 | **受限** | 优先使用 `static inline` 函数 |
| 多语句宏 | 必须用 `do { ... } while(0)` 包裹 | — |
| 带副作用参数的宏 | **禁止** | 必须使用函数 |

```c
/* WRONG: 参数被求值两次 */
#define MAX(a, b)  ((a) > (b) ? (a) : (b))
int x = MAX(i++, j++);  /* 未定义行为 */

/* CORRECT: 使用 inline 函数 */
static inline int32_t util_Max_i32(int32_t a, int32_t b)
{
    return (a > b) ? a : b;
}
```

### 7.2 条件编译规则

```c
/* 组织原则：层次化的配置头文件 */

/* product_config.h - 产品级配置 */
#define PRODUCT_HAS_CAN        (1U)
#define PRODUCT_HAS_LIN        (0U)
#define PRODUCT_UART_COUNT     (3U)

/* 条件编译规则 */
/* CORRECT: 使用 #if defined() 或 #if (FEATURE == 1U) */
#if defined(PRODUCT_HAS_CAN) && (PRODUCT_HAS_CAN == 1U)
    #include "drv_can.h"
#endif

/* WRONG: #ifdef 检测值是否定义但不检查值 */
#ifdef PRODUCT_HAS_CAN    /* 即使值为0也会进入 */
```

**条件编译限制**：
- `#if` 嵌套深度 ≤ 3
- 禁止在函数体中间使用 `#if`/`#else` 分割逻辑（应封装为独立函数）
- 所有编译开关必须在配置头文件中集中定义

### 7.3 Include Guard

所有头文件必须使用 include guard：

```c
/* 格式：<MODULE>_<FILENAME>_H */
#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

/* ... 头文件内容 ... */

#ifdef __cplusplus
}
#endif

#endif /* HAL_GPIO_H */
```

- 使用 `#ifndef` / `#define` / `#endif` 三件套（MISRA 兼容）
- `#pragma once` 仅在团队评估后作为补充使用（非 MISRA 兼容）
- guard 宏名必须与文件路径匹配，避免冲突

---

## 8. 错误处理

### 8.1 错误码体系

采用分层错误码设计：

```c
/* 通用状态码 */
typedef enum
{
    STATUS_OK              = 0,     /* 成功 */
    STATUS_ERROR           = -1,    /* 通用错误 */
    STATUS_BUSY            = -2,    /* 资源忙 */
    STATUS_TIMEOUT         = -3,    /* 超时 */
    STATUS_INVALID_PARAM   = -4,    /* 参数无效 */
    STATUS_NOT_INIT        = -5,    /* 未初始化 */
    STATUS_OVERFLOW        = -6,    /* 溢出 */
    STATUS_HW_ERROR        = -7     /* 硬件错误 */
} status_e;

/* 模块级错误码扩展（高16位=模块ID，低16位=模块内错误码） */
#define ERR_MODULE_ID_SHIFT  ((uint32_t)16U)

#define ERR_CODE(module, code) \
    ((uint32_t)((uint32_t)(module) << ERR_MODULE_ID_SHIFT) | (uint32_t)(code))

/* 模块 ID 定义 */
#define ERR_MOD_HAL_GPIO     ((uint32_t)0x0001U)
#define ERR_MOD_HAL_UART     ((uint32_t)0x0002U)
#define ERR_MOD_DRV_SPI      ((uint32_t)0x0003U)

/* 示例：UART 模块错误码 */
#define ERR_UART_FRAME       ERR_CODE(ERR_MOD_HAL_UART, 0x0001U)
#define ERR_UART_PARITY      ERR_CODE(ERR_MOD_HAL_UART, 0x0002U)
#define ERR_UART_OVERRUN     ERR_CODE(ERR_MOD_HAL_UART, 0x0003U)
```

### 8.2 断言策略

```c
/* 开发阶段断言 - 捕获编程错误 */
#if defined(DEBUG_BUILD)
    #define DEV_ASSERT(expr) \
        do { \
            if (!(expr)) { \
                assert_Handler(__FILE__, __LINE__); \
            } \
        } while (0)
#else
    #define DEV_ASSERT(expr)  ((void)0)
#endif

/* 生产阶段断言 - 仅用于不可恢复错误，触发安全状态 */
#define SAFETY_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            safety_EnterFailSafe(__FILE__, __LINE__); \
        } \
    } while (0)
```

**断言使用原则**：
- `DEV_ASSERT`: 用于 `static` 内部函数的前置条件检查，生产版移除
- `SAFETY_ASSERT`: 用于不可恢复的运行时异常（数据腐败、栈溢出检测），生产版保留
- 禁止在断言中使用有副作用的表达式

### 8.3 故障安全状态

每个产品必须定义故障安全状态（Safe State），并在以下场景触发：

| 触发条件 | 动作 |
|---------|------|
| `SAFETY_ASSERT` 失败 | 进入故障安全状态 |
| 看门狗超时 | 硬件复位 → 启动自检 → 故障安全状态 |
| 栈溢出检测 | 进入故障安全状态 |
| RAM/Flash 完整性校验失败 | 进入故障安全状态 |
| 时钟异常 | 切换至内部 RC，进入降级模式 |

```c
/* 故障安全状态处理模板 */
_Noreturn void safety_EnterFailSafe(const char *file, uint32_t line)
{
    __disable_irq();

    /* 1. 将所有输出置为安全值 */
    hal_Output_SetAllSafe();

    /* 2. 记录故障信息到非易失存储 */
    nvm_LogFault(file, line);

    /* 3. 通知看门狗进入安全模式 */
    hal_Wdg_TriggerSafe();

    /* 4. 持续喂狗，保持安全状态 */
    for (;;)
    {
        hal_Wdg_Feed();
    }
}
```

---

## 9. 文件组织与模块化

### 9.1 目录结构模板

```
project_root/
├── platform/                    # 平台公共代码
│   ├── hal/                     # 硬件抽象层
│   │   ├── inc/                 # 公共头文件
│   │   │   ├── hal_gpio.h
│   │   │   ├── hal_uart.h
│   │   │   └── hal_spi.h
│   │   └── src/
│   │       ├── cortex_m/        # Cortex-M 实现
│   │       └── cortex_r52/      # Cortex-R52 实现
│   ├── drivers/                 # 设备驱动
│   │   ├── inc/
│   │   └── src/
│   └── middleware/              # 中间件（通信栈、诊断等）
│       ├── inc/
│       └── src/
├── products/                    # 产品特定代码
│   ├── product_a/
│   │   ├── config/              # 产品配置头文件
│   │   ├── app/                 # 应用逻辑
│   │   └── bsp/                 # 板级支持包
│   └── product_b/
│       ├── config/
│       ├── app/
│       └── bsp/
├── common/                      # 通用工具库
│   ├── inc/
│   │   ├── util_crc.h
│   │   ├── util_fifo.h
│   │   └── util_assert.h
│   └── src/
├── tests/                       # 单元测试（Tessy 工程文件）
│   ├── unit/                    # Tessy 单元测试用例
│   ├── integration/             # 集成测试
│   └── tessy_project/           # Tessy 项目配置
├── tools/                       # 构建脚本、静态分析配置
│   ├── lint/
│   └── scripts/
└── docs/                        # 设计文档
    ├── architecture/
    └── safety/
```

### 9.2 .h/.c 文件对应关系

| 规则 | 说明 |
|------|------|
| F-01 | 每个 `.c` 文件有且仅有一个同名 `.h` 文件 |
| F-02 | `.h` 文件仅包含接口声明（函数原型、类型定义、宏） |
| F-03 | `.c` 文件首先 `#include` 自身对应的 `.h` 文件（自检） |
| F-04 | 禁止在 `.h` 文件中定义变量或函数体（`inline` 除外） |
| F-05 | 模块内部类型/函数使用 `static`，不暴露在头文件中 |
| F-06 | 头文件包含顺序：自身头文件 → 标准库 → 平台头文件 → 项目头文件 |

```c
/* hal_gpio.c */
#include "hal_gpio.h"       /* 1. 自身头文件（自检） */

#include <stdint.h>         /* 2. 标准库 */
#include <stdbool.h>

#include "core_cm4.h"       /* 3. 平台头文件 */

#include "hal_clock.h"      /* 4. 项目依赖 */
#include "util_assert.h"
```

### 9.3 模块接口设计原则

每个模块提供统一的接口模式：

```c
/* hal_gpio.h - 模块公共接口 */
#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include <stdint.h>
#include <stdbool.h>

/* 模块级类型定义 */
typedef struct
{
    uint8_t  port;
    uint8_t  pin;
    uint8_t  mode;
    uint8_t  pull;
} hal_GpioConfig_t;

/* 初始化/反初始化 */
hal_Status_e hal_Gpio_Init(const hal_GpioConfig_t *pConfig);
hal_Status_e hal_Gpio_DeInit(uint8_t port, uint8_t pin);

/* 功能接口 */
hal_Status_e hal_Gpio_Write(uint8_t port, uint8_t pin, bool level);
bool         hal_Gpio_Read(uint8_t port, uint8_t pin);
hal_Status_e hal_Gpio_Toggle(uint8_t port, uint8_t pin);

#endif /* HAL_GPIO_H */
```

**设计原则**：
- 模块间通过头文件暴露的接口通信，禁止直接访问其它模块的内部变量
- 每个模块提供 `Init()` / `DeInit()` 生命周期函数
- 状态查询接口与控制接口分离
- 回调函数通过注册模式（`RegisterCallback`）提供

---

## 10. 编译与静态分析

### 10.1 编译警告等级

```makefile
# GCC ARM 强制编译选项
CFLAGS += -std=c99
CFLAGS += -Wall -Wextra -Werror
CFLAGS += -Wshadow
CFLAGS += -Wdouble-promotion       # float 隐式提升为 double
CFLAGS += -Wformat=2               # printf 格式检查
CFLAGS += -Wconversion             # 隐式类型转换
CFLAGS += -Wundef                  # 未定义宏在 #if 中使用
CFLAGS += -Wcast-align             # 指针对齐
CFLAGS += -Wstrict-prototypes      # 函数原型
CFLAGS += -Wmissing-prototypes     # 缺少原型声明
CFLAGS += -Wredundant-decls        # 冗余声明
CFLAGS += -Wnested-externs         # 嵌套 extern
CFLAGS += -Wswitch-enum            # switch 未覆盖所有枚举值
CFLAGS += -Wno-unused-parameter    # 允许（回调接口常见）

# ARM Compiler 6 等效选项
ARMCLANG_FLAGS += -std=c99
ARMCLANG_FLAGS += -Wall -Werror -Weverything
ARMCLANG_FLAGS += -Wno-padded
ARMCLANG_FLAGS += -Wno-covered-switch-default

# 生产构建额外选项
CFLAGS_RELEASE += -O2 -flto
CFLAGS_RELEASE += -fstack-usage        # 输出栈使用报告
CFLAGS_RELEASE += -ffunction-sections -fdata-sections
LDFLAGS_RELEASE += -Wl,--gc-sections   # 移除未使用段
```

**规则**：CI 构建**零警告**。任何新增警告视为构建失败。

### 10.2 静态分析工具

| 工具 | 用途 | 要求 |
|------|------|------|
| **QAC (Helix QAC)** | MISRA C 合规检查、编码规范执行 | 所有项目必须，CI 集成 |
| **QAC MISRA C:2012 Module** | MISRA C:2012 全规则集检查 | ASIL-A+ 必须 |
| **Polyspace Bug Finder** | 运行时缺陷检测（溢出、除零、越界等） | 所有项目必须 |
| **Polyspace Code Prover** | 形式化证明（证明无运行时错误） | ASIL-C/D 要求，ASIL-B 推荐 |
| **GCC `-fanalyzer`** | 编译器内置分析 | 开发阶段辅助使用 |
| **Stack Analyzer** | 栈深度分析 | 所有项目必须 |

### 10.3 QAC MISRA-C 检查配置

```
/* QAC (Helix QAC) 项目配置模板 */

/* 规则集：MISRA C:2012 (含 Amendment 1/2/3) */
/* 分析模块：MISRA C:2012 Compliance Module */

/* --- 严重级别映射 --- */
/* Mandatory 规则 → Level 9 (构建失败) */
/* Required 规则  → Level 7 (ASIL-A: warning, ASIL-B+: 构建失败) */
/* Advisory 规则  → Level 4 (QM: info, ASIL-A+: warning) */

/* --- 项目批准的偏离 (Deviation Permits) --- */
/* Rule 14.3 (Required): 允许防御性不可达检查 */
/*   偏离编号: DEV-2026-001 */
/*   理由: 防御性编程要求冗余检查以满足安全需求 */

/* --- QAC 命令行集成 --- */
/* qacli analyze -P <project.prj> -R MISRA-C-2012 */
/* qacli report  -P <project.prj> -T MISRA_COMPLIANCE */

/* --- CI 集成阈值 --- */
/* Level 9 (Mandatory 违规): 0 (零容忍) */
/* Level 7 (Required 违规):  0 (ASIL-B+) / 可配置 (QM/ASIL-A) */
/* Level 4 (Advisory 违规):  趋势监控，不阻塞构建 */
```

### 10.4 Polyspace 分析配置

```
/* Polyspace Bug Finder 配置 */
/* 目标: 检测运行时缺陷 (溢出、除零、越界、未初始化等) */

/* 编译器设置 */
/* -compiler gnu-arm / -compiler arm-clang */
/* -target arm-cortex-m4 / -target arm-cortex-r52 */

/* 检查项启用 */
/* 数值类: 整数溢出、除零、浮点异常 */
/* 内存类: 数组越界、空指针解引用、未初始化变量 */
/* 并发类: 数据竞争、死锁检测 (多任务项目) */
/* 安全类: 不安全类型转换、不安全字符串操作 */

/* Polyspace Code Prover 配置 (ASIL-C/D) */
/* 目标: 形式化证明无运行时错误 */
/* 结果分类: Green (已证明安全) / Red (缺陷) / Orange (无法证明) / Gray (不可达) */
/* 验收标准: */
/*   - Red: 0 (零容忍) */
/*   - Orange: 逐一审查并记录，限制数量趋势下降 */
/*   - Gray: 审查是否为预期的不可达代码 */
```

### 10.5 单元测试（Tessy）

采用 **Razorcat Tessy** 作为单元测试工具，实现函数级白盒测试和覆盖率分析。

| 要求 | 说明 |
|------|------|
| 工具 | Tessy（Razorcat Development GmbH） |
| 测试对象 | 所有模块的公共函数 + 安全关键 `static` 函数 |
| 覆盖率目标（QM/ASIL-A） | 语句覆盖 (Statement) ≥ 80%，分支覆盖 (Branch) ≥ 70% |
| 覆盖率目标（ASIL-B） | 语句覆盖 ≥ 90%，分支覆盖 ≥ 80% |
| 覆盖率目标（ASIL-C/D） | 分支覆盖 ≥ 90%，MC/DC 覆盖 ≥ 80% |
| CI 集成 | Tessy CLI (`tessycli`) 集成到 CI Pipeline |
| 测试报告 | 自动生成 XML/HTML 报告，归档到构建产物 |

**Tessy 使用规范**：

- 每个被测模块创建独立的 Tessy 测试模块（Test Module）
- 使用 Tessy 的 CTE（Classification Tree Editor）设计测试用例，确保等价类和边界值覆盖
- 桩函数（Stub）和驱动函数（Driver）由 Tessy 自动生成，手动桩需记录理由
- 测试用例必须覆盖：正常路径、边界值、错误路径、防御性检查路径
- 安全关键模块的测试用例需追溯到需求（通过 Tessy 的需求链接功能）

```bash
# CI 集成命令示例
tessycli --project <project.tpr> --execute-all --report xml --output results/
tessycli --project <project.tpr> --coverage branch --threshold 80
```

### 11.1 Doxygen 注释格式

**文件头注释**（所有文件必须）：

```c
/**
 * @file    hal_gpio.c
 * @brief   GPIO 硬件抽象层实现
 * @author  <作者>
 * @date    2026-01-15
 *
 * @copyright Copyright (c) 2026 <公司名>. All rights reserved.
 *
 * @safety   ASIL-B
 * @version  1.2.0
 *
 * @details
 * 提供 Cortex-M 系列 MCU 的 GPIO 初始化、读写、中断配置功能。
 * 支持 STM32F4/L4/H7 系列。
 */
```

**函数注释**（公共函数必须，`static` 函数按需）：

```c
/**
 * @brief   初始化 GPIO 引脚
 *
 * @param[in]  pConfig  GPIO 配置结构体指针，不允许为 NULL
 *
 * @return  hal_Status_e
 * @retval  HAL_STATUS_OK            初始化成功
 * @retval  HAL_STATUS_INVALID_PARAM 参数无效（NULL 指针或非法端口/引脚）
 * @retval  HAL_STATUS_HW_ERROR      硬件配置失败
 *
 * @pre     时钟模块已初始化（hal_Clock_Init() 已调用）
 * @post    指定 GPIO 引脚按配置工作
 *
 * @safety  ASIL-B
 * @trace   REQ-HAL-GPIO-001, REQ-HAL-GPIO-002
 *
 * @note    该函数非线程安全，初始化阶段调用
 */
hal_Status_e hal_Gpio_Init(const hal_GpioConfig_t *pConfig);
```

### 11.2 安全关键代码追溯性注释

功能安全代码必须包含追溯性标签，建立代码与需求的双向追溯：

| 标签 | 用途 | 示例 |
|------|------|------|
| `@safety` | 安全等级 | `@safety ASIL-B` |
| `@trace` | 关联需求 ID | `@trace REQ-HAL-GPIO-001` |
| `@hazard` | 关联危害分析 | `@hazard HAZ-003` |
| `@verify` | 验证方法 | `@verify Unit test: test_gpio_init` |
| `@deviation` | MISRA 偏离 | `@deviation MISRA-R14.3: 防御性检查` |

```c
/**
 * @brief   电机紧急停止
 * @safety  ASIL-D
 * @trace   REQ-MOTOR-SAFE-001
 * @hazard  HAZ-001: 电机失控
 * @verify  Unit test: test_motor_emergency_stop
 *          Integration test: test_fault_to_safe_state
 */
void app_Motor_EmergencyStop(void)
{
    /* @deviation MISRA-R14.3:
     * 即使当前状态已是停止，仍强制执行硬件关断，
     * 作为防御性冗余措施 */
    hal_Pwm_ForceOutput(MOTOR_PWM_CH, PWM_OUTPUT_LOW);
    hal_Gpio_Write(MOTOR_EN_PORT, MOTOR_EN_PIN, false);

    s_motorState = MOTOR_STATE_SAFE;
}
```

### 11.3 注释风格规则

| 规则 | 说明 |
|------|------|
| C-01 | 使用 `/* */` 风格（MISRA 兼容），禁止 `//`（C99 前不标准） |
| C-02 | 注释解释 "为什么"，不解释 "是什么"（代码自身应可读） |
| C-03 | TODO/FIXME 必须附带责任人和日期：`/* TODO(zhangsan, 2026-04-01): ... */` |
| C-04 | 禁止注释掉的代码提交到主分支（使用版本控制） |
| C-05 | 魔数必须用命名常量替代或添加注释解释来源 |

---

## 附录 A：规范检查清单

### 代码审查检查项

- [ ] 所有整型使用 `<stdint.h>` 固定宽度类型
- [ ] 指针使用前判空，释放后置 NULL
- [ ] 数组访问有边界检查
- [ ] 公共函数有参数校验
- [ ] 无递归调用
- [ ] ISR 尽可能短，无阻塞操作
- [ ] 共享变量使用 `volatile` + 临界区保护
- [ ] 无动态内存分配（或使用内存池）
- [ ] 错误码正确传播和处理
- [ ] 编译零警告
- [ ] Doxygen 注释完整
- [ ] 安全关键代码有追溯性标签
- [ ] 命名符合模块前缀规范
- [ ] 函数复杂度在限制内

### 提交前检查

- [ ] QAC MISRA C:2012 检查通过（零 Mandatory/Required 违规）
- [ ] Polyspace Bug Finder 无 Red 缺陷
- [ ] 栈使用分析在预算内
- [ ] Tessy 单元测试覆盖率 ≥ 80%

---

## 附录 B：常用 MISRA C:2012 规则速查

| 规则 | 级别 | 说明 |
|------|------|------|
| Dir 1.1 | Required | 不依赖未定义/未指定行为 |
| Dir 4.1 | Required | 运行时错误需最小化 |
| Dir 4.6 | Advisory→**Required** | 使用固定宽度整型 |
| Dir 4.12 | Required | 禁止动态内存分配 |
| Rule 1.3 | Required | 无未定义行为 |
| Rule 2.7 | Advisory | 未使用参数需 `(void)` 标注 |
| Rule 8.4 | Required | 外部函数/变量需先声明 |
| Rule 8.13 | Advisory | 只读指针需 `const` |
| Rule 10.1 | Required | 操作数类型合适 |
| Rule 10.3 | Required | 赋值时类型匹配 |
| Rule 11.3 | Required | 禁止不相关类型间指针转换 |
| Rule 11.5 | Advisory→**Required** | `void*` 到具体类型需显式转换 |
| Rule 12.1 | Advisory | 表达式需显式括号 |
| Rule 14.3 | Required（**偏离**） | 允许防御性不可达检查 |
| Rule 15.5 | Advisory | 单一出口 |
| Rule 15.7 | Required | `if...else if` 必须以 `else` 结束 |
| Rule 16.4 | Required | `switch` 必须有 `default` |
| Rule 17.2 | Required | 禁止递归 |
| Rule 17.7 | Required | 返回值不能忽略 |
| Rule 21.3 | Required | 禁止 `<stdlib.h>` 内存函数 |
| Rule 21.6 | Required | 禁止 `<stdio.h>` I/O 函数 |

---

*文档结束。本规范由团队安全负责人维护，每季度评审一次。*
