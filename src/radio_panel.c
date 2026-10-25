#include "radio_panel.h"
#include "app_time.h"
#include "app_state.h"
#include <string.h>

radio_panel_t panel;

uint8_t RadioPanel_AttDb(uint8_t mask)
{
    /* bit0=6 дБ, bit1=12 дБ, bit2=24 дБ; 000..111 -> 0..42 дБ. */
    return (uint8_t)((mask & 7U) * 6U);
}

static void UpdateGain(void)
{
    /* Усиление предусилителя ещё не задано: рамка показывает P.AMP либо ослабление. */
    menu.gain_db = (int8_t)(-(int8_t)RadioPanel_AttDb(panel.att_mask));
    menu.need_redraw = 1U;
}

void RadioPanel_Init(void)
{
    memset(&panel, 0, sizeof(panel));
    panel.nb_level = 50U;
    UpdateGain();
}

void RadioPanel_CycleAtt(void)
{
    /* Маски реле: OFF, -6, -12, -18, -24, -30 дБ.
     * Последние две ступени временно исключены для эксперимента. */
    static const uint8_t masks[] = {
        0U, 1U, 2U, 3U, 4U, 5U,
        /* 6U, 7U, */ /* -36, -42 дБ: вернуть в цикл после проверки. */
    };
    uint8_t next = (uint8_t)(panel.att_mask + 1U);
    if (next >= sizeof(masks) / sizeof(masks[0])) next = 0U;
    panel.att_mask = masks[next];
    UpdateGain();
}

void RadioPanel_TogglePreamp(void)
{
    panel.preamp ^= 1U;
    UpdateGain();
}

void RadioPanel_Tuner(uint8_t long_press)
{
    panel.tuner = (long_press != 0U) ? 2U : (uint8_t)(panel.tuner == 0U);
    panel.tuner_tick = App_GetTick500us();
    menu.need_redraw = 1U;
}

void RadioPanel_Task(void)
{
    /* Пока нет сигнала результата от тюнера, запрос настройки истекает через 20 с. */
    if (panel.tuner == 2U && (uint32_t)(App_GetTick500us() - panel.tuner_tick) >= 40000U)
    { panel.tuner = 0U; menu.need_redraw = 1U; }
}
