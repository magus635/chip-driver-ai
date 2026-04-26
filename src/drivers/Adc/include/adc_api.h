/**
 * @file    adc_api.h
 * @brief   ADC Driver API
 */
#ifndef ADC_API_H
#define ADC_API_H

#include "adc_types.h"

Adc_ReturnType Adc_Init(const Adc_ConfigType *pConfig);
void Adc_DeInit(void);

/* Polling APIs */
Adc_ReturnType Adc_StartConversion(void);
Adc_ReturnType Adc_PollForConversion(uint32_t timeout);
uint16_t Adc_GetConversionValue(void);

/* DMA/Asynchronous APIs (R14/R15 Compliance) */
Adc_ReturnType Adc_StartConversionDMA(uint16_t *pData, uint16_t length);
Adc_ReturnType Adc_StopConversionDMA(void);

#endif /* ADC_API_H */
