# STM32F103 ADC Intermediate Representation (IR)

## 1. 模块信息
- **模块名称**: Analog-to-Digital Converter (ADC)
- **支持通道**: 最多 18 个多路复用通道 (16 个外部, 2 个内部 VREF/Temp)
- **分辨率**: 12-bit 逐次逼近型

## 2. 核心寄存器
- **ADC_SR** (0x00): 状态寄存器 (EOC, AWD, JEOC, STRT)
- **ADC_CR1** (0x04): 控制寄存器 1 (EOCIE, AWDIE, JEOCIE, SCAN, DUALMOD)
- **ADC_CR2** (0x08): 控制寄存器 2 (ADON, CONT, CAL, ALIGN, DMA, SWSTART)
- **ADC_SMPRx** (0x0C/0x10): 采样时间寄存器
- **ADC_SQRx** (0x2C/0x30/0x34): 规则序列寄存器
- **ADC_DR** (0x4C): 规则数据寄存器 (只读)

## 3. 功能特性矩阵
| 功能特性 | 硬件支持情况 (基于 IR) | 驱动层实现要求 |
| :--- | :--- | :--- |
| **基础配置** | ADC_CR2_ADON, CONT | 必须支持单次/连续转换 |
| **规则组转换** | SWSTART, SQRx | 必须支持软件触发转换 |
| **中断支持** | EOCIE, JEOCIE, AWDIE | 必须支持转换完成中断回调 |
| **DMA 传输** | ADC_CR2_DMA | **必须**支持 DMA 异步读取 (映射到 DMA1_CH1) |
| **通道采样** | SMPRx | 必须支持通道独立采样时间配置 |
| **模拟看门狗** | AWD, HTR, LTR | 支持越界监控 |
| **注入组** | JSTRT, JEOC, JSQR | 支持高优先级注入转换 |
