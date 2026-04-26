/**
 * @file    adc_reg.h
 * @brief   STM32F103C8T6 ADC Register Map
 */
#ifndef ADC_REG_H
#define ADC_REG_H

#include <stdint.h>

typedef struct
{
    volatile uint32_t SR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SMPR1;
    volatile uint32_t SMPR2;
    volatile uint32_t JOFR1;
    volatile uint32_t JOFR2;
    volatile uint32_t JOFR3;
    volatile uint32_t JOFR4;
    volatile uint32_t HTR;
    volatile uint32_t LTR;
    volatile uint32_t SQR1;
    volatile uint32_t SQR2;
    volatile uint32_t SQR3;
    volatile uint32_t JSQR;
    volatile uint32_t JDATA1;
    volatile uint32_t JDATA2;
    volatile uint32_t JDATA3;
    volatile uint32_t JDATA4;
    volatile uint32_t DR;
} ADC_TypeDef;

#define ADC1_BASE       (0x40012400UL)
#define ADC2_BASE       (0x40012800UL)

/* SR */
#define ADC_SR_EOC_Pos  (1U)
#define ADC_SR_EOC_Msk  (1UL << ADC_SR_EOC_Pos)
#define ADC_SR_STRT_Pos (4U)
#define ADC_SR_STRT_Msk (1UL << ADC_SR_STRT_Pos)

/* CR1 */
#define ADC_CR1_EOCIE_Pos (5U)
#define ADC_CR1_EOCIE_Msk (1UL << ADC_CR1_EOCIE_Pos)
#define ADC_CR1_SCAN_Pos  (8U)
#define ADC_CR1_SCAN_Msk  (1UL << ADC_CR1_SCAN_Pos)

/* CR2 */
#define ADC_CR2_ADON_Pos  (0U)
#define ADC_CR2_ADON_Msk  (1UL << ADC_CR2_ADON_Pos)
#define ADC_CR2_CONT_Pos  (1U)
#define ADC_CR2_CONT_Msk  (1UL << ADC_CR2_CONT_Pos)
#define ADC_CR2_CAL_Pos   (2U)
#define ADC_CR2_CAL_Msk   (1UL << ADC_CR2_CAL_Pos)
#define ADC_CR2_RSTCAL_Pos (3U)
#define ADC_CR2_RSTCAL_Msk (1UL << ADC_CR2_RSTCAL_Pos)
#define ADC_CR2_DMA_Pos   (8U)
#define ADC_CR2_DMA_Msk   (1UL << ADC_CR2_DMA_Pos)
#define ADC_CR2_ALIGN_Pos (11U)
#define ADC_CR2_ALIGN_Msk (1UL << ADC_CR2_ALIGN_Pos)
#define ADC_CR2_EXTTRIG_Pos (20U)
#define ADC_CR2_EXTTRIG_Msk (1UL << ADC_CR2_EXTTRIG_Pos)
#define ADC_CR2_SWSTART_Pos (22U)
#define ADC_CR2_SWSTART_Msk (1UL << ADC_CR2_SWSTART_Pos)
#define ADC_CR2_EXTSEL_Pos  (17U)
#define ADC_CR2_EXTSEL_Msk  (7UL << ADC_CR2_EXTSEL_Pos)

/* SQR1 */
#define ADC_SQR1_L_Pos    (20U)
#define ADC_SQR1_L_Msk    (0xFUL << ADC_SQR1_L_Pos)

#endif /* ADC_REG_H */
