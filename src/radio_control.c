/* Диапазоны, настройка RF/ПЧ и обработка кнопки TS. */
#include "radio_control.h"
#include "app_state.h"
#include "app_time.h"
#include "si5351.h"
#include "ts_button.h"
#include "radio_vfo.h"

const band_t bands[] = {{"160m", 1800000UL, 2000000UL, 1840000UL},   {"80m", 3500000UL, 3800000UL, 3650000UL},
                        {"40m", 7000000UL, 7200000UL, 7100000UL},    {"30m", 10100000UL, 10150000UL, 10120000UL},
                        {"20m", 14000000UL, 14350000UL, 14200000UL}, {"17m", 18068000UL, 18168000UL, 18100000UL},
                        {"15m", 21000000UL, 21450000UL, 21200000UL}, {"12m", 24890000UL, 24990000UL, 24940000UL},
                        {"10m", 28000000UL, 29700000UL, 28200000UL}};

const uint32_t steps_hz[] = {1UL, 10UL, 50UL, 100UL, 1000UL, 10000UL};

const uint32_t if_list_hz[] = {4608000UL, 8860000UL, 11059200UL};

static ts_button_t ts_button;
static int16_t rit_hz;
static int16_t shift_hz;
static uint8_t rit_active;
static uint8_t tuning_locked;

/* Возвращает количество диапазонов в таблице проекта. */
uint8_t Radio_BandsCount(void)
{
    return (uint8_t)(sizeof(bands) / sizeof(bands[0]));
}


uint8_t Radio_FindBand(uint32_t hz)
{
    uint8_t i;
    for (i = 0U; i < Radio_BandsCount(); i++)
        if (hz >= bands[i].min_hz && hz <= bands[i].max_hz)
            return i;
    return Radio_BandsCount();
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
    Radio_ClampOffsets();
    if (Si5351_SetVfoHighSide(VFO_CLK, Radio_ReceiveHz(), if_list_hz[menu.if_index]) == SI5351_OK)
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
    uint32_t new_step = TS_Button_Update(&ts_button, Input_KeyIsDown(Key_10_3), (uint8_t)(key == Key_10_3),
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
    if (hz < RADIO_MIN_HZ)
        hz = RADIO_MIN_HZ;
    if (hz > RADIO_MAX_HZ)
        hz = RADIO_MAX_HZ;
    menu.rf_hz = hz;
    i = Radio_FindBand(hz);
    if (i < Radio_BandsCount())
        menu.band_index = i;
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
    if (steps == 0 || tuning_locked != 0U)
        return;
    /* Общие пределы оставлены для GEN из прежних настроек. В выбранном КВ-диапазоне
     * энкодер упирается в края и не переключает диапазон через Radio_SetFrequency. */
    if (menu.rf_hz >= band->min_hz && menu.rf_hz <= band->max_hz)
    {
        min_hz = band->min_hz;
        max_hz = band->max_hz;
    }
    hz = (int64_t)menu.rf_hz + (int64_t)steps * steps_hz[menu.step_index];
    if (hz < (int64_t)min_hz)
        hz = min_hz;
    if (hz > (int64_t)max_hz)
        hz = max_hz;
    Radio_SetFrequency((uint32_t)hz);
}


/* Сбрасывает автомат TS при включении аппарата. */
void Radio_InitTS(void)
{
    TS_Button_Init(&ts_button, steps_hz[menu.step_index]);
}


void Radio_InitOffsets(void)
{
    rit_hz = 0;
    shift_hz = 0;
    rit_active = 0U;
    tuning_locked = 0U;
}


uint8_t Radio_RitActive(void)
{
    return rit_active;
}


uint8_t Radio_Locked(void)
{
    return tuning_locked;
}


int16_t Radio_ShiftLimitHz(void)
{
    uint8_t mode = RadioVfo_Mode();
    if (mode == RADIO_AM)
        return 0;
    if (RadioVfo_Filter() == 2U &&
        (mode == RADIO_CW || mode == RADIO_CW_R || mode == RADIO_RTTY || mode == RADIO_RTTY_R))
        return 250;
    return 1200;
}


void Radio_ClampOffsets(void)
{
    int16_t limit = Radio_ShiftLimitHz();
    if (shift_hz > limit)
        shift_hz = limit;
    if (shift_hz < -limit)
        shift_hz = (int16_t)-limit;
}


uint8_t Radio_ShiftActive(void)
{
    return (uint8_t)(shift_hz != 0 && Radio_ShiftLimitHz() != 0);
}


int16_t Radio_OffsetHz(input_aux_encoder_t encoder)
{
    if (encoder == INPUT_AUX_RIT)
        return rit_hz;
    if (encoder == INPUT_AUX_SHIFT)
        return shift_hz;
    return 0;
}


uint32_t Radio_ReceiveHz(void)
{
    int64_t hz = (int64_t)menu.rf_hz;
    if (rit_active != 0U && menu.tx_active == 0U)
        hz += rit_hz;
    /* RIT может выйти за край любительского диапазона, но не за пределы приёмника. */
    if (hz < (int64_t)RADIO_MIN_HZ)
        hz = RADIO_MIN_HZ;
    if (hz > (int64_t)RADIO_MAX_HZ)
        hz = RADIO_MAX_HZ;
    return (uint32_t)hz;
}


void Radio_ToggleRit(void)
{
    rit_active ^= 1U;
    /* Выключение убирает расстройку из синтезатора, сохраняя её значение. */
    Radio_ApplySynth();
    menu.need_redraw = 1U;
}


void Radio_ToggleLock(void)
{
    tuning_locked ^= 1U;
    menu.need_redraw = 1U;
}


uint8_t Radio_AdjustAuxEncoder(input_aux_encoder_t encoder, int32_t steps)
{
    int16_t limit;
    int16_t old;
    int64_t value;
    if ((uint8_t)encoder >= INPUT_AUX_COUNT || steps == 0 || (encoder == INPUT_AUX_RIT && rit_active == 0U))
        return 0U;
    limit = encoder == INPUT_AUX_RIT ? 1200 : Radio_ShiftLimitHz();
    if (limit == 0)
        return 0U;
    old = Radio_OffsetHz(encoder);
    value = (int64_t)old + (int64_t)steps * 10;
    if (value > limit)
        value = limit;
    if (value < -limit)
        value = -limit;
    if (old == value)
        return 0U;
    if (encoder == INPUT_AUX_RIT)
    {
        rit_hz = (int16_t)value;
        Radio_ApplySynth();
    }
    else
    {
        /* До реализации BFO/коммутации полосы SHIFT — программное состояние.
         * Односторонний сдвиг VFO здесь ошибочно превратил бы SHIFT в ещё один RIT. */
        shift_hz = (int16_t)value;
    }
    menu.need_redraw = 1U;
    return 1U;
}
