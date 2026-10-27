#include "ui_service_tools.h"
#include "fw_version.h"
#include "gmg12864.h"
#include "text_utils.h"

void ServiceTools_DrawVersion(void)
{
    uint8_t row;
    char text[24];
    char count[8];
    LCD_ClearBuffer();
    LCD_Print5x7(0U, 0U, FWInfo_Title(), 1U, CH_UNINVERT);
    Text_U32ToDec(text, (uint32_t)FWInfo_Page() + 1U, 1U);
    Text_Append(text, "/");
    Text_U32ToDec(count, FWInfo_Pages(), 1U);
    Text_Append(text, count);
    LCD_Print5x7((uint8_t)(128U - LCD_GetText5x7Width(text, 1U)), 0U, text, 1U, CH_UNINVERT);
    for (row = 0U; row < 5U; row++)
    {
        FWInfo_Line(row, text);
        LCD_Print5x7(0U, (uint8_t)(11U + row * 9U), text, 1U, CH_UNINVERT);
    }
    LCD_Print5x7(0U, 56U, "ENC:PAGE UP:BACK", 1U, CH_UNINVERT);
    LCD_Update();
}
