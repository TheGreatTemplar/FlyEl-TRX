/* Крупные цифровые шрифты: Terminus и отдельные растры SW2019 17/15. */
#include "gmg12864.h"
#include "font_terminus_15_digi.h"
#include "font_terminus_10x19_digi.h"
#include "../font/font_sw2019_refined_digi.h"

static uint8_t LCD_FindSymbol10x15(uint8_t symbol,
                                   uint16_t *symbol_offset,
                                   uint8_t *symbol_width)
{
    uint8_t i;

    for (i = 0U; i < FONT_TERMINUS_10X15__LENGTH; i++)
    {
        if (font_terminus_10x15_map[i] == symbol)
        {
            *symbol_offset = (uint16_t)i * FONT_TERMINUS_10X15__CHAR_HEIGHT * 2U;
            *symbol_width = font_terminus_10x15_width[i];
            return 1U;
        }
    }

    return 0U;
}

static uint8_t LCD_GetSymbol10x15Width(uint8_t symbol)
{
    uint16_t symbol_offset;
    uint8_t symbol_width;

    if (symbol == (uint8_t)' ')
    {
        return 5U;
    }

    if (LCD_FindSymbol10x15(symbol, &symbol_offset, &symbol_width) == 0U)
    {
        return 0U;
    }

    return symbol_width;
}

void LCD_DrawDigit10x15(uint8_t x_pos, uint8_t y_pos, uint8_t symbol, uint8_t invert)
{
    uint8_t px_ch;
    uint8_t px_bg;
    uint8_t x;
    uint8_t y;
    uint16_t symbol_offset;
    uint8_t symbol_width;

    /*
     * Большой шрифт содержит:
     * '0'...'9', '.', 'V', 'M', 'k', 'H', 'z', 'G'.
     *
     * Точка имеет ширину 6 пикселей.
     * Остальные символы имеют ширину 10 пикселей.
     */
    if (LCD_FindSymbol10x15(symbol, &symbol_offset, &symbol_width) == 0U)
    {
        return;
    }

    if ((invert & CH_INVERT) == 0U)
    {
        px_ch = PX_ON;
        px_bg = PX_OFF;
    }
    else
    {
        px_ch = PX_OFF;
        px_bg = PX_ON;
    }

    for (y = 0U; y < FONT_TERMINUS_10X15__CHAR_HEIGHT; y++)
    {
        uint8_t left;
        uint8_t right;

        left = font_terminus_10x15_[symbol_offset + ((uint16_t)y * 2U)];
        right = font_terminus_10x15_[symbol_offset + ((uint16_t)y * 2U) + 1U];

        for (x = 0U; x < symbol_width; x++)
        {
            uint8_t bit_is_set;

            if (x < 2U)
            {
                /*
                 * Первые два пикселя строки лежат в right[1:0].
                 */
                bit_is_set = (uint8_t)((right >> (1U - x)) & 0x01U);
            }
            else
            {
                /*
                 * Остальные восемь пикселей лежат в left[7:0].
                 */
                bit_is_set = (uint8_t)((left >> (9U - x)) & 0x01U);
            }

            if (bit_is_set != 0U)
            {
                LCD_DrawPixel((uint8_t)(x_pos + x), (uint8_t)(y_pos + y), px_ch);
            }
            else
            {
                LCD_DrawPixel((uint8_t)(x_pos + x), (uint8_t)(y_pos + y), px_bg);
            }
        }
    }
}

void LCD_PrintDigits10x15(uint8_t x_pos, uint8_t y_pos, const char *text, uint8_t spacing, uint8_t invert)
{
    uint8_t symbol;
    uint8_t symbol_width;

    if (text == 0)
    {
        return;
    }

    while (*text != '\0')
    {
        symbol = (uint8_t)*text;
        symbol_width = LCD_GetSymbol10x15Width(symbol);

        if (symbol_width != 0U)
        {
            if (symbol != (uint8_t)' ')
            {
                LCD_DrawDigit10x15(x_pos, y_pos, symbol, invert);
            }

            x_pos = (uint8_t)(x_pos + symbol_width + spacing);
        }

        text++;
    }
}

static uint8_t LCD_FindSymbol10x19(uint8_t symbol,
                                   uint16_t *symbol_offset,
                                   uint8_t *symbol_width)
{
    uint8_t i;

    for (i = 0U; i < FONT_TERMINUS_10X19_DIGI_LENGTH; i++)
    {
        if (font_terminus_10x19_digi_map[i] == symbol)
        {
            *symbol_offset = (uint16_t)i * FONT_TERMINUS_10X19_DIGI_CHAR_HEIGHT * 2U;
            *symbol_width = font_terminus_10x19_digi_width[i];
            return 1U;
        }
    }

    return 0U;
}

static uint8_t LCD_GetSymbol10x19Width(uint8_t symbol)
{
    uint16_t symbol_offset;
    uint8_t symbol_width;

    if (symbol == (uint8_t)' ')
    {
        return 5U;
    }

    if (LCD_FindSymbol10x19(symbol, &symbol_offset, &symbol_width) == 0U)
    {
        return 0U;
    }

    return symbol_width;
}

