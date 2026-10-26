#include "frequency_parse.h"
#include "app_config.h"

/* Позиционный ввод с разрешением 10 Гц. Незаполненный правый хвост — нули. */
uint8_t Frequency_ParseDigits(const char *text, uint8_t mhz_digits, uint32_t *hz)
{
    uint8_t count = 0U;
    uint8_t limit = (uint8_t)(mhz_digits + 5U);
    uint32_t value = 0U;
    if (text == 0 || hz == 0 || (mhz_digits != 1U && mhz_digits != 2U))
        return 0U;
    while (*text != '\0')
    {
        if (*text < '0' || *text > '9' || count >= limit)
            return 0U;
        value = value * 10U + (uint32_t)(*text++ - '0');
        count++;
    }
    if (count == 0U)
        return 0U;
    while (count++ < limit)
        value *= 10U;
    value *= 10U;
    if (value < RADIO_MIN_HZ || value > RADIO_MAX_HZ)
        return 0U;
    *hz = value;
    return 1U;
}


/* Шаблон _.___.__ / __.___.__: только экранные точки, исходные данные — цифры. */
void Frequency_FormatDigits(const char *digits, uint8_t mhz_digits, char *text)
{
    uint8_t i, cursor = 0U;
    for (i = 0U; i < (uint8_t)(mhz_digits + 5U); i++)
    {
        if (i == mhz_digits || i == (uint8_t)(mhz_digits + 3U))
            text[cursor++] = '.';
        text[cursor++] = (*digits != '\0') ? *digits++ : '_';
    }
    text[cursor] = '\0';
}


uint8_t Channel_Parse(const char *text, uint8_t *zero_based)
{
    uint32_t channel = 0U;
    if (text == 0 || zero_based == 0 || *text == '\0')
        return 0U;
    while (*text != '\0')
    {
        if (*text < '0' || *text > '9' || channel > MEMORY_CHANNEL_COUNT)
            return 0U;
        channel = channel * 10U + (uint32_t)(*text++ - '0');
    }
    if (channel == 0U || channel > MEMORY_CHANNEL_COUNT)
        return 0U;
    *zero_based = (uint8_t)(channel - 1U);
    return 1U;
}
