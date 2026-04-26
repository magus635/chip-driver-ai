/**
 * @file    adc_ll.h
 * @brief   ADC Low-Level inline functions
 */
#ifndef ADC_LL_H
#define ADC_LL_H

#include "adc_reg.h"
#include "adc_types.h"

static inline void ADC_LL_Enable(ADC_TypeDef *ADCx)
{
    ADCx->CR2 |= ADC_CR2_ADON_Msk;
}

static inline void ADC_LL_Disable(ADC_TypeDef *ADCx)
{
    ADCx->CR2 &= ~ADC_CR2_ADON_Msk;
}

static inline void ADC_LL_StartCalibration(ADC_TypeDef *ADCx)
{
    ADCx->CR2 |= ADC_CR2_CAL_Msk;
}

static inline bool ADC_LL_IsCalibrationDone(const ADC_TypeDef *ADCx)
{
    return ((ADCx->CR2 & ADC_CR2_CAL_Msk) == 0U);
}

static inline void ADC_LL_ResetCalibration(ADC_TypeDef *ADCx)
{
    ADCx->CR2 |= ADC_CR2_RSTCAL_Msk;
}

static inline bool ADC_LL_IsResetCalibrationDone(const ADC_TypeDef *ADCx)
{
    return ((ADCx->CR2 & ADC_CR2_RSTCAL_Msk) == 0U);
}

static inline void ADC_LL_SoftwareStart(ADC_TypeDef *ADCx)
{
    ADCx->CR2 |= (ADC_CR2_EXTTRIG_Msk | ADC_CR2_SWSTART_Msk);
}

static inline bool ADC_LL_IsConversionComplete(const ADC_TypeDef *ADCx)
{
    return ((ADCx->SR & ADC_SR_EOC_Msk) != 0U);
}

static inline uint16_t ADC_LL_ReadData(const ADC_TypeDef *ADCx)
{
    return (uint16_t)(ADCx->DR & 0xFFFFU);
}

static inline uint32_t ADC_LL_GetDRAddress(const ADC_TypeDef *ADCx)
{
    return (uint32_t)&(ADCx->DR);
}

static inline void ADC_LL_ClearEocFlag(ADC_TypeDef *ADCx)
{
    ADCx->SR &= ~ADC_SR_EOC_Msk;
}

static inline void ADC_LL_EnableDMA(ADC_TypeDef *ADCx)
{
    ADCx->CR2 |= ADC_CR2_DMA_Msk;
}

static inline void ADC_LL_DisableDMA(ADC_TypeDef *ADCx)
{
    ADCx->CR2 &= ~ADC_CR2_DMA_Msk;
}

static inline void ADC_LL_EnableInterrupts(ADC_TypeDef *ADCx)
{
    ADCx->CR1 |= ADC_CR1_EOCIE_Msk;
}

static inline void ADC_LL_DisableInterrupts(ADC_TypeDef *ADCx)
{
    ADCx->CR1 &= ~ADC_CR1_EOCIE_Msk;
}

/* 声明需要在 adc_ll.c 中实现的复杂寄存器合成函数 */
void ADC_LL_ConfigureSequence(ADC_TypeDef *ADCx, const Adc_ConfigType *pConfig);
void ADC_LL_ConfigureMode(ADC_TypeDef *ADCx, const Adc_ConfigType *pConfig);

#endif /* ADC_LL_H */
