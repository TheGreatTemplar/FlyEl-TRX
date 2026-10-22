/* Рамки, подписи и строки меню. */
#include "ui_widgets.h"
#include "app_state.h"
#include "text_utils.h"
#include "gmg12864.h"

/* Рисует строку сервисного меню с выделением выбранного пункта. */
void Widgets_MenuLine(uint8_t y, const char *left, const char *right, uint8_t selected)
{
    if (selected != 0U)
    {
        uint8_t inv_y;

        inv_y = (uint8_t)(y + 2U);

        LCD_FillRectangle(0U, inv_y, 128U, 9U, PX_ON);
        LCD_Print(4U, y + 1U, left, 1U, CH_INVERT);
        LCD_Print(50U, y + 1U, right, 1U, CH_INVERT);
    }
    else
    {
        LCD_Print(4U, y + 1U, left, 1U, CH_UNINVERT);
        LCD_Print(50U, y + 1U, right, 1U, CH_UNINVERT);
    }
}

static void ClearBoxCorners(uint8_t x, uint8_t y, uint8_t width, uint8_t height)
{
    if ((width == 0U) || (height == 0U))
    {
        return;
    }

    LCD_DrawPixel(x, y, PX_OFF);
    LCD_DrawPixel((uint8_t)(x + width - 1U), y, PX_OFF);
    LCD_DrawPixel(x, (uint8_t)(y + height - 1U), PX_OFF);
    LCD_DrawPixel((uint8_t)(x + width - 1U), (uint8_t)(y + height - 1U), PX_OFF);
}

static void DrawFrameNoCorners(uint8_t x, uint8_t y, uint8_t width, uint8_t height)
{
    if ((width < 2U) || (height < 2U))
    {
        return;
    }

    LCD_DrawLine((uint8_t)(x + 1U), y, (uint8_t)(x + width - 2U), y, PX_ON);
    LCD_DrawLine((uint8_t)(x + 1U), (uint8_t)(y + height - 1U), (uint8_t)(x + width - 2U), (uint8_t)(y + height - 1U), PX_ON);
    LCD_DrawLine(x, (uint8_t)(y + 1U), x, (uint8_t)(y + height - 2U), PX_ON);
    LCD_DrawLine((uint8_t)(x + width - 1U), (uint8_t)(y + 1U), (uint8_t)(x + width - 1U), (uint8_t)(y + height - 2U), PX_ON);
}

/* Рисует подпись 5x7: по одному пустому пикселю внутри рамки с каждой стороны. */
uint8_t Widgets_Label(uint8_t x, uint8_t y, const char *text, uint8_t invert)
{
    uint8_t text_width;
    uint8_t box_width;
    uint8_t text_x;
    const uint8_t box_height = 11U;

    text_width = LCD_GetText5x7Width(text, 1U);
    /* Border + one blank column on either side. */
    box_width = (uint8_t)(text_width + 4U);
    text_x = (uint8_t)(x + 2U);

    if (invert != 0U)
    {
        /*
         * LCD_FillRectangle() uses inclusive width/height arguments, so
         * subtract one here. This restores the one-pixel blank gap to
         * the right and below the TX badge.
         */
        LCD_FillRectangle(x, y, (uint8_t)(box_width - 1U), (uint8_t)(box_height - 1U), PX_ON);
        ClearBoxCorners(x, y, box_width, box_height);
        LCD_Print5x7(text_x, (uint8_t)(y + 2U), text, 1U, CH_INVERT);

        /* Keep a blank column and row outside the inverted badge. */
        LCD_DrawLine((uint8_t)(x + box_width), y,
                     (uint8_t)(x + box_width), (uint8_t)(y + box_height - 1U), PX_OFF);
        LCD_DrawLine(x, (uint8_t)(y + box_height),
                     (uint8_t)(x + box_width - 1U), (uint8_t)(y + box_height), PX_OFF);
    }
    else
    {
        LCD_Print5x7(text_x, (uint8_t)(y + 2U), text, 1U, CH_UNINVERT);
        DrawFrameNoCorners(x, y, box_width, box_height);
    }

    return box_width;
}

/* Рисует обычную рамку для SPLIT/RIT. */
uint8_t Widgets_CompactLabel(uint8_t x, uint8_t y, const char *text)
{
    uint8_t text_width;
    uint8_t box_width;
    uint8_t text_x;
    const uint8_t box_height = 11U;

    text_width = LCD_GetText5x7Width(text, 1U);
    /* Border + one blank column on either side. */
    box_width = (uint8_t)(text_width + 4U);
    text_x = (uint8_t)(x + 2U);

    LCD_Print5x7(text_x, (uint8_t)(y + 2U), text, 1U, CH_UNINVERT);
    DrawFrameNoCorners(x, y, box_width, box_height);

    return box_width;
}

static void FormatGainLabel(char *buf)
{
    char part[8];
    int8_t gain_db;

    gain_db = menu.gain_db;
    buf[0] = '\0';

    /* PREAMP and ATT are both OFF. */
    if (gain_db == 0)
    {
        Text_Append(buf, "ATT");
        return;
    }

    if (gain_db < 0)
    {
        Text_Append(buf, "-");
        gain_db = (int8_t)(-gain_db);
    }
    else
    {
        Text_Append(buf, "+");
    }

    Text_U32ToDec(part, (uint32_t)gain_db, 1U);
    Text_Append(buf, part);
    Text_Append(buf, "dB");
}

/* Рисует текущую подпись усиления/ATT; аппаратное управление пока не подключено. */
void Widgets_Gain(uint8_t x, uint8_t y)
{
    char text[12];
    uint8_t text_width;
    uint8_t box_width;
    uint8_t text_x;
    const uint8_t box_height = 11U;

    FormatGainLabel(text);

    text_width = LCD_GetText5x7Width(text, 1U);
    box_width = (uint8_t)(text_width + 4U);
    text_x = (uint8_t)(x + 2U);

    if (menu.gain_db != 0)
    {
        /* PREAMP +20 dB or active attenuator: inverted badge. */
        LCD_FillRectangle(x, y, (uint8_t)(box_width - 1U),
                          (uint8_t)(box_height - 1U), PX_ON);
        ClearBoxCorners(x, y, box_width, box_height);
        LCD_Print5x7(text_x, (uint8_t)(y + 2U), text, 1U, CH_INVERT);

        /* Keep the one-pixel blank gap outside the badge. */
        LCD_DrawLine((uint8_t)(x + box_width), y,
                     (uint8_t)(x + box_width),
                     (uint8_t)(y + box_height - 1U), PX_OFF);
        LCD_DrawLine(x, (uint8_t)(y + box_height),
                     (uint8_t)(x + box_width - 1U),
                     (uint8_t)(y + box_height), PX_OFF);
    }
    else
    {
        /* PREAMP and ATT are OFF: plain ATT in a non-inverted frame. */
        LCD_Print5x7(text_x, (uint8_t)(y + 2U), text, 1U, CH_UNINVERT);
        DrawFrameNoCorners(x, y, box_width, box_height);
    }
}
