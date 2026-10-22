/* Цифровой ввод с фиксированными позициями МГц/кГц/десятков герц. */
#include "freq_entry.h"
#include "app_state.h"
#include "radio_control.h"
#include "frequency_parse.h"

static uint8_t mhz_digits;
static uint8_t digit_limit;
static uint8_t channel_input;

void FreqEntry_Start(uint8_t channel_mode)
{
    const band_t *band = &bands[menu.band_index];
    uint32_t reference = menu.rf_hz;
    channel_input = channel_mode;
    menu.freq_input_active = 1U;
    menu.freq_input_len = 0U;
    menu.freq_input_dot = 0U;
    menu.freq_input[0] = '\0';
    if (reference >= band->min_hz && reference <= band->max_hz) reference = band->max_hz;
    mhz_digits = (reference >= 10000000UL) ? 2U : 1U;
    digit_limit = (uint8_t)(mhz_digits + 5U);
    if (channel_input != 0U)
    {
        uint32_t count = MEMORY_CHANNEL_COUNT;
        digit_limit = 1U;
        while (count >= 10U) { count /= 10U; digit_limit++; }
    }
    menu.need_redraw = 1U;
}

/* Точки не занимают буфер: они рисуются автоматически. Лишние цифры игнорируются. */
uint8_t FreqEntry_Append(char ch)
{
    if (ch < '0' || ch > '9' || menu.freq_input_len >= digit_limit) return 0U;
    menu.freq_input[menu.freq_input_len++] = ch;
    menu.freq_input[menu.freq_input_len] = '\0';
    menu.need_redraw = 1U;
    return 1U;
}

void FreqEntry_Format(char *text)
{
    uint8_t i;
    if (channel_input == 0U)
    { Frequency_FormatDigits(menu.freq_input, mhz_digits, text); return; }
    for (i = 0U; i < menu.freq_input_len; i++) text[i] = menu.freq_input[i];
    if (i == 0U) text[i++] = '_';
    text[i] = '\0';
}

uint8_t FreqEntry_Parse(uint32_t *hz)
{
    return Frequency_ParseDigits(menu.freq_input, mhz_digits, hz);
}

void FreqEntry_Cancel(void)
{
    menu.freq_input_active = 0U;
    menu.need_redraw = 1U;
}
