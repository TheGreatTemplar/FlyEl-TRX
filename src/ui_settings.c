/* Три компактных меню: UP/DOWN выбирают строку, энкодер меняет значение. */
#include "ui_settings.h"
#include "settings_control.h"
#include "radio_panel.h"
#include "radio_vfo.h"
#include "scan_control.h"
#include "app_state.h"
#include "text_utils.h"
#include "ui_widgets.h"
#include "gmg12864.h"

void SettingsUI_Draw(void)
{
    char text[20];
    uint8_t page = Settings_Page();
    uint8_t item = Settings_Item();
    LCD_ClearBuffer();
    LCD_Print5x7(0U, 0U, (page == 3U) ? "ENGINEER" : ((page == 2U) ? "NB LEVEL" : "QUICK SET"), 1U, CH_UNINVERT);
    LCD_Print5x7(83U, 0U, "SET:OUT", 1U, CH_UNINVERT);
    if (page == 2U)
    {
        Text_U32ToDec(text, panel.nb_level, 1U);
        Widgets_MenuLine(12U, "LEVEL", text, 1U);
        LCD_Print5x7(0U, 40U, "ENC:0..100", 1U, CH_UNINVERT);
        LCD_Print5x7(0U, 52U, "SOFTWARE ONLY", 1U, CH_UNINVERT);
    }
    else if (page == 1U)
    {
        Text_U32ToDec(text, panel.nb_level, 1U);
        Widgets_MenuLine(11U, "NB", text, (uint8_t)(item == 0U));
        Text_U32ToDec(text, Scan_Interval() / 2U, 1U); Text_Append(text, "ms");
        Widgets_MenuLine(21U, "SCAN", text, (uint8_t)(item == 1U));
        Text_U32ToDec(text, (uint32_t)RadioVfo_Filter() + 1U, 1U);
        Widgets_MenuLine(31U, "FIL", text, (uint8_t)(item == 2U));
        Widgets_MenuLine(41U, "ENG", "ENT", (uint8_t)(item == 3U));
        Widgets_MenuLine(51U, "EXIT", "ENT", (uint8_t)(item == 4U));
    }
    else
    {
        Text_U32ToDec(text, steps_hz[menu.step_index], 1U); Text_Append(text, "Hz");
        Widgets_MenuLine(11U, "STEP", text, (uint8_t)(item == 0U));
        Text_U32ToDec(text, if_list_hz[menu.if_index] / 1000U, 1U); Text_Append(text, "k");
        Widgets_MenuLine(21U, "IF", text, (uint8_t)(item == 1U));
        Widgets_MenuLine(31U, "BAND", bands[menu.band_index].name, (uint8_t)(item == 2U));
        Widgets_MenuLine(41U, "OUT", "CLK1", (uint8_t)(item == 3U));
        Widgets_MenuLine(51U, "EXIT", "ENT", (uint8_t)(item == 4U));
    }
    LCD_Update();
}
