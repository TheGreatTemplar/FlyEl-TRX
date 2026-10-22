/* Диапазоны, настройка RF/ПЧ и обработка кнопки TS. */
#include "radio_control.h"
#include "app_state.h"
#include "app_time.h"
#include "si5351.h"
#include "ts_button.h"

const band_t bands[] =
{
    { "160m",  1800000UL,  2000000UL,  1840000UL },
    { "80m",   3500000UL,  3800000UL,  3650000UL },
    { "40m",   7000000UL,  7200000UL,  7100000UL },
    { "30m",  10100000UL, 10150000UL, 10120000UL },
    { "20m",  14000000UL, 14350000UL, 14200000UL },
    { "17m",  18068000UL, 18168000UL, 18100000UL },
    { "15m",  21000000UL, 21450000UL, 21200000UL },
    { "12m",  24890000UL, 24990000UL, 24940000UL },
    { "10m",  28000000UL, 29700000UL, 28200000UL }
};

const uint32_t steps_hz[] =
{
    1UL,
    10UL,
	50UL,
    100UL,
    1000UL,
    10000UL
};

const uint32_t if_list_hz[] =
{
    4608000UL,
    8860000UL,
    11059200UL
};

static ts_button_t ts_button;

/* Возвращает количество диапазонов в таблице проекта. */
uint8_t Radio_BandsCount(void)
{
    return (uint8_t)(sizeof(bands) / sizeof(bands[0]));
}

/* Возвращает количество доступных шагов настройки. */
static uint8_t StepsCount(void)
{
    return (uint8_t)(sizeof(steps_hz) / sizeof(steps_hz[0]));
}

/* Возвращает количество вариантов промежуточной частоты. */
static uint8_t IfCount(void)
{
    return (uint8_t)(sizeof(if_list_hz) / sizeof(if_list_hz[0]));
}

/* Ограничивает частоту границами выбранного диапазона. */
void Radio_ClampRf(void)
{
    if (menu.rf_hz < bands[menu.band_index].min_hz)
    {
        menu.rf_hz = bands[menu.band_index].min_hz;
    }

    if (menu.rf_hz > bands[menu.band_index].max_hz)
    {
        menu.rf_hz = bands[menu.band_index].max_hz;
    }
}

/* Передаёт RF + ПЧ на CLK1 и сохраняет результат обмена с синтезатором. */
void Radio_ApplySynth(void)
{
    if (Si5351_SetVfoHighSide(VFO_CLK, menu.rf_hz, if_list_hz[menu.if_index]) == SI5351_OK)
    {
        menu.si5351_ok = 1U;
    }
    else
    {
        menu.si5351_ok = 0U;
    }
}

/* Выбирает диапазон, устанавливает стартовую частоту и обновляет синтезатор. */
void Radio_SetBand(uint8_t band)
{
    if (band >= Radio_BandsCount())
    {
        band = 0U;
    }

    menu.band_index = band;
    menu.rf_hz = bands[menu.band_index].start_hz;

    Radio_ClampRf();
    Radio_ApplySynth();

    menu.need_redraw = 1U;
}

/* Переключает полный список шагов сервисного меню; TS использует отдельный цикл. */
void Radio_CycleStep(void)
{
    menu.step_index++;

    if (menu.step_index >= StepsCount())
    {
        menu.step_index = 0U;
    }

    menu.need_redraw = 1U;
}

/* Обрабатывает Key_10_3 и применяет результат автомата TS к текущему шагу. */
void Radio_UpdateTS(input_key_t key)
{
    uint32_t old_step = steps_hz[menu.step_index];
    uint32_t new_step = TS_Button_Update(&ts_button, Input_KeyIsDown(Key_10_3),
                                         (uint8_t)(key == Key_10_3),
                                         App_GetTick500us(), old_step);
    uint8_t i;
    if (new_step != old_step)
    {
        for (i = 0U; i < StepsCount(); i++)
        {
            if (steps_hz[i] == new_step)
            {
                menu.step_index = i;
                menu.need_redraw = 1U;
                break;
            }
        }
    }
}

/* Выбирает следующую ПЧ и перестраивает гетеродин. */
void Radio_CycleIF(void)
{
    menu.if_index++;

    if (menu.if_index >= IfCount())
    {
        menu.if_index = 0U;
    }

    Radio_ApplySynth();
    menu.need_redraw = 1U;
}

/* Применяет частоту приёмника; вне любительских диапазонов экран покажет GEN. */
void Radio_SetFrequency(uint32_t hz)
{
    uint8_t i;
    if (hz < RADIO_MIN_HZ) hz = RADIO_MIN_HZ;
    if (hz > RADIO_MAX_HZ) hz = RADIO_MAX_HZ;
    menu.rf_hz = hz;
    for (i = 0U; i < Radio_BandsCount(); i++)
    {
        if (hz >= bands[i].min_hz && hz <= bands[i].max_hz)
        { menu.band_index = i; break; }
    }
    Radio_ApplySynth();
    menu.need_redraw = 1U;
}

/* 64-битный промежуточный результат исключает переполнение при накоплении энкодера. */
void Radio_TuneSteps(int32_t steps)
{
    int64_t hz;
    uint32_t min_hz = RADIO_MIN_HZ;
    uint32_t max_hz = RADIO_MAX_HZ;
    const band_t *band = &bands[menu.band_index];
    if (steps == 0) return;
    /* Общие пределы нужны только GEN, введённому явно. В выбранном КВ-диапазоне
     * энкодер упирается в края и не переключает диапазон через Radio_SetFrequency. */
    if (menu.rf_hz >= band->min_hz && menu.rf_hz <= band->max_hz)
    { min_hz = band->min_hz; max_hz = band->max_hz; }
    hz = (int64_t)menu.rf_hz + (int64_t)steps * steps_hz[menu.step_index];
    if (hz < (int64_t)min_hz) hz = min_hz;
    if (hz > (int64_t)max_hz) hz = max_hz;
    Radio_SetFrequency((uint32_t)hz);
}

/* Сбрасывает автомат TS при включении аппарата. */
void Radio_InitTS(void)
{
    TS_Button_Init(&ts_button, steps_hz[menu.step_index]);
}
