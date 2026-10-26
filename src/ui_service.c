/* Прокручиваемые списки SERVICE, код пункта и честный статус реализации. */
#include "ui_service.h"
#include "service_control.h"
#include "service_values.h"
#include "app_state.h"
#include "gmg12864.h"
#include "text_utils.h"
#include "ui_service_tools.h"

static void Code(char *text, uint16_t id)
{
    char part[4];
    Text_U32ToDec(text, id / 100U, 2U);
    Text_Append(text, ".");
    Text_U32ToDec(part, id % 100U, 2U);
    Text_Append(text, part);
}


void ServiceUI_Draw(void)
{
    uint8_t stage = Service_Stage();
    uint8_t section = Service_Section();
    uint8_t selected = (stage == SERVICE_ROOT) ? section : ((stage == SERVICE_BAND) ? Service_Band() : Service_Item());
    uint8_t count = (stage == SERVICE_ROOT) ? SERVICE_SECTION_COUNT
                                            : ((stage == SERVICE_BAND) ? 9U : service_sections[section].count);
    uint8_t first = (uint8_t)((selected / 4U) * 4U);
    char text[24], part[8];
    if (stage == SERVICE_CONTRAST)
    {
        ServiceTools_DrawContrast();
        return;
    }
    if (stage == SERVICE_FACTORY)
    {
        ServiceTools_DrawFactory();
        return;
    }
    if (stage == SERVICE_UART)
    {
        ServiceTools_DrawUart();
        return;
    }
    if (stage == SERVICE_KEY_TEST)
    {
        ServiceTools_DrawKeyboard();
        return;
    }
    LCD_ClearBuffer();
    LCD_Print5x7(0U, 0U, "SERVICE", 1U, CH_UNINVERT);
    if (stage == SERVICE_VIEW)
    {
        char value[24], detail[24];
        const service_item_t *current = Service_Current();
        Code(text, current->id);
        LCD_Print5x7(92U, 0U, text, 1U, CH_UNINVERT);
        LCD_Print5x7(0U, 12U, current->label, 1U, CH_UNINVERT);
        ServiceValues_Read(current->id, value, detail);
        LCD_Print5x7(0U, 24U, value, 1U, CH_UNINVERT);
        LCD_Print5x7(0U, 35U, detail, 1U, CH_UNINVERT);
        LCD_Print5x7(0U, 46U, ServiceCatalog_TypeName(current->type), 1U, CH_UNINVERT);
        LCD_Print5x7(80U, 46U, (Service_BandScoped() != 0U) ? bands[Service_Band()].name : "VIEW ONLY", 1U,
                     CH_UNINVERT);
    }
    else
    {
        uint8_t row;
        Text_U32ToDec(text, (uint32_t)selected + 1U, 2U);
        Text_Append(text, " OF ");
        Text_U32ToDec(part, count, 2U);
        Text_Append(text, part);
        LCD_Print5x7((uint8_t)(LCD_WIDTH - LCD_GetText5x7Width(text, 1U)), 0U, text, 1U, CH_UNINVERT);
        for (row = 0U; row < 4U && (uint16_t)first + row < count; row++)
        {
            uint8_t index = (uint8_t)(first + row);
            uint8_t y = (uint8_t)(12U + row * 10U);
            const char *label;
            if (stage == SERVICE_ROOT)
            {
                Text_U32ToDec(text, (uint32_t)index + 1U, 2U);
                Text_Append(text, " ");
                label = service_sections[index].label;
            }
            else if (stage == SERVICE_BAND)
            {
                text[0] = '\0';
                label = bands[index].name;
            }
            else
            {
                const service_item_t *current = &service_items[service_sections[section].first + index];
                Code(text, current->id);
                Text_Append(text, " ");
                label = current->label;
            }
            Text_Append(text, label);
            if (index == selected)
                LCD_FillRectangle(0U, (uint8_t)(y - 1U), 127U, 8U, PX_ON);
            LCD_Print5x7(1U, y, text, 1U, (index == selected) ? CH_INVERT : CH_UNINVERT);
        }
    }
    LCD_Print5x7(0U, 56U, "DN:IN UP:BACK", 1U, CH_UNINVERT);
    LCD_Update();
}
