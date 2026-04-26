# ADC Driver Detailed Design V4

## 1. 架构目标
- 遵循 4 层抽象：Register, Low-Level (LL), Driver, API。
- 遵循 R14: 全量实现 IR 中提取的功能（包含 DMA、中断、轮询），并提供状态公示。
- 遵循 R15: 实现非阻塞 DMA 接口 `Adc_StartConversionDMA`。
- 遵循 R16: 与底层 DMA 联动，提供 `Adc_DmaCallback` 通知。
- **遵循 AR-01**: 严禁在 Driver 层进行算术运算或位操作，所有寄存器配置值的合成必须在 `adc_ll.c` 中完成。

## 2. 文件清单
| 文件名 | 职责 (AR-01 兼容) |
| --- | --- |
| `adc_reg.h` | 寄存器映射与掩码 (纯宏定义) |
| `adc_types.h` | 结构体 `Adc_ConfigType`, `Adc_ReturnType` 等 |
| `adc_ll.h` | 原子寄存器访问 (`static inline`) |
| `adc_ll.c` | **复杂寄存器合成**：`ADC_LL_Configure` |
| `adc_api.h` | 向上提供的 API |
| `adc_drv.c` | 驱动流程，纯业务流（无掩码位操作） |
| `adc_cfg.h` | 静态配置（超时等） |
| `adc_isr.c` | 中断处理 (ADC1_2_IRQHandler) |

## 3. 功能特性全量实现矩阵 (R14 状态公示)
| 功能特性 | 硬件支持情况 (IR 映射) | 实现状态 | 对应的 API / 说明 |
| :--- | :--- | :--- | :--- |
| **基础单次转换** | ADC_CR2_ADON | 🟢 **已完成** | `Adc_StartConversion`, `Adc_PollForConversion` |
| **连续转换模式** | ADC_CR2_CONT | 🟢 **已完成** | `ADC_LL_ConfigureMode` 中根据 Config 配置 |
| **规则组序列** | SQRx, SCAN | 🟢 **已完成** | `ADC_LL_ConfigureSequence` 自动计算长度和位移 |
| **异步 DMA 模式** | ADC_CR2_DMA, DMA1_CH1| 🟢 **已完成** | `Adc_StartConversionDMA`, `Adc_DmaCallback` 回调解耦 |
| **转换完成中断** | ADC_CR1_EOCIE | 🟡 **未完成** | LL 层已提供开关接口，但 Driver 层尚未封装 `Adc_StartConversionIT` |
| **通道采样时间** | SMPRx | 🟢 **已完成** | `ADC_LL_ConfigureSequence` 自动配置 |
| **模拟看门狗** | AWD, AWDIE, HTR, LTR | 🔴 **未完成** | IR 中存在定义，但 V4.0 基线暂未纳入需求 |
| **注入组转换** | JSTRT, JEOC, JSQR | 🔴 **未完成** | 硬件支持，当前优先满足 Regular Group 流程 |

## 4. 异步闭环设计 (DMA)
- 通道映射: ADC1 对应 `DMA_CH1`。
- 联动逻辑: 由 DMA 的 `Adc_DmaCallback` 负责释放 `g_adc_state`。
