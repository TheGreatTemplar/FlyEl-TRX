/* SET: быстрые установки. SERVICE открывается отдельным сочетанием SET + FILTER. */
#include "settings_control.h"
#include "app_state.h"
#include "radio_panel.h"
#include "radio_control.h"
#include "scan_control.h"
#include "radio_vfo.h"
#include "service_control.h"

static uint8_t page; /* 0 рабочий экран, 1 quick, 2 NB. SERVICE возвращается как страница 3. */
static uint8_t item;

void Settings_Init(void) { page = 0U; item = 0U; Service_Init(); }
uint8_t Settings_Page(void) { return (Service_Active() != 0U) ? 3U : page; }
uint8_t Settings_Item(void) { return item; }

static void Open(uint8_t next)
{
    page = next; item = 0U;
    menu.menu_active = (uint8_t)(page != 0U);
    menu.need_redraw = 1U;
}

static void Move(int32_t steps)
{
    int32_t count = (page == 2U) ? 1 : 5;
    int32_t value = ((int32_t)item + steps % count) % count;
    if (value < 0) value += count;
    item = (uint8_t)value;
    menu.need_redraw = 1U;
}

void Settings_Encoder(int32_t steps)
{
    if (Service_Active() != 0U) { Service_Encoder(steps); return; }
    if (steps == 0) return;
    if (page == 2U || (page == 1U && item == 0U))
    {
        int64_t value = (int64_t)panel.nb_level + steps;
        if (value < 0) value = 0;
        if (value > 100) value = 100;
        panel.nb_level = (uint8_t)value;
    }
    else if (page == 1U && item == 1U) Scan_AdjustInterval(steps);
    else if (page == 1U && item == 2U) RadioVfo_AdjustFilter(steps);
    menu.need_redraw = 1U;
}

uint8_t Settings_OpenService(void)
{
    if (Service_Open() == 0U) return 0U;
    page = 0U; item = 0U;
    return 1U;
}

uint8_t Settings_HandleKey(input_key_t key, uint8_t long_press)
{
    if (Service_Active() != 0U) { Service_HandleKey(key); return 1U; }
    if (key == Key_6_3)
    {
        if (long_press != 0U) Open((page == 0U) ? 1U : 0U);
        else if (page != 0U) Open(0U);
        else { panel.meter = (uint8_t)((panel.meter + 1U) % 3U); menu.need_redraw = 1U; }
        return 1U;
    }
    if (key == Key_6_1 && long_press != 0U) { Open(2U); return 1U; }
    if (page == 0U) return 0U;
    /* Пункты идут сверху вниз: DN — следующий, UP — предыдущий. */
    if (key == Key_9_2) Move(1);
    else if (key == Key_9_3) Move(-1);
    else if (key == Key_5_3)
    {
        if (page == 1U && item == 3U) return 1U; /* Подсказка входа только двумя кнопками. */
        else if (item == 4U || page == 2U) Open(0U);
        else if (page == 1U && item == 2U) RadioVfo_CycleFilter();
        else Settings_Encoder(1);
    }
    return 1U;
}
