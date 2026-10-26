#include "ui_service_tools.h"
#include "service_factory.h"
#include "eeprom_factory.h"
#include "gmg12864.h"
#include "text_utils.h"

void ServiceTools_DrawFactory(void)
{
    uint8_t state = EEPROMFactory_State();
    char line[24];
    LCD_ClearBuffer();
    LCD_Print5x7(0U, 0U, "17.03 EEPROM INIT", 1U, CH_UNINVERT);
    if (state == EEPROM_FACTORY_IDLE)
    {
        LCD_Print5x7(0U, 12U, ServiceFactory_Status(), 1U, CH_UNINVERT);
        LCD_Print5x7(0U, 23U, "CHANNELS + UART LOST", 1U, CH_UNINVERT);
        LCD_Print5x7(0U, 35U, ServiceFactory_Confirmed() ? "YES - ERASE" : "NO - CANCEL", 1U, CH_UNINVERT);
        LCD_Print5x7(0U, 46U, "ENC:SELECT", 1U, CH_UNINVERT);
        LCD_Print5x7(0U, 56U, "DN:OK UP:BACK", 1U, CH_UNINVERT);
    }
    else
    {
        LCD_Print5x7(0U, 12U, EEPROMFactory_Status(), 1U, CH_UNINVERT);
        Text_U32ToDec(line, (uint32_t)EEPROMFactory_Offset() * 100U / 32768U, 1U);
        Text_Append(line, "%");
        LCD_Print5x7(0U, 24U, line, 1U, CH_UNINVERT);
        if (state == EEPROM_FACTORY_DONE || state == EEPROM_FACTORY_FAILED)
        {
            Text_U32ToDec(line, EEPROMFactory_Offset(), 1U);
            Text_Append(line, " BYTES VERIFIED");
            LCD_Print5x7(0U, 36U, line, 1U, CH_UNINVERT);
            LCD_Print5x7(0U, 56U, "F-INP:RESTART", 1U, CH_UNINVERT);
        }
        else
            LCD_Print5x7(0U, 46U, "KEEP POWER ON", 1U, CH_UNINVERT);
    }
    LCD_Update();
}
