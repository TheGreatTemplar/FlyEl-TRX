/* Текст Tahoma и 5x7; состояние межсимвольного интервала принадлежит этому модулю. */
#include "gmg12864.h"
#include "../font/font_tahoma_8_prop.h"
#include "../font/font_status_5x7.h"

static uint8_t lcd_char_spacing;

/* Рисует символ Tahoma, сохраняя его ширину для последующего LCD_Print. */
void LCD_DrawChar8(uint8_t x_pos, uint8_t y_pos, uint8_t symbol, uint8_t invert)
{
    uint8_t px_ch;
    uint8_t px_bg;
    uint8_t ch_width;
    uint8_t x;
    uint8_t y;
    uint16_t index;

    if (symbol < 32U)
    {
        symbol = (uint8_t)' ';
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

    index = (uint16_t)(symbol - 0x20U) * (uint16_t)(1U + FONT_TAHOMA_8_CHAR_HEIGHT);
    ch_width = (uint8_t)(font_tahoma_8[index] + 1U);
    lcd_char_spacing = ch_width;

    for (y = 1U; y <= FONT_TAHOMA_8_CHAR_HEIGHT; y++)
    {
        for (x = 0U; x < ch_width; x++)
        {
            if (((font_tahoma_8[index + y] >> (7U - x)) & 0x01U) != 0U)
            {
                LCD_DrawPixel((uint8_t)(x_pos + x), (uint8_t)(y_pos + y), px_ch);
            }
            else
            {
                LCD_DrawPixel((uint8_t)(x_pos + x), (uint8_t)(y_pos + y), px_bg);
            }
        }

        if (ch_width < FONT_TAHOMA_8_CHAR_WIDTH)
        {
            uint8_t n;

            for (n = ch_width; n < (FONT_TAHOMA_8_CHAR_WIDTH + 1U); n++)
            {
                LCD_DrawPixel((uint8_t)(x_pos + n), (uint8_t)(y_pos + y), px_bg);
            }
        }
    }
}


/* Печатает Tahoma из нуль-терминированной строки; инверсия меняет цвет букв/фона. */
void LCD_Print(uint8_t x_pos, uint8_t y_pos, const char *text, uint8_t spacing, uint8_t invert)
{
    uint8_t symbol;

    if (text == 0)
    {
        return;
    }

    while (*text != '\0')
    {
        symbol = (uint8_t)*text;
        LCD_DrawChar8(x_pos, y_pos, symbol, invert);

        x_pos = (uint8_t)(x_pos + lcd_char_spacing - 1U + spacing);
        text++;
    }
}


static const font_status_5x7_glyph_t *LCD_FindStatusGlyph(uint8_t symbol)
{
    uint32_t i;
    for (i = 0U; i < sizeof(font_status_5x7) / sizeof(font_status_5x7[0]); i++)
    {
        if (font_status_5x7[i].symbol == symbol)
        {
            return &font_status_5x7[i];
        }
    }
    return &font_status_5x7[0];
}


uint8_t LCD_GetText5x7Width(const char *text, uint8_t spacing)
{
    uint32_t width = 0U;
    if (text == 0 || *text == '\0')
    {
        return 0U;
    }
    while (*text != '\0')
    {
        width += LCD_FindStatusGlyph((uint8_t)*text)->width;
        text++;
        if (*text != '\0')
        {
            width += spacing;
        }
    }
    return (uint8_t)((width > LCD_WIDTH) ? LCD_WIDTH : width);
}


/* Печатает компактные статусные подписи с отсечением по границам дисплея. */
void LCD_Print5x7(uint8_t x, uint8_t y, const char *text, uint8_t spacing, uint8_t invert)
{
    uint16_t cursor = x;
    if (text == 0)
    {
        return;
    }
    while (*text != '\0' && cursor < LCD_WIDTH)
    {
        const font_status_5x7_glyph_t *glyph = LCD_FindStatusGlyph((uint8_t)*text);
        uint8_t row;
        uint8_t col;
        for (row = 0U; row < FONT_STATUS_5X7_HEIGHT; row++)
        {
            for (col = 0U; col < glyph->width; col++)
            {
                uint8_t on = (uint8_t)((glyph->rows[row] >> (glyph->width - 1U - col)) & 1U);
                if ((cursor + col < LCD_WIDTH) && ((uint16_t)y + row < LCD_HEIGHT))
                {
                    LCD_DrawPixel((uint8_t)(cursor + col), (uint8_t)(y + row),
                                  (uint8_t)(on ^ ((invert & CH_INVERT) != 0U)));
                }
            }
        }
        cursor += glyph->width + spacing;
        text++;
    }
}


uint8_t LCD_GetTextWidth(const char *text, uint8_t spacing)
{
    uint8_t width;
    uint8_t symbol;
    uint16_t index;

    if (text == 0)
    {
        return 0U;
    }

    width = 0U;

    while (*text != '\0')
    {
        symbol = (uint8_t)*text;

        if (symbol < 32U)
        {
            symbol = (uint8_t)' ';
        }

        index = (uint16_t)(symbol - 0x20U) * (uint16_t)(1U + FONT_TAHOMA_8_CHAR_HEIGHT);
        width = (uint8_t)(width + font_tahoma_8[index] + spacing);
        text++;
    }

    if (width != 0U)
    {
        width = (uint8_t)(width - spacing + 1U);
    }

    return width;
}
