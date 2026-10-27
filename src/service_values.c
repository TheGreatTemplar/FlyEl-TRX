/* Первые доступные страницы мониторинга. Остальные пункты явно не реализованы. */
#include "service_values.h"
#include "app_state.h"
#include "radio_panel.h"
#include "text_utils.h"
#include "gmg12864.h"
#include "build_info.h"

void ServiceValues_Read(uint16_t id, char *value, char *detail)
{
    value[0] = '\0';
    detail[0] = '\0';
    switch (id)
    {
        case 101U:
            Text_Append(value, "NOT SET");
            Text_Append(detail, "BOARD REVISION");
            break;
        case 102U:
            Text_Append(value, FW_GIT_SHORT);
            Text_Append(detail, FW_BUILD_DATE);
            break;
        case 104U:
        case 105U:
            Text_Append(value, "RX PROTOTYPE");
            Text_Append(detail, "TX NOT CONNECTED");
            break;
        case 109U:
        case 211U:
            Text_Append(value, (menu.si5351_ok != 0U) ? "SI5351 WRITE OK" : "SI5351 WRITE ERR");
            Text_Append(detail, "NO LOCK READBACK");
            break;
        case 501U:
        case 1607U:
            Text_U32ToDec(value, if_list_hz[menu.if_index], 1U);
            Text_Append(value, " HZ");
            Text_Append(detail, "CURRENT IF");
            break;
        case 1402U:
            Text_U32ToDec(value, LCD_GetContrast(), 1U);
            Text_Append(detail, "0..63 ENCODER");
            break;
        case 1501U:
            Text_Append(value, "PB6 PB7   PB8 PB9");
            Text_Append(detail, "SOFTWARE I2C");
            break;
        case 1603U:
            Text_U32ToDec(value, menu.adc0_mv, 1U);
            Text_Append(value, " MV ADC0");
            Text_U32ToDec(detail, menu.adc1_mv, 1U);
            Text_Append(detail, " MV ADC1");
            break;
        case 1608U:
            Text_U32ToDec(value, RadioPanel_AttDb(panel.att_mask), 1U);
            Text_Append(value, " DB ATT");
            Text_Append(detail, (panel.preamp != 0U) ? "P.AMP ON" : "P.AMP OFF");
            break;
        default:
            Text_Append(value, "NOT IMPLEMENTED");
            Text_Append(detail, "NEEDS SPEC OR HW");
            break;
    }
}
