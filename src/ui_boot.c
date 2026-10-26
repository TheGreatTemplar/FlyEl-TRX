#include "ui_boot.h"
#include "gmg12864.h"

void BootUI_Clear(void)
{
    LCD_ClearBuffer();
}


void BootUI_Line(uint8_t row, const char *text, uint8_t inverse)
{
    uint8_t y = (uint8_t)(row * 9U);
    if (row >= 7U)
        return;
    LCD_FillRectangle(0U, y, 127U, 8U, PX_OFF);
    if (inverse != 0U)
    {
        uint16_t width = (uint16_t)LCD_GetText5x7Width(text, 1U) + 1U;
        if (width > 127U)
            width = 127U;
        /* Прямоугольник включает границы: поле 1 пиксель вокруг текста 5x7. */
        LCD_FillRectangle(0U, y, (uint8_t)width, 8U, PX_ON);
    }
    LCD_Print5x7(1U, (uint8_t)(y + 1U), text, 1U, inverse);
}


void BootUI_Update(void)
{
    LCD_Update();
}
