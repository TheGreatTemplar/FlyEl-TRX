#include "frequency_parse.h"
#include "app_config.h"

uint8_t Frequency_Parse(const char *text, uint32_t *hz)
{
    uint32_t whole = 0U, fraction = 0U, scale = 100000U;
    uint8_t dot = 0U, digits = 0U;
    char ch;
    if (text == 0 || hz == 0) return 0U;
    while ((ch = *text++) != '\0')
    {
        if (ch == '.' && dot == 0U) { dot = 1U; continue; }
        if (ch < '0' || ch > '9') return 0U;
        digits = 1U;
        if (dot == 0U)
        {
            if (whole > 30000U) return 0U;
            whole = whole * 10U + (uint32_t)(ch - '0');
        }
        else
        {
            if (scale == 0U) return 0U;
            fraction += (uint32_t)(ch - '0') * scale;
            scale /= 10U;
        }
    }
    if (digits == 0U) return 0U;
    if (dot != 0U)
    {
        if (whole > 30U) return 0U;
        whole = whole * 1000000U + fraction;
    }
    else
    {
        if (whole > 30000U) return 0U;
        whole *= 1000U;
    }
    if (whole < RADIO_MIN_HZ || whole > RADIO_MAX_HZ) return 0U;
    *hz = whole;
    return 1U;
}

uint8_t Channel_Parse(const char *text, uint8_t *zero_based)
{
    uint32_t channel = 0U;
    if (text == 0 || zero_based == 0 || *text == '\0') return 0U;
    while (*text != '\0')
    {
        if (*text < '0' || *text > '9' || channel > MEMORY_CHANNEL_COUNT) return 0U;
        channel = channel * 10U + (uint32_t)(*text++ - '0');
    }
    if (channel == 0U || channel > MEMORY_CHANNEL_COUNT) return 0U;
    *zero_based = (uint8_t)(channel - 1U);
    return 1U;
}
