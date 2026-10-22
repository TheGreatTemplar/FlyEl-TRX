/* Состояния VFO принадлежат основному циклу; ISR к ним не обращаются. */
#include "radio_vfo.h"
#include "app_state.h"
#include "radio_control.h"

static radio_preset_t vfo[2];
static uint8_t active_vfo;
static uint8_t split_active;
static uint8_t filters[RADIO_MODE_COUNT];

radio_preset_t RadioVfo_Capture(void)
{
    radio_preset_t result = vfo[active_vfo];
    result.hz = menu.rf_hz;
    result.band = menu.band_index;
    return result;
}

/* Сохраняет рабочую настройку перед переключением; старая частота не теряется. */
static void SaveActive(void)
{
    vfo[active_vfo] = RadioVfo_Capture();
}

void RadioVfo_Apply(const radio_preset_t *preset)
{
    vfo[active_vfo] = *preset;
    filters[preset->mode] = preset->filter;
    menu.rf_hz = preset->hz;
    menu.band_index = preset->band;
    menu.usb_active = (uint8_t)(preset->mode == RADIO_USB);
    Radio_ApplySynth();
    menu.need_redraw = 1U;
}

void RadioVfo_Init(void)
{
    uint8_t i;
    radio_preset_t initial = { menu.rf_hz, RADIO_LSB, 0U, 0U, menu.band_index };
    active_vfo = 0U;
    split_active = 0U;
    for (i = 0U; i < RADIO_MODE_COUNT; i++) filters[i] = 0U;
    vfo[0] = initial;
    vfo[1] = initial;
}

/* A/B меняет активную настройку. Физическое переключение RX/TX ещё не подключено. */
void RadioVfo_Switch(void)
{
    SaveActive();
    active_vfo ^= 1U;
    RadioVfo_Apply(&vfo[active_vfo]);
}

/* По раскладке A=B всегда копирует A в B, независимо от выбранного VFO. */
void RadioVfo_CopyAToB(void)
{
    SaveActive();
    vfo[1] = vfo[0];
    if (active_vfo == 1U) RadioVfo_Apply(&vfo[1]);
    menu.need_redraw = 1U;
}

void RadioVfo_ChangeMode(uint8_t long_press)
{
    SaveActive();
    RadioMode_Change(&vfo[active_vfo], long_press);
    vfo[active_vfo].filter = filters[vfo[active_vfo].mode];
    RadioVfo_Apply(&vfo[active_vfo]);
}

void RadioVfo_CycleFilter(void)
{
    RadioVfo_AdjustFilter(1);
}

void RadioVfo_AdjustFilter(int32_t steps)
{
    uint8_t mode = vfo[active_vfo].mode;
    int32_t value = ((int32_t)filters[mode] + steps % (int32_t)RADIO_FILTER_COUNT) % (int32_t)RADIO_FILTER_COUNT;
    if (value < 0) value += (int32_t)RADIO_FILTER_COUNT;
    filters[mode] = (uint8_t)value;
    vfo[active_vfo].filter = filters[mode];
    menu.need_redraw = 1U;
}

uint8_t RadioVfo_Filter(void) { return vfo[active_vfo].filter; }

void RadioVfo_ToggleSplit(void)
{
    split_active ^= 1U;
    menu.need_redraw = 1U;
}

uint8_t RadioVfo_Split(void) { return split_active; }
uint8_t RadioVfo_Active(void) { return active_vfo; }
uint8_t RadioVfo_Mode(void) { return vfo[active_vfo].mode; }
uint32_t RadioVfo_OtherHz(void) { return vfo[active_vfo ^ 1U].hz; }
