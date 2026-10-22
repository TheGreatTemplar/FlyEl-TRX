#include "adc1_app.h"

#define ADC_CR2_ADON_BIT            (1UL << 0)
#define ADC_CR2_CAL_BIT             (1UL << 2)
#define ADC_CR2_RSTCAL_BIT          (1UL << 3)
#define ADC_CR2_EXTSEL_SWSTART      (7UL << 17)
#define ADC_CR2_EXTTRIG_BIT         (1UL << 20)
#define ADC_CR2_SWSTART_BIT         (1UL << 22)

static void ADC_ShortDelay(void)
{
    volatile uint32_t i;

    for (i = 0U; i < 1000U; i++)
    {
        __NOP();
    }
}

/* Настраивает PA0/PA1, время выборки и калибровку ADC1 перед измерениями. */
void ADC1_App_Init(void)
{
    /*
     * PA0 = ADC_IN0
     * PA1 = ADC_IN1
     */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_ADC1EN;

    /*
     * Analog input: MODE = 00, CNF = 00.
     */
    GPIOA->CRL &= ~((0xFUL << 0U) | (0xFUL << 4U));

    /*
     * Одна регулярная конверсия.
     */
    ADC1->SQR1 = 0U;

    /*
     * Длинное время выборки для устойчивого измерения.
     * SMP0 = 111, SMP1 = 111.
     */
    ADC1->SMPR2 &= ~((7UL << 0U) | (7UL << 3U));
    ADC1->SMPR2 |=  ((7UL << 0U) | (7UL << 3U));

    ADC1->CR1 = 0U;
    ADC1->CR2 = ADC_CR2_EXTSEL_SWSTART | ADC_CR2_EXTTRIG_BIT;

    /*
     * Включение ADC.
     */
    ADC1->CR2 |= ADC_CR2_ADON_BIT;
    ADC_ShortDelay();

    /*
     * Сброс калибровки.
     */
    ADC1->CR2 |= ADC_CR2_RSTCAL_BIT;
    while ((ADC1->CR2 & ADC_CR2_RSTCAL_BIT) != 0U)
    {
    }

    /*
     * Калибровка.
     */
    ADC1->CR2 |= ADC_CR2_CAL_BIT;
    while ((ADC1->CR2 & ADC_CR2_CAL_BIT) != 0U)
    {
    }
}

/* Выполняет одну блокирующую конверсию канала; возвращает 12-битный отсчёт. */
uint16_t ADC1_App_ReadRaw(adc_app_channel_t channel)
{
    uint32_t adc_channel;

    if (channel == ADC_APP_CH1)
    {
        adc_channel = 1U;
    }
    else
    {
        adc_channel = 0U;
    }

    ADC1->SQR3 = adc_channel;

    ADC1->SR = 0U;
    ADC1->CR2 |= ADC_CR2_SWSTART_BIT;

    while ((ADC1->SR & ADC_SR_EOC) == 0U)
    {
    }

    return (uint16_t)(ADC1->DR & 0x0FFFU);
}

/* Переводит отсчёт в мВ по переданному опорному напряжению. */
uint16_t ADC1_App_ReadMv(adc_app_channel_t channel, uint16_t vref_mv)
{
    uint32_t raw;
    uint32_t mv;

    raw = ADC1_App_ReadRaw(channel);
    mv = (raw * (uint32_t)vref_mv) / 4095U;

    return (uint16_t)mv;
}
