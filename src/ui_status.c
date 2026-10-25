/* Состояния, для которых нет места в левых рамках. Только рисование, без I2C. */
#include "ui_status.h"
#include "radio_panel.h"
#include "radio_vfo.h"
#include "memory_control.h"
#include "memory_store.h"
#include "entry_controller.h"
#include "scan_control.h"
#include "eeprom_startup.h"
#include "key_layout.h"
#include "text_utils.h"
#include "app_state.h"
#include "app_time.h"
#include "gmg12864.h"
#include "ui_layout.h"

const char *StatusUI_Header(void)
{
    if (EEPROM_TestStatus()[0] != '\0') return EEPROM_TestStatus();
    if (Entry_Active() != 0U) return Entry_Status();
    if (MemoryStore_Status()[0] != '\0') return MemoryStore_Status();
    if (Scan_Active() != 0U) return "SCAN";
    return KeyLayout_Name(menu.last_key);
}

void StatusUI_Draw(void)
{
    char text[24] = "";
    char number[5];
    uint8_t text_x = 36U;
    uint8_t show_channel = (uint8_t)(MemoryControl_Mode() != 0U || MemoryControl_Selecting() != 0U);
    /* Выбранный канал виден на рабочем экране всегда, включая VFO и прямой ввод. */
    Text_U32ToDec(number, (uint32_t)MemoryControl_Channel() + 1U, 2U);
    LCD_DrawDigit8x9(UI_CHANNEL_X, UI_CHANNEL_Y, (uint8_t)number[0], CH_UNINVERT);
    LCD_DrawDigit8x9((uint8_t)(UI_CHANNEL_X + 9U), UI_CHANNEL_Y, (uint8_t)number[1], CH_UNINVERT);
    if (Entry_Active() != 0U)
    {
        /* FUNC уже виден сверху, здесь нужна только ошибка ввода. */
        if (Entry_Status()[0] == 'I')
            LCD_Print5x7(36U, 44U, Entry_Status(), 1U, CH_UNINVERT);
        return;
    }
    if (show_channel != 0U)
    {
        text_x = UI_LABEL_X;
        /* Шрифт пропорциональный: пробелы уже букв MEMO. Номер рисуем отдельно,
         * с постоянной координатой, чтобы мигание не сдвигало остаток строки. */
        if (MemoryControl_Selecting() == 0U || (App_GetTick500us() / 1000U) % 2U == 0U)
            LCD_Print5x7(text_x, 44U, "MEMO", 1U, CH_UNINVERT);
        /* WAIT/BLANK между MEMO и закреплённым справа номером. */
        text_x = UI_MEMORY_STATUS_X;
        if (MemoryStore_Ready() == 0U) LCD_Print5x7(text_x, 44U, "WAIT", 1U, CH_UNINVERT);
        else if (MemoryControl_Blank() != 0U)
        {
            /* Плоский инверсный фон с полем в один пиксель вокруг BLANK. */
            LCD_FillRectangle((uint8_t)(text_x - 1U), 42U,
                              (uint8_t)(LCD_GetText5x7Width("BLANK", 1U) + 1U), 8U, PX_ON);
            LCD_Print5x7(text_x, 43U, "BLANK", 1U, CH_INVERT);
        }
        return;
    }
    if (show_channel == 0U)
    {
        char filter[4] = "F";
        if (panel.nb != 0U) Text_Append(text, "N");
        if (panel.comp != 0U) Text_Append(text, "C");
        if (panel.vox != 0U && RadioMode_IsSSB(RadioVfo_Mode()) != 0U) Text_Append(text, "V");
        if (panel.tuner != 0U) Text_Append(text, (panel.tuner == 2U) ? "T*" : "T");
        Text_U32ToDec(number, (uint32_t)RadioVfo_Filter() + 1U, 1U);
        Text_Append(filter, number);
        LCD_Print5x7(UI_LABEL_X, 44U, filter, 1U, CH_UNINVERT);
        /* Прочие флаги идут после фильтра, не меняя его координату. */
        text_x = (uint8_t)(UI_LABEL_X + LCD_GetText5x7Width(filter, 1U) + 4U);
    }
    LCD_Print5x7(text_x, 44U, text, 1U, CH_UNINVERT);
}
