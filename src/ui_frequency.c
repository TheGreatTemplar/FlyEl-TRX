/* Форматирование и отрисовка частот; размеры и интервалы задаются здесь. */
#include "ui_frequency.h"
#include "app_state.h"
#include "text_utils.h"
#include "gmg12864.h"

static void FormatMainFrequency(char *large, char *small, uint32_t hz)
{
    char part[12];

    /*
     * Main frequency display:
     * 3650123 Hz -> large = "3.650.", small = "12"
     *
     * The 1 Hz digit is deliberately not displayed. The final two visible
     * digits are hundreds and tens of hertz and are drawn with the 10x15
     * font. The MHz/kHz part is drawn with the new 10x19 font.
     */
    large[0] = '\0';

    Text_U32ToDec(part, hz / 1000000UL, 1U);
    Text_Append(large, part);
    Text_Append(large, ".");

    Text_U32ToDec(part, (hz / 1000UL) % 1000UL, 3U);
    Text_Append(large, part);
    Text_Append(large, ".");

    Text_U32ToDec(small, (hz / 10UL) % 100UL, 2U);
}

static uint8_t GetMainLargeWidth(const char *text)
{
    uint8_t width;
    uint8_t symbol_width;

    width = 0U;

    while (*text != '\0')
    {
        if (*text == '.')
        {
            symbol_width = 2U;
        }
        else
        {
            symbol_width = 10U;
        }

        width = (uint8_t)(width + symbol_width + 1U);
        text++;
    }

    /*
     * Every glyph gets one explicit blank column. The large digits already
     * have one blank edge column, so the visible digit-to-dot and
     * dot-to-digit gaps are 2 pixels. The last +1 is also the gap from the
     * final decimal point to the 10x15 hundreds/tens-of-hertz digits.
     */
    return width;
}

static uint8_t DrawMainLargeFrequency(uint8_t x, uint8_t y, const char *text)
{
    uint8_t symbol_width;

    while (*text != '\0')
    {
        LCD_DrawDigit10x19(x, y, (uint8_t)*text, CH_UNINVERT);

        if (*text == '.')
        {
            symbol_width = 2U;
        }
        else
        {
            symbol_width = 10U;
        }

        x = (uint8_t)(x + symbol_width + 1U);
        text++;
    }

    return x;
}

static uint8_t GetSW2019FrequencyWidth(const char *text)
{
    uint8_t width = 0U;
    while (*text != '\0')
    {
        width = (uint8_t)(width + ((*text == '.') ? 2U : 10U));
        if (*text == '.')
        {
            width++;
        }
        text++;
        if (*text != '\0' && *text != '.')
        {
            width++;
        }
    }
    return width;
}

static void DrawSW2019Frequency(uint8_t x, uint8_t y, const char *text)
{
    uint8_t dots = 0U;
    while (*text != '\0')
    {
        if (*text == '.')
        {
            /* 2x2 dot ends on the same row as the visible digit body. */
            LCD_FillRectangle(x, (uint8_t)(y + 13U), 1U, 1U, PX_ON);
            x = (uint8_t)(x + 4U);
            dots++;
        }
        else
        {
            uint8_t height = (dots < 2U) ? 17U : 15U;
            /* Grow the MHz/kHz digits upward, keeping the common baseline. */
            uint8_t top = (dots < 2U && y >= 2U) ? (uint8_t)(y - 2U) : y;
            LCD_DrawDigitSW2019(x, top, (uint8_t)*text, height, CH_UNINVERT);
            x = (uint8_t)(x + ((*(text + 1) == '.') ? 10U : 11U));
        }
        text++;
    }
}

/* Вычисляет левую координату основной частоты для выравнивания вправо. */
uint8_t FrequencyUI_MainLeftX(uint32_t hz)
{
    char large[12];
    char small[3];
    uint8_t large_width;
    uint8_t small_width;
    uint8_t total_width;

    FormatMainFrequency(large, small, hz);

    if (MAIN_FREQ_USE_SW2019 != 0U)
    {
        Text_Append(large, small);
        total_width = GetSW2019FrequencyWidth(large);
        /* The supplied 10-pixel cells have two empty right columns. */
        return (total_width < LCD_WIDTH) ? (uint8_t)(LCD_WIDTH + 2U - total_width) : 0U;
    }


    large_width = GetMainLargeWidth(large);
    small_width = LCD_GetDigits10x15Width(small, 1U);

    if (small_width != 0U)
    {
        small_width--;
    }

    total_width = (uint8_t)(large_width + small_width);

    if (total_width < 128U)
    {
        /* Shift the complete frequency one pixel to the right so the
         * last visible pixel reaches display column 127. */
        return (uint8_t)(129U - total_width);
    }

    return 0U;
}

/* Рисует основную частоту; SW2019 использует высоты 17/15 пикселей. */
void FrequencyUI_DrawMain(uint32_t hz, uint8_t y)
{
    char large[12];
    char small[3];
    uint8_t large_x;
    uint8_t small_x;

    FormatMainFrequency(large, small, hz);

    large_x = FrequencyUI_MainLeftX(hz);

    if (MAIN_FREQ_USE_SW2019 != 0U)
    {
        Text_Append(large, small);
        DrawSW2019Frequency(large_x, y, large);
        return;
    }


    /* The 10x19 raster has an empty first row. */
    small_x = DrawMainLargeFrequency(large_x, y, large);

    /* Bottom-align the 10x15 hundreds/tens-of-hertz digits with 10x19. */
    LCD_PrintDigits10x15(small_x, (uint8_t)(y + 4U), small, 1U, CH_UNINVERT);
}

