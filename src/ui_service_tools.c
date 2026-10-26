/* Редактор UART и общий диагностический экран клавиатуры 14.13 / 16.11. */
#include "ui_service_tools.h"
#include "service_uart.h"
#include "service_keytest.h"
#include "text_utils.h"
#include "gmg12864.h"

void ServiceTools_DrawUart(void)
{
    uint8_t row, selected = ServiceUart_Item();
    uint8_t first = (uint8_t)((selected / 4U) * 4U);
    char value[16];
    LCD_ClearBuffer();
    LCD_Print5x7(0U, 0U, "UART CONFIG", 1U, CH_UNINVERT);
    {
        const char *status = (ServiceUart_Editing() != 0U) ? "EDIT" : "15.04";
        LCD_Print5x7((uint8_t)(LCD_WIDTH - LCD_GetText5x7Width(status, 1U)), 0U, status, 1U, CH_UNINVERT);
    }
    for (row = 0U; row < 4U && (uint16_t)first + row < 6U; row++)
    {
        uint8_t index = (uint8_t)(first + row);
        uint8_t y = (uint8_t)(12U + row * 10U);
        uint8_t invert = (index == selected) ? CH_INVERT : CH_UNINVERT;
        if (index == selected)
            LCD_FillRectangle(0U, (uint8_t)(y - 1U), 127U, 8U, PX_ON);
        ServiceUart_Value(index, value);
        LCD_Print5x7(1U, y, ServiceUart_Label(index), 1U, invert);
        LCD_Print5x7(60U, y, value, 1U, invert);
    }
    LCD_Print5x7(0U, 56U, ServiceUart_Status(), 1U, CH_UNINVERT);
    LCD_Update();
}


static void HexMask(char *text, uint32_t value)
{
    uint8_t i;
    static const char digits[] = "0123456789ABCDEF";
    for (i = 0U; i < 7U; i++)
        text[i] = digits[(value >> ((6U - i) * 4U)) & 15U];
    text[7] = '\0';
}


void ServiceTools_DrawKeyboard(void)
{
    static const char *const events[] = {"NONE", "DOWN", "SHORT", "LONG"};
    char text[24], number[12];
    input_key_t key = KeyTest_LastKey();
    LCD_ClearBuffer();
    LCD_Print5x7(0U, 0U, "KEYBOARD TEST", 1U, CH_UNINVERT);
    LCD_Print5x7(0U, 11U, (key == INPUT_KEY_COUNT) ? "PRESS A KEY" : Input_KeyName(key), 1U, CH_UNINVERT);
    text[0] = '\0';
    Text_Append(text, events[KeyTest_Event()]);
    Text_Append(text, " ");
    Text_U32ToDec(number, KeyTest_DurationMs(), 1U);
    Text_Append(text, number);
    Text_Append(text, " MS");
    LCD_Print5x7(0U, 22U, text, 1U, CH_UNINVERT);
    text[0] = '\0';
    Text_Append(text, "RAW ");
    HexMask(number, KeyTest_RawMask());
    Text_Append(text, number);
    LCD_Print5x7(0U, 33U, text, 1U, CH_UNINVERT);
    text[0] = '\0';
    Text_Append(text, "KEY ");
    HexMask(number, KeyTest_DownMask());
    Text_Append(text, number);
    LCD_Print5x7(0U, 44U, text, 1U, CH_UNINVERT);
    LCD_Print5x7(0U, 56U, (KeyTest_Ready() != 0U) ? "UP OR NR HOLD 2S:OUT" : "RELEASE ALL KEYS", 1U, CH_UNINVERT);
    LCD_Update();
}
