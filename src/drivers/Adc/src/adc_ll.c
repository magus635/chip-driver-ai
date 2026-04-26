/**
 * @file    adc_ll.c
 * @brief   ADC Low-Level complex register synthesis (AR-01 compliance)
 */
#include "adc_ll.h"
#include <stddef.h>

void ADC_LL_ConfigureMode(ADC_TypeDef *ADCx, const Adc_ConfigType *pConfig)
{
    if (pConfig == NULL) return;

    /* 1. Configure CR1 (SCAN) */
    if (pConfig->scanMode)
    {
        ADCx->CR1 |= ADC_CR1_SCAN_Msk;
    }
    else
    {
        ADCx->CR1 &= ~ADC_CR1_SCAN_Msk;
    }

    /* 2. Configure CR2 (CONT, ALIGN) */
    uint32_t cr2 = ADCx->CR2;
    cr2 &= ~(ADC_CR2_CONT_Msk | ADC_CR2_ALIGN_Msk);

    if (pConfig->continuous)
    {
        cr2 |= ADC_CR2_CONT_Msk;
    }
    if (pConfig->alignment == ADC_ALIGN_LEFT)
    {
        cr2 |= ADC_CR2_ALIGN_Msk;
    }
    
    ADCx->CR2 = cr2;
}

void ADC_LL_ConfigureSequence(ADC_TypeDef *ADCx, const Adc_ConfigType *pConfig)
{
    if (pConfig == NULL) return;

    uint32_t sqr1 = 0;
    uint32_t sqr2 = 0;
    uint32_t sqr3 = 0;
    uint32_t smpr1 = 0;
    uint32_t smpr2 = 0;

    /* Set total number of conversions in SQR1 */
    uint8_t num = pConfig->numOfChannels;
    if (num > 0)
    {
        sqr1 |= ((uint32_t)(num - 1U) << ADC_SQR1_L_Pos);
    }

    /* Build sequence and sample times */
    for (uint8_t i = 0; i < num; i++)
    {
        uint8_t ch = pConfig->channels[i];
        uint8_t st = pConfig->sampleTimes[i] & 0x07U; /* 3-bit sample time */

        /* SQRx */
        if (i < 6)
        {
            sqr3 |= ((uint32_t)ch << (5U * i));
        }
        else if (i < 12)
        {
            sqr2 |= ((uint32_t)ch << (5U * (i - 6U)));
        }
        else if (i < 16)
        {
            sqr1 |= ((uint32_t)ch << (5U * (i - 12U)));
        }

        /* SMPRx */
        if (ch < 10)
        {
            smpr2 |= ((uint32_t)st << (3U * ch));
        }
        else if (ch < 18)
        {
            smpr1 |= ((uint32_t)st << (3U * (ch - 10U)));
        }
    }

    /* Write registers */
    ADCx->SQR1 = sqr1;
    ADCx->SQR2 = sqr2;
    ADCx->SQR3 = sqr3;
    ADCx->SMPR1 = smpr1;
    ADCx->SMPR2 = smpr2;
}
