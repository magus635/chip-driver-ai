# DMA Peripheral IR Summary

- **Name**: DMA (Direct Memory Access Controller)
- **Family**: STM32F1
- **Model**: STM32F103C8T6
- **Reference**: RM0008 Rev 21

## Instances

| Name | Base Address | Bus | Channels |
| :--- | :--- | :--- | :--- |
| DMA1 | 0x40020000 | AHB | 7 |

## Register Map

### Common Registers
| Name | Offset | Access | Description |
| :--- | :--- | :--- | :--- |
| ISR | 0x00 | R | Interrupt Status Register |
| IFCR | 0x04 | W | Interrupt Flag Clear Register |

### Channel Registers (x = 1..7)
| Name | Base Offset | Step | Description |
| :--- | :--- | :--- | :--- |
| CCRx | 0x08 | 0x14 | Configuration Register |
| CNDTRx | 0x0C | 0x14 | Number of Data Register |
| CPARx | 0x10 | 0x14 | Peripheral Address Register |
| CMARx | 0x14 | 0x14 | Memory Address Register |

## Invariants

- **INV_DMA_001**: 通道必须在禁用状态 (`CCR.EN == 0`) 下才能修改地址、数据长度和基本配置位。

## Initialization Sequence

1. 使能 DMA 时钟 (RCC_AHBENR)。
2. 确保通道已禁用 (CCR.EN=0)。
3. 设置外设地址 (CPAR)。
4. 设置内存地址 (CMAR)。
5. 设置数据传输总量 (CNDTR)。
6. 配置传输方向、位宽、自增模式、循环模式及优先级 (CCR)。
7. 根据需要使能中断 (TCIE, HTIE, TEIE)。
8. 使能通道 (CCR.EN=1)。

<!-- ir_json_sha256: 44ddbdbcb5a76d9b5602bf88c4877eb5f84bb16644c6e3cc13373de51bbeb617 -->
