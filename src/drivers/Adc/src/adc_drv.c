/**
 * @file    adc_drv.c
 * @brief   ADC Driver Implementation
 */
#include "adc_api.h"
#include "adc_ll.h"
#include "dma_api.h"
#include <stddef.h>

#define ADC1_DMA_CH DMA_CH1

static ADC_TypeDef * const g_pAdc = (ADC_TypeDef *)ADC1_BASE;
static volatile Adc_ReturnType g_adc_state = ADC_OK;

/* R16 Linkage: Callback from DMA when transfer is complete */
static void Adc_DmaCallback(void *pContext)
{
    (void)pContext;
    ADC_LL_DisableDMA(g_pAdc);
    g_adc_state = ADC_OK;
}

Adc_ReturnType Adc_Init(const Adc_ConfigType *pConfig)
{
    if (pConfig == NULL) return ADC_ERR_PARAM;

    ADC_LL_Disable(g_pAdc);

    /* AR-01 Compliant: No bit manipulation here */
    ADC_LL_ConfigureMode(g_pAdc, pConfig);
    ADC_LL_ConfigureSequence(g_pAdc, pConfig);

    ADC_LL_Enable(g_pAdc);

    /* Calibration */
    ADC_LL_ResetCalibration(g_pAdc);
    uint32_t timeout = 100000;
    while (!ADC_LL_IsResetCalibrationDone(g_pAdc) && --timeout > 0);
    
    ADC_LL_StartCalibration(g_pAdc);
    timeout = 100000;
    while (!ADC_LL_IsCalibrationDone(g_pAdc) && --timeout > 0);

    g_adc_state = ADC_OK;
    return ADC_OK;
}

void Adc_DeInit(void)
{
    ADC_LL_Disable(g_pAdc);
    g_adc_state = ADC_OK;
}

Adc_ReturnType Adc_StartConversion(void)
{
    if (g_adc_state != ADC_OK) return ADC_BUSY;
    g_adc_state = ADC_BUSY;
    
    ADC_LL_SoftwareStart(g_pAdc);
    return ADC_OK;
}

Adc_ReturnType Adc_PollForConversion(uint32_t timeout)
{
    while (!ADC_LL_IsConversionComplete(g_pAdc))
    {
        if (timeout == 0U) return ADC_ERR_TIMEOUT;
        timeout--;
    }
    ADC_LL_ClearEocFlag(g_pAdc);
    g_adc_state = ADC_OK;
    return ADC_OK;
}

uint16_t Adc_GetConversionValue(void)
{
    return ADC_LL_ReadData(g_pAdc);
}

Adc_ReturnType Adc_StartConversionDMA(uint16_t *pData, uint16_t length)
{
    if (pData == NULL) return ADC_ERR_PARAM;
    if (g_adc_state != ADC_OK) return ADC_BUSY;

    Dma_ConfigType dmaCfg = {
        .channel = ADC1_DMA_CH,
        .direction = DMA_DIR_P2M,
        .srcSize = DMA_SIZE_16BIT,
        .dstSize = DMA_SIZE_16BIT,
        .srcInc = false,
        .dstInc = true,
        .pCallback = Adc_DmaCallback,
        .pContext = NULL
    };
    Dma_InitChannel(&dmaCfg);
    
    /* AR-01 Compliant: Use GetDRAddress */
    Dma_StartTransfer(ADC1_DMA_CH, ADC_LL_GetDRAddress(g_pAdc), (uint32_t)pData, length);

    g_adc_state = ADC_BUSY;
    ADC_LL_EnableDMA(g_pAdc);
    ADC_LL_SoftwareStart(g_pAdc);

    return ADC_OK;
}

Adc_ReturnType Adc_StopConversionDMA(void)
{
    Dma_StopTransfer(ADC1_DMA_CH);
    ADC_LL_DisableDMA(g_pAdc);
    g_adc_state = ADC_OK;
    return ADC_OK;
}
