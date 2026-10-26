/* Цифровые растры второго VFO и шкалы S-метра. */
#include "gmg12864.h"
#include "../font/font_terminus_7x11_digi.h"
#include "../font/font_meter_4x6.h"
#include "../font/font_channel_bold_8x9.h"

/* Жирная цифра 8x9 в framebuffer; вызов из основного цикла, координаты в пикселях. */
void LCD_DrawDigit8x9(uint8_t x, uint8_t y, uint8_t symbol, uint8_t invert)
{
    uint8_t row, col;
    if (symbol < (uint8_t)'0' || symbol > (uint8_t)'9')
        return;
    for (row = 0U; row < FONT_CHANNEL_BOLD_HEIGHT; row++)
    {
        uint8_t bits = font_channel_bold_8x9[symbol - (uint8_t)'0'][row];
        for (col = 0U; col < FONT_CHANNEL_BOLD_WIDTH; col++)
        {
            if ((uint16_t)x + col < LCD_WIDTH && (uint16_t)y + row < LCD_HEIGHT)
            {
                uint8_t on = (uint8_t)((bits >> (7U - col)) & 1U);
                LCD_DrawPixel((uint8_t)(x + col), (uint8_t)(y + row), (uint8_t)(on ^ ((invert & CH_INVERT) != 0U)));
            }
        }
    }
}


static uint8_t LCD_FindSymbol7x11(uint8_t symbol, uint16_t *symbol_offset, uint8_t *symbol_width)
{
    uint8_t i;

    for (i = 0U; i < FONT_TERMINUS_7X11_DIGI_LENGTH; i++)
    {
        if (font_terminus_7x11_digi_map[i] == symbol)
        {
            *symbol_offset = (uint16_t)i * FONT_TERMINUS_7X11_DIGI_CHAR_HEIGHT * 2U;
            *symbol_width = font_terminus_7x11_digi_width[i];
            return 1U;
        }
    }

    return 0U;
}


static uint8_t LCD_GetSymbol7x11Width(uint8_t symbol)
{
    uint16_t symbol_offset;
    uint8_t symbol_width;

    if (LCD_FindSymbol7x11(symbol, &symbol_offset, &symbol_width) == 0U)
    {
        return 0U;
    }

    return symbol_width;
}


void LCD_DrawDigit7x11(uint8_t x_pos, uint8_t y_pos, uint8_t symbol, uint8_t invert)
{
    uint8_t px_ch;
    uint8_t px_bg;
    uint8_t x;
    uint8_t y;
    uint16_t symbol_offset;
    uint8_t symbol_width;

    if (LCD_FindSymbol7x11(symbol, &symbol_offset, &symbol_width) == 0U)
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

    for (y = 0U; y < FONT_TERMINUS_7X11_DIGI_CHAR_HEIGHT; y++)
    {
        uint16_t row_bits;

        row_bits = (uint16_t)font_terminus_7x11_digi[symbol_offset + ((uint16_t)y * 2U)];
        row_bits |= (uint16_t)((uint16_t)font_terminus_7x11_digi[symbol_offset + ((uint16_t)y * 2U) + 1U] << 8U);

        for (x = 0U; x < symbol_width; x++)
        {
            uint8_t bit_is_set;

            bit_is_set = (uint8_t)((row_bits >> (symbol_width - 1U - x)) & 0x01U);

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


void LCD_PrintDigits7x11(uint8_t x_pos, uint8_t y_pos, const char *text, uint8_t spacing, uint8_t invert)
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
        symbol_width = LCD_GetSymbol7x11Width(symbol);

        if (symbol_width != 0U)
        {
            LCD_DrawDigit7x11(x_pos, y_pos, symbol, invert);
            x_pos = (uint8_t)(x_pos + symbol_width + spacing);
        }

        text++;
    }
}


void LCD_DrawDigit4x6(uint8_t x_pos, uint8_t y_pos, uint8_t symbol, uint8_t invert)
{
    uint8_t px_ch;
    uint8_t px_bg;
    uint8_t x;
    uint8_t y;
    uint8_t row;
    uint8_t digit;

    if ((symbol < (uint8_t)'0') || (symbol > (uint8_t)'9'))
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

    digit = (uint8_t)(symbol - (uint8_t)'0');

    for (y = 0U; y < FONT_METER_4X6_HEIGHT; y++)
    {
        row = font_meter_4x6[digit][y];

        for (x = 0U; x < FONT_METER_4X6_WIDTH; x++)
        {
            if (((row >> (3U - x)) & 0x01U) != 0U)
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


void LCD_PrintDigits4x6(uint8_t x_pos, uint8_t y_pos, const char *text, uint8_t spacing, uint8_t invert)
{
    if (text == 0)
    {
        return;
    }

    while (*text != '\0')
    {
        if ((*text >= '0') && (*text <= '9'))
        {
            LCD_DrawDigit4x6(x_pos, y_pos, (uint8_t)*text, invert);
            x_pos = (uint8_t)(x_pos + FONT_METER_4X6_WIDTH + spacing);
        }

        text++;
    }
}


uint8_t LCD_GetDigits4x6Width(const char *text, uint8_t spacing)
{
    uint8_t count;

    if (text == 0)
    {
        return 0U;
    }

    count = 0U;

    while (*text != '\0')
    {
        if ((*text >= '0') && (*text <= '9'))
        {
            count++;
        }

        text++;
    }

    if (count == 0U)
    {
        return 0U;
    }

    return (uint8_t)((count * FONT_METER_4X6_WIDTH) + ((count - 1U) * spacing));
}
