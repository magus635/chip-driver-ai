/**
 * @file    adc_types.h
 * @brief   ADC Type Definitions
 */
#ifndef ADC_TYPES_H
#define ADC_TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    ADC_OK          = 0,
    ADC_ERR         = 1,
    ADC_BUSY        = 2,
    ADC_ERR_PARAM   = 3,
    ADC_ERR_TIMEOUT = 4
} Adc_ReturnType;

typedef enum
{
    ADC_ALIGN_RIGHT = 0,
    ADC_ALIGN_LEFT  = 1
} Adc_DataAlignType;

typedef struct
{
    uint8_t             instance;       /* 1 for ADC1, 2 for ADC2 */
    bool                continuous;     /* true: continuous, false: single */
    bool                scanMode;       /* true: scan mode, false: single channel */
    Adc_DataAlignType   alignment;      /* Right or Left */
    uint8_t             numOfChannels;  /* 1 to 16 */
    uint8_t             channels[16];   /* Channel sequence */
    uint8_t             sampleTimes[16];/* Sample time for each channel in sequence */
} Adc_ConfigType;

#endif /* ADC_TYPES_H */