static uint8_t VfoBGlyphWidth(char ch, uint8_t dots)
{
    return (ch == '.') ? 2U : ((dots >= 2U) ? 5U : 7U);
}

static uint8_t VfoBGlyphGap(char current, char next, uint8_t dots)
{
    if (next == '\0')
    {
        return 0U;
    }
    if (current == '.')
    {
        return (dots >= 2U) ? 2U : 1U;
    }
    if (next == '.')
    {
        return 1U;
    }
    return (dots >= 2U) ? 1U : 0U;
}

static uint8_t GetVfoBFrequencyWidth(uint32_t hz)
{
    char text[12];
    char small[3];
    const char *p;
    uint8_t width = 0U;
    uint8_t dots = 0U;
    FormatMainFrequency(text, small, hz);
    Text_Append(text, small);
    for (p = text; *p != '\0'; p++)
    {
        width = (uint8_t)(width + VfoBGlyphWidth(*p, dots));
        if (*p == '.')
        {
            dots++;
        }
        width = (uint8_t)(width + VfoBGlyphGap(*p, *(p + 1), dots));
    }
    return width;
}

/* Вычисляет левую координату строки второго VFO. */
uint8_t FrequencyUI_SecondLeftX(uint32_t hz)
{
    uint8_t width = GetVfoBFrequencyWidth(hz);
    return (width < LCD_WIDTH) ? (uint8_t)(LCD_WIDTH - width) : 0U;
}

/* Рисует VFO B: старшие разряды 7x11, последние два — 5x7. */
void FrequencyUI_DrawSecond(uint32_t hz, uint8_t y)
{
    char text[12];
    char small[3];
    const char *p;
    uint8_t x;
    uint8_t dots = 0U;
    FormatMainFrequency(text, small, hz);
    Text_Append(text, small);
    x = FrequencyUI_SecondLeftX(hz);
    for (p = text; *p != '\0'; p++)
    {
        uint8_t glyph_width = VfoBGlyphWidth(*p, dots);
        if (*p == '.')
        {
            LCD_FillRectangle(x, (uint8_t)(y + 8U), 1U, 1U, PX_ON);
            dots++;
        }
        else if (dots >= 2U)
        {
            char glyph[2];
            glyph[0] = *p;
            glyph[1] = '\0';
            /* The 7x11 digit body ends at y+9; 5x7 ends there too. */
            LCD_Print5x7(x, (uint8_t)(y + 3U), glyph, 1U, CH_UNINVERT);
        }
        else
        {
            LCD_DrawDigit7x11(x, y, (uint8_t)*p, CH_UNINVERT);
        }
        x = (uint8_t)(x + glyph_width + VfoBGlyphGap(*p, *(p + 1), dots));
    }
}

static void FormatRfKHzParts(char *value, char *unit, uint32_t hz)
{
    char part[12];

    /*
     * Для меню FREQ:
     * 18159200 Hz -> value = "18159.200", unit = "kHz".
     * Единицы потом рисуются отдельно с пиксельным зазором.
     */
    value[0] = '\0';

    Text_U32ToDec(part, hz / 1000UL, 1U);
    Text_Append(value, part);
    Text_Append(value, ".");

    Text_U32ToDec(part, hz % 1000UL, 3U);
    Text_Append(value, part);

    unit[0] = 'k';
    unit[1] = 'H';
    unit[2] = 'z';
    unit[3] = '\0';
}

/* Рисует числовую частоту в кГц для сервисного меню. */
void FrequencyUI_DrawKHz(uint8_t x, uint8_t y, uint32_t hz, uint8_t invert)
{
    char value[16];
    char unit[5];

    FormatRfKHzParts(value, unit, hz);

    LCD_Print(x, y, value, 1U, invert);

    /*
     * Позиции рассчитаны для текущего малого шрифта и формата kHz.
     * Зазор между числом и единицами около 3...4 пикселей.
     */
    if (hz >= 10000000UL)
    {
        /* Например 18159.200 -> 9 символов. */
        LCD_Print(104U, y, unit, 1U, invert);
    }
    else if (hz >= 1000000UL)
    {
        /* Например 3650.000 или 7100.000 -> 8 символов. */
        LCD_Print(98U, y, unit, 1U, invert);
    }
    else
    {
        /* Резерв для частот ниже 1 MHz. */
        LCD_Print(92U, y, unit, 1U, invert);
    }
}

/* Форматирует выбранную ПЧ в МГц для экрана меню. */
void FrequencyUI_FormatIF(char *buf)
{
    char part[12];
    uint32_t hz;

    hz = if_list_hz[menu.if_index];

    /*
     * Формат IF:
     * 4.6080 MHz
     * 8.8600 MHz
     * 11.0592 MHz
     */
    buf[0] = '\0';

    Text_U32ToDec(part, hz / 1000000UL, 1U);
    Text_Append(buf, part);
    Text_Append(buf, ".");

    Text_U32ToDec(part, (hz / 100UL) % 10000UL, 4U);
    Text_Append(buf, part);
    Text_Append(buf, "M");
}
