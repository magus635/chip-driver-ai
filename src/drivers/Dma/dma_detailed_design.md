# DMA 驱动详细设计报告 (Detailed Design Report)

## 1. 架构概述 (Architecture Overview)

本驱动遵循 4 层解耦架构：
- **Reg 层**：定义 `DMA_TypeDef` 和 `DMA_Channel_TypeDef`，支持多通道数组访问。
- **LL 层**：实现原子操作和 `INV_DMA_001` 守卫逻辑。
- **Drv 层**：负责通道生命周期管理和传输控制。

## 2. 功能特性实现矩阵 (R10 · 由 ir/dma_feature_matrix.json 自动渲染)

> **不要手工编辑下方表格**。状态变更走 `scripts/feature-update.py`，循环补齐走 `scripts/feature-loop.sh`。

<!-- FEATURE_MATRIX:BEGIN -->
| 特性ID | 类别 | 功能特性 | 状态 | API | 备注 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `DMA-INIT` | 生命周期 | Init / DeInit | 🟢 done | `Dma_Init, Dma_DeInit` |  |
| `DMA-GUARD-INV-DMA-001` | 硬件保护 | INV_DMA_001 守卫 | 🟢 done | `-` |  |
| `DMA-XFER-M2P` | 基础传输 | 内存到外设 (M2P) | 🟢 done | `Dma_StartTransfer` |  |
| `DMA-XFER-P2M` | 基础传输 | 外设到内存 (P2M) | 🟢 done | `Dma_StartTransfer` |  |
| `DMA-XFER-M2M` | 基础传输 | 内存到内存 (M2M) | 🟢 done | `Dma_StartTransfer` |  |
| `DMA-WIDTH` | 数据宽度 | PSIZE/MSIZE | 🟢 done | `Dma_StartTransfer` |  |
| `DMA-INC` | 寻址模式 | PINC/MINC | 🟢 done | `Dma_StartTransfer` |  |
| `DMA-CIRC` | 寻址模式 | 循环模式 | 🟢 done | `Dma_StartTransfer` |  |
| `DMA-IRQ-TC` | 中断与事件 | TC 轮询 | 🟢 done | `Dma_IsTransferComplete` |  |
| `DMA-IRQ-HTTE` | 中断与事件 | HT/TE 事件 | 🟢 done | `Dma_IsHalfTransfer, Dma_HasError, Dma_ClearFlags` | 已解析寄存器位，暂无 API 导出 |
| `DMA-ISR` | 中断与事件 | IRQHandler 入口与回调 | 🟢 done | `DMA1_Channel1_IRQHandler, Dma_RegisterCallback, Dma_HandleInterrupt` | 框架已建立，等待回调逻辑注入 |
<!-- FEATURE_MATRIX:END -->

## 3. 核心流程图 (Process Flowcharts)

### 3.1 Dma_StartTransfer 流程

```mermaid
flowchart TD
    Start([Dma_StartTransfer]) --> ParamCheck{参数合法?}
    ParamCheck -- No --> ErrParam[返回 DMA_PARAM]
    ParamCheck -- Yes --> GuardCheck{通道已开启?}
    GuardCheck -- Yes --> ErrBusy[返回 DMA_BUSY]
    GuardCheck -- No --> SetAddr[设置 CPAR / CMAR 地址]
    SetAddr --> SetLen[设置 CNDTR 数据长度]
    SetLen --> ClearFlags[清除 ISR 状态位 - W1C]
    ClearFlags --> EnableCH[置位 CCR.EN]
    EnableCH --> End([返回 OK])
```

## 4. 关键不变式审计 (Invariant Audit)

- **INV_DMA_001**: 为了防止传输过程中配置冲突，LL 层在修改 `CPAR`, `CMAR`, `CNDTR` 之前会强制检查 `CCR.EN`。如果通道正在运行，修改请求将被静默忽略（底层保护）或通过 Drv 层返回 `DMA_BUSY`。

