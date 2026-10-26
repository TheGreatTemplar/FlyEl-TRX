#include "ui_service_tools.h"
#include "service_contrast.h"
#include "gmg12864.h"
#include "text_utils.h"

void ServiceTools_DrawContrast(void)
{
    char value[16];
    LCD_ClearBuffer();
    LCD_Print5x7(0U, 0U, "14.02 LCD CONTRAST", 1U, CH_UNINVERT);
    Text_U32ToDec(value, LCD_GetContrast(), 2U);
    Text_Append(value, " / 63");
    LCD_Print5x7(0U, 12U, value, 1U, CH_UNINVERT);
    LCD_DrawRectangle(0U, 23U, 127U, 8U, PX_ON);
    if (LCD_GetContrast() != 0U)
        LCD_FillRectangle(2U, 25U, (uint8_t)((uint32_t)LCD_GetContrast() * 123U / LCD_CONTRAST_MAX), 4U, PX_ON);
    LCD_Print5x7(0U, 35U, "0123456789 ABCDE", 1U, CH_UNINVERT);
    LCD_Print5x7(0U, 46U, ServiceContrast_Status(), 1U, CH_UNINVERT);
    LCD_Print5x7(0U, 56U, "DN:SAVE UP:BACK", 1U, CH_UNINVERT);
    LCD_Update();
}
