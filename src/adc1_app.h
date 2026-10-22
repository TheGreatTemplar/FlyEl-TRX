#ifndef ADC1_APP_H
#define ADC1_APP_H

#include "stm32f10x.h"
#include <stdint.h>

typedef enum
{
    ADC_APP_CH0 = 0,
    ADC_APP_CH1 = 1
} adc_app_channel_t;

void ADC1_App_Init(void);
uint16_t ADC1_App_ReadRaw(adc_app_channel_t channel);
uint16_t ADC1_App_ReadMv(adc_app_channel_t channel, uint16_t vref_mv);

#endif
