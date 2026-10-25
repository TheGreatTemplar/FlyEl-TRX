/* SET: быстрые установки; инженерное меню доступно отдельным пунктом.
 * Комбинация SET+PWR пока недоступна: кнопка PWR в схеме не назначена. */
#include "settings_control.h"
#include "app_state.h"
#include "radio_panel.h"
#include "radio_control.h"
#include "scan_control.h"
#include "radio_vfo.h"

static uint8_t page; /* 0 рабочий экран, 1 quick, 2 NB, 3 инженерный. */
static uint8_t item;

void Settings_Init(void) { page = 0U; item = 0U; }
uint8_t Settings_Page(void) { return page; }
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
    else if (page == 3U && (item == 0U || item == 1U))
    {
        int32_t count = (item == 0U) ? 6 : 3;
        int32_t current = (item == 0U) ? menu.step_index : menu.if_index;
        int32_t value = (current + steps % count) % count;
        if (value < 0) value += count;
        if (item == 0U) menu.step_index = (uint8_t)value;
        else { menu.if_index = (uint8_t)value; Radio_ApplySynth(); }
    }
    else if (page == 3U && item == 2U)
    {
        int32_t band = ((int32_t)menu.band_index + steps % Radio_BandsCount()) % Radio_BandsCount();
        if (band < 0) band += Radio_BandsCount();
        Radio_SetBand((uint8_t)band);
    }
    menu.need_redraw = 1U;
}

uint8_t Settings_HandleKey(input_key_t key, uint8_t long_press)
{
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
        if (page == 1U && item == 3U) Open(3U);
        else if (item == 4U || page == 2U) Open(0U);
        else if (page == 1U && item == 2U) RadioVfo_CycleFilter();
        else Settings_Encoder(1);
    }
    return 1U;
}