void LCD_DrawDigit10x19(uint8_t x_pos, uint8_t y_pos, uint8_t symbol, uint8_t invert)
{
    uint8_t px_ch;
    uint8_t px_bg;
    uint8_t x;
    uint8_t y;
    uint16_t symbol_offset;
    uint8_t symbol_width;

    if (LCD_FindSymbol10x19(symbol, &symbol_offset, &symbol_width) == 0U)
    {
        return;
    }

    if ((invert & CH_INVERT) == 0U)
    {
        px_ch = PX_ON;
        px_bg = PX_OFF;
    }
    else
    {
        px_ch = PX_OFF;
        px_bg = PX_ON;
    }

    for (y = 0U; y < FONT_TERMINUS_10X19_DIGI_CHAR_HEIGHT; y++)
    {
        uint8_t left;
        uint8_t right;

        left = font_terminus_10x19_digi[symbol_offset + ((uint16_t)y * 2U)];
        right = font_terminus_10x19_digi[symbol_offset + ((uint16_t)y * 2U) + 1U];

        for (x = 0U; x < symbol_width; x++)
        {
            uint8_t bit_is_set;

            /*
             * Raster storage is 10 bits wide even for proportional glyphs.
             * Bits 9..8 are in right[1:0], bits 7..0 are in left[7:0].
             */
            if (x < 2U)
            {
                bit_is_set = (uint8_t)((right >> (1U - x)) & 0x01U);
            }
            else
            {
                bit_is_set = (uint8_t)((left >> (9U - x)) & 0x01U);
            }

            if (bit_is_set != 0U)
            {
                LCD_DrawPixel((uint8_t)(x_pos + x), (uint8_t)(y_pos + y), px_ch);
            }
            else
            {
                LCD_DrawPixel((uint8_t)(x_pos + x), (uint8_t)(y_pos + y), px_bg);
            }
        }
    }
}

void LCD_PrintDigits10x19(uint8_t x_pos, uint8_t y_pos, const char *text, uint8_t spacing, uint8_t invert)
{
    uint8_t symbol;
    uint8_t symbol_width;

    if (text == 0)
    {
        return;
    }

    while (*text != '\0')
    {
        symbol = (uint8_t)*text;
        symbol_width = LCD_GetSymbol10x19Width(symbol);

        if (symbol_width != 0U)
        {
            if (symbol != (uint8_t)' ')
            {
                LCD_DrawDigit10x19(x_pos, y_pos, symbol, invert);
            }

            x_pos = (uint8_t)(x_pos + symbol_width + spacing);
        }

        text++;
    }
}

uint8_t LCD_GetDigits10x19Width(const char *text, uint8_t spacing)
{
    uint16_t width;
    uint8_t symbol_width;

    if (text == 0)
    {
        return 0U;
    }

    width = 0U;

    while (*text != '\0')
    {
        symbol_width = LCD_GetSymbol10x19Width((uint8_t)*text);

        if (symbol_width != 0U)
        {
            width += (uint16_t)symbol_width + spacing;
        }

        text++;
    }

    if (width > 255U)
    {
        width = 255U;
    }

    return (uint8_t)width;
}

uint8_t LCD_GetDigits10x15Width(const char *text, uint8_t spacing)
{
    uint16_t width;
    uint8_t symbol_width;

    if (text == 0)
    {
        return 0U;
    }

    width = 0U;

    while (*text != '\0')
    {
        symbol_width = LCD_GetSymbol10x15Width((uint8_t)*text);

        if (symbol_width != 0U)
        {
            width += (uint16_t)symbol_width + spacing;
        }

        text++;
    }

    if (width > 255U)
    {
        width = 255U;
    }

    return (uint8_t)width;
}

/* Выбирает готовый растр 17 или 15 пикселей; масштабирования на лету нет. */
void LCD_DrawDigitSW2019(uint8_t x, uint8_t y, uint8_t symbol, uint8_t height, uint8_t invert)
{
    const uint8_t *glyph;
    uint8_t row;
    uint8_t col;
    if (symbol < (uint8_t)'0' || symbol > (uint8_t)'9')
    {
        return;
    }
    if (height == 17U)
    {
        glyph = font_sw2019_refined_17[symbol - (uint8_t)'0'];
    }
    else if (height == 15U)
    {
        glyph = font_sw2019_refined_15[symbol - (uint8_t)'0'];
    }
    else
    {
        return;
    }
    for (row = 0U; row < height; row++)
    {
        for (col = 0U; col < 10U; col++)
        {
            uint8_t on = (col < 8U) ? (uint8_t)((glyph[row] >> (7U - col)) & 1U) : 0U;
            if ((uint16_t)x + col < LCD_WIDTH && (uint16_t)y + row < LCD_HEIGHT)
            {
                LCD_DrawPixel((uint8_t)(x + col), (uint8_t)(y + row),
                              (uint8_t)(on ^ ((invert & CH_INVERT) != 0U)));
            }
        }
    }
}
