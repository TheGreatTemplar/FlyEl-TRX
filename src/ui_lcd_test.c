/* Чистые полноэкранные рисунки: подписи не закрывают проверяемые пиксели матрицы. */
#include "ui_service_tools.h"
#include "service_lcd_test.h"
#include "gmg12864.h"

static void DrawGrid(uint8_t page)
{
    uint8_t x;
    uint8_t y;
    for (y = 0U; y < LCD_HEIGHT; y++)
    {
        for (x = 0U; x < LCD_WIDTH; x++)
        {
            uint8_t on;
            if (page == LCD_TEST_CHECK || page == LCD_TEST_CHECK_INV)
                on = (uint8_t)((x + y + (page == LCD_TEST_CHECK_INV ? 1U : 0U)) % 2U);
            else if (page == LCD_TEST_VERTICAL || page == LCD_TEST_VERTICAL_INV)
                on = (uint8_t)((x + (page == LCD_TEST_VERTICAL_INV ? 1U : 0U)) % 2U);
            else
                on = (uint8_t)((y + (page == LCD_TEST_HORIZONTAL_INV ? 1U : 0U)) % 2U);
            LCD_DrawPixel(x, y, on);
        }
    }
}


static void DrawMainFonts(void)
{
    uint8_t digit;
    LCD_Print5x7(0U, 0U, "SW2019 17 / 15", 1U, CH_UNINVERT);
    for (digit = 0U; digit < 10U; digit++)
    {
        uint8_t symbol = (uint8_t)('0' + digit);
        LCD_DrawDigitSW2019((uint8_t)(digit * 11U), 12U, symbol, 17U, CH_UNINVERT);
        LCD_DrawDigitSW2019((uint8_t)(digit * 11U), 36U, symbol, 15U, CH_UNINVERT);
    }
}


static void DrawSmallFonts(void)
{
    uint8_t digit;
    LCD_Print5x7(0U, 0U, "VFOB / CH / SMALL", 1U, CH_UNINVERT);
    LCD_PrintDigits7x11(0U, 11U, "0123456789", 1U, CH_UNINVERT);
    for (digit = 0U; digit < 10U; digit++)
        LCD_DrawDigit8x9((uint8_t)(digit * 9U), 28U, (uint8_t)('0' + digit), CH_UNINVERT);
    LCD_Print5x7(0U, 43U, "0123456789 ABCDE", 1U, CH_UNINVERT);
    LCD_PrintDigits4x6(0U, 55U, "0123456789", 1U, CH_UNINVERT);
}


static void DrawOtherFonts(void)
{
    LCD_Print5x7(0U, 0U, "TAHOMA / TERMINUS", 1U, CH_UNINVERT);
    LCD_Print(0U, 10U, "FlyEl 0123456789", 1U, CH_UNINVERT);
    LCD_PrintDigits10x19(0U, 24U, "0123456789", 1U, CH_UNINVERT);
    LCD_PrintDigits10x15(0U, 46U, "0123456789", 1U, CH_UNINVERT);
}


/* Содержимое LCD обновляется обычным драйвером; команды поворота/контраста не выдаются. */
void ServiceTools_DrawLCDTest(void)
{
    uint8_t page = LCDTest_Page();
    LCD_ClearBuffer();
    if (page == LCD_TEST_ON)
        LCD_FillRectangle(0U, 0U, 127U, 63U, PX_ON);
    else if (page >= LCD_TEST_CHECK && page <= LCD_TEST_HORIZONTAL_INV)
        DrawGrid(page);
    else if (page == LCD_TEST_GEOMETRY)
    {
        LCD_DrawRectangle(0U, 0U, 127U, 63U, PX_ON);
        LCD_DrawLine(0U, 0U, 127U, 63U, PX_ON);
        LCD_DrawLine(0U, 63U, 127U, 0U, PX_ON);
    }
    else if (page == LCD_TEST_SWEEP_X)
        LCD_DrawLine(LCDTest_Position(), 0U, LCDTest_Position(), 63U, PX_ON);
    else if (page == LCD_TEST_SWEEP_Y)
        LCD_DrawLine(0U, LCDTest_Position(), 127U, LCDTest_Position(), PX_ON);
    else if (page == LCD_TEST_FONT_MAIN)
        DrawMainFonts();
    else if (page == LCD_TEST_FONT_SMALL)
        DrawSmallFonts();
    else if (page == LCD_TEST_FONT_OTHER)
        DrawOtherFonts();
    LCD_Update();
}
