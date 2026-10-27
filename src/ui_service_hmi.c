#include "ui_service_tools.h"
#include "service_hmi.h"
#include "service_control.h"
#include "gmg12864.h"

void ServiceTools_DrawHMI(void)
{
    char value[24];
    LCD_ClearBuffer();
    LCD_Print5x7(0U, 0U, "HMI SETTINGS", 1U, CH_UNINVERT);
    LCD_Print5x7(0U, 12U, Service_Current()->label, 1U, CH_UNINVERT);
    ServiceHMI_Value(value);
    LCD_Print5x7(0U, 24U, value, 1U, CH_UNINVERT);
    LCD_Print5x7(0U, 35U, ServiceHMI_Range(), 1U, CH_UNINVERT);
    LCD_Print5x7(0U, 46U, ServiceHMI_Status(), 1U, CH_UNINVERT);
    LCD_Print5x7(0U, 56U, "DN:SAVE UP:BACK", 1U, CH_UNINVERT);
    LCD_Update();
}
