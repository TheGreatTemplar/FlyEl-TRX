#include "ui_boot.h"
#include "gmg12864.h"

void BootUI_Clear(void)
{
    LCD_ClearBuffer();
}


void BootUI_Line(uint8_t row, const char *text, uint8_t inverse)
{
    if (row >= 7U)
        return;
    /* LCD_FillRectangle включает обе границы: размеры здесь заданы приращениями. */
    LCD_FillRectangle(0U, (uint8_t)(row * 9U), 127U, 6U, inverse ? PX_ON : PX_OFF);
    LCD_Print5x7(0U, (uint8_t)(row * 9U), text, 1U, inverse);
}


/* Спиннер рисуется пикселями: для него не требуется расширять таблицу шрифта. */
void BootUI_Spinner(uint8_t row, uint32_t tick)
{
    uint8_t y = (uint8_t)(row * 9U);
    uint8_t phase = (uint8_t)((tick / 200U) % 4U);
    LCD_FillRectangle(121U, y, 6U, 6U, PX_OFF);
    switch (phase)
    {
        case 0U:
            LCD_DrawLine(124U, y, 124U, (uint8_t)(y + 6U), PX_ON);
            break;
        case 1U:
            LCD_DrawLine(121U, (uint8_t)(y + 6U), 127U, y, PX_ON);
            break;
        case 2U:
            LCD_DrawLine(121U, (uint8_t)(y + 3U), 127U, (uint8_t)(y + 3U), PX_ON);
            break;
        default:
            LCD_DrawLine(121U, y, 127U, (uint8_t)(y + 6U), PX_ON);
            break;
    }
}


void BootUI_Update(void)
{
    LCD_Update();
}
