/* Полные жесты физических кнопок исполняются на настоящем ARM-коде контроллеров.
 * GPIO и EEPROM подменены; протокол EEPROM проверяется отдельно в memory_arm.c. */
#include <string.h>
#include "ts_button.c"
#include "key_gesture.c"
#include "key_layout.c"
#include "radio_mode.c"
#include "radio_vfo.c"
#include "radio_control.c"
#include "radio_panel.c"
#include "frequency_parse.c"
#include "freq_entry.c"
#include "memory_control.c"
#include "entry_controller.c"
#include "scan_control.c"
#include "settings_control.c"
#include "keyboard.c"
#define CHECK(expr) do { if (!(expr)) return __LINE__; } while (0)

generator_menu_t menu;
static uint8_t key_down[INPUT_KEY_COUNT];
static uint32_t now_tick;
static uint32_t synth_frequency;
static uint32_t store_calls;
static uint8_t cache_used[MEMORY_CHANNEL_COUNT];
static radio_preset_t cache[MEMORY_CHANNEL_COUNT];

uint32_t App_GetTick500us(void) { return now_tick; }
uint8_t Input_KeyIsDown(input_key_t key) { return key_down[key]; }
char Input_KeyToChar(input_key_t key) { return KeyLayout_Character(key); }
const char *Input_KeyName(input_key_t key) { (void)key; return ""; }
si5351_status_t Si5351_SetVfoHighSide(uint8_t clk, uint32_t hz, uint32_t if_hz)
{ (void)clk; (void)if_hz; synth_frequency = hz; return SI5351_OK; }

void MemoryStore_Init(void) { memset(cache_used, 0, sizeof(cache_used)); store_calls = 0U; }
uint8_t MemoryStore_Task(void) { return 0U; }
uint8_t MemoryStore_Ready(void) { return 1U; }
uint8_t MemoryStore_Busy(void) { return 0U; }
uint8_t MemoryStore_Get(uint8_t index, radio_preset_t *preset)
{
    if (cache_used[index] == 0U) return 0U;
    if (preset != 0) *preset = cache[index];
    return 1U;
}
uint8_t MemoryStore_Request(uint8_t index, const radio_preset_t *preset)
{
    store_calls++; cache_used[index] = (uint8_t)(preset != 0);
    if (preset != 0) cache[index] = *preset;
    return 1U;
}

static void InitControls(void)
{
    memset(&menu, 0, sizeof(menu));
    memset(key_down, 0, sizeof(key_down));
    now_tick = 0U; menu.rf_hz = 28200000U; menu.band_index = 8U; menu.step_index = 3U;
    RadioVfo_Init(); RadioPanel_Init(); MemoryControl_Init();
    Entry_Init(); Settings_Init(); Scan_Init(); Keyboard_Init(); Radio_InitTS();
}

static void Tap(input_key_t key, uint32_t duration)
{
    key_down[key] = 1U; Keyboard_Task(key);
    now_tick += duration;
    Keyboard_Task(INPUT_KEY_COUNT);
    key_down[key] = 0U; Keyboard_Task(INPUT_KEY_COUNT);
}

int test_controls_att_six_enabled_levels_and_all_relay_masks(void)
{
    uint8_t i;
    InitControls();
    for (i = 0U; i < 8U; i++)
        CHECK(RadioPanel_AttDb(i) == (uint8_t)(i * 6U));
    for (i = 1U; i <= 12U; i++)
    {
        Tap(Key_7_2, 100U);
        CHECK(panel.att_mask == (i % 6U));
        CHECK(RadioPanel_AttDb(panel.att_mask) == (uint8_t)((i % 6U) * 6U));
        CHECK(menu.gain_db == (int8_t)(-(int8_t)((i % 6U) * 6U)));
    }
    return 0;
}

int test_controls_vfo_and_mode_memory(void)
{
    InitControls();
    Radio_SetFrequency(7100000U);
    Tap(Key_10_1, 100U); CHECK(RadioVfo_Mode() == RADIO_USB);
    Tap(Key_1_3, 100U); CHECK(menu.rf_hz == 28200000U && RadioVfo_Active() == 1U);
    Tap(Key_1_3, 100U); CHECK(menu.rf_hz == 7100000U && RadioVfo_Mode() == RADIO_USB);
    Tap(Key_1_2, 100U); CHECK(RadioVfo_OtherHz() == 7100000U);
    Tap(Key_10_1, 2001U); CHECK(RadioVfo_Mode() == RADIO_CW);
    Tap(Key_10_1, 100U); CHECK(RadioVfo_Mode() == RADIO_CW_R);
    Tap(Key_10_1, 2001U); CHECK(RadioVfo_Mode() == RADIO_RTTY);
    Tap(Key_10_1, 2001U); CHECK(RadioVfo_Mode() == RADIO_AM);
    Tap(Key_10_1, 2001U); CHECK(RadioVfo_Mode() == RADIO_USB);
    return 0;
}

int test_controls_func_frequency_commit_and_cancel(void)
{
    InitControls();
    Tap(Key_5_3, 100U); CHECK(Entry_Active() != 0U);
    Tap(Key_1_2, 100U); /* 2 */
    Tap(Key_1_1, 100U); /* 1 */
    Tap(Key_5_1, 100U); /* . */
    Tap(Key_2_1, 100U); /* 4 */
    Tap(Key_2_1, 100U); /* 4 */
    Tap(Key_4_3, 100U); /* 9 */
    Tap(Key_4_1, 100U); /* 7 */
    Tap(Key_4_2, 100U); /* 8 */
    CHECK(store_calls == 0U && Scan_Active() == 0U && RadioVfo_Split() == 0U);
    Tap(Key_5_3, 100U);
    CHECK(Entry_Active() == 0U && menu.rf_hz == 21449780U && synth_frequency == 21449780U);
    Tap(Key_5_3, 100U); Tap(Key_5_3, 100U); CHECK(Entry_Active() == 0U);
    Tap(Key_5_3, 100U); now_tick += FUNC_TIMEOUT_TICKS; Entry_Task();
    CHECK(Entry_Active() == 0U && menu.rf_hz == 21449780U);
    return 0;
}

int test_controls_func_hold_cannot_write_memory(void)
{
    InitControls(); Tap(Key_5_3, 100U);
    Tap(Key_2_1, 25000U); Entry_Task();
    CHECK(store_calls == 0U);
    CHECK(RadioVfo_Mode() == RADIO_LSB);
    return 0;
}

int test_controls_memory_keys_require_strict_long_press(void)
{
    InitControls(); Tap(Key_2_1, 2000U); CHECK(store_calls == 0U);
    Tap(Key_2_1, 2001U); CHECK(store_calls == 1U && cache_used[0] != 0U);
    Tap(Key_2_2, 2001U); CHECK(cache_used[0] != 0U); /* CLEAR только при CH. */
    Tap(Key_9_1, 100U); Tap(Key_2_2, 2001U);
    CHECK(cache_used[0] == 0U && store_calls == 2U);
    return 0;
}

int test_controls_channel_entry_and_vfo_restore(void)
{
    InitControls(); Tap(Key_9_1, 100U); Tap(Key_5_3, 100U);
    Tap(Key_4_3, 100U); Tap(Key_4_3, 100U); Tap(Key_5_3, 100U);
    CHECK(MemoryControl_Channel() == 98U && MemoryControl_Selecting() != 0U);
    Radio_SetFrequency(7100000U); Tap(Key_2_1, 2001U);
    Radio_SetFrequency(14200000U); Tap(Key_1_1, 100U);
    CHECK(MemoryControl_Mode() != 0U && menu.rf_hz == 7100000U);
    Tap(Key_1_1, 100U); CHECK(menu.rf_hz == 14200000U && MemoryControl_Mode() == 0U);
    Tap(Key_2_3, 2001U); CHECK(menu.rf_hz == 7100000U);
    return 0;
}

int test_controls_up_down_and_scan_stop(void)
{
    InitControls(); Tap(Key_9_3, 100U); CHECK(menu.band_index == 0U);
    Tap(Key_9_2, 100U); CHECK(menu.band_index == 8U);
    Tap(Key_4_2, 100U); CHECK(Scan_Active() != 0U);
    now_tick += Scan_Interval(); Scan_Task(); CHECK(menu.rf_hz == 28200100U);
    Tap(Key_9_2, 100U); CHECK(Scan_Active() == 0U && menu.band_index == 8U);
    Tap(Key_9_1, 100U); Tap(Key_9_2, 100U); CHECK(MemoryControl_Channel() == 98U);
    return 0;
}

/* Длительное удержание обеих стрелок переключает диапазон только один раз. */
int test_band_arrows_do_not_repeat_while_held(void)
{
    uint8_t direction;
    uint32_t elapsed;

    for (direction = 0U; direction < 2U; direction++)
    {
        input_key_t key = (direction == 0U) ? Key_9_3 : Key_9_2;
        uint8_t expected_band = (direction == 0U) ? 0U : 7U;

        InitControls();
        key_down[key] = 1U;
        Keyboard_Task(key);
        CHECK(menu.band_index == expected_band);
        for (elapsed = 0U; elapsed < 6000U; elapsed++)
        {
            now_tick++;
            Keyboard_Task(INPUT_KEY_COUNT);
            CHECK(menu.band_index == expected_band);
        }
        key_down[key] = 0U;
        Keyboard_Task(INPUT_KEY_COUNT);
        CHECK(menu.band_index == expected_band);
        Tap(key, 100U);
        CHECK(menu.band_index == ((direction == 0U) ? 1U : 6U));
    }
    return 0;
}

/* В CH обе стрелки срабатывают сразу, через 600 мс и далее через 200 мс. */
int test_channel_arrows_repeat_at_600_and_200_ms(void)
{
    uint8_t direction;

    for (direction = 0U; direction < 2U; direction++)
    {
        input_key_t key = (direction == 0U) ? Key_9_3 : Key_9_2;
        uint32_t press_tick;

        InitControls();
        Tap(Key_9_1, 100U);
        MemoryControl_Select(50U);
        press_tick = now_tick;
        key_down[key] = 1U;
        Keyboard_Task(key);
        CHECK(MemoryControl_Channel() == ((direction == 0U) ? 51U : 49U));
        now_tick = press_tick + 1199U;
        Keyboard_Task(INPUT_KEY_COUNT);
        CHECK(MemoryControl_Channel() == ((direction == 0U) ? 51U : 49U));
        now_tick++;
        Keyboard_Task(INPUT_KEY_COUNT);
        CHECK(MemoryControl_Channel() == ((direction == 0U) ? 52U : 48U));
        now_tick = press_tick + 1599U;
        Keyboard_Task(INPUT_KEY_COUNT);
        CHECK(MemoryControl_Channel() == ((direction == 0U) ? 52U : 48U));
        now_tick++;
        Keyboard_Task(INPUT_KEY_COUNT);
        CHECK(MemoryControl_Channel() == ((direction == 0U) ? 53U : 47U));
        now_tick = press_tick + 2000U;
        key_down[key] = 0U;
        Keyboard_Task(INPUT_KEY_COUNT);
        CHECK(MemoryControl_Channel() == ((direction == 0U) ? 53U : 47U));
        CHECK(menu.band_index == 8U);
    }
    return 0;
}

int test_controls_memory_scan_skips_blank(void)
{
    InitControls();
    cache_used[1] = 1U; cache[1] = (radio_preset_t){7100000U, RADIO_LSB, 0U, 0U, 2U};
    cache_used[7] = 1U; cache[7] = (radio_preset_t){14200000U, RADIO_USB, 1U, 0U, 4U};
    cache_used[98] = 1U; cache[98] = (radio_preset_t){28200000U, RADIO_LSB, 0U, 0U, 8U};
    Tap(Key_1_1, 100U); Tap(Key_4_2, 100U);
    now_tick += Scan_Interval(); Scan_Task(); CHECK(MemoryControl_Channel() == 1U && menu.rf_hz == 7100000U);
    now_tick += Scan_Interval(); Scan_Task(); CHECK(MemoryControl_Channel() == 7U && menu.rf_hz == 14200000U);
    now_tick += Scan_Interval(); Scan_Task(); CHECK(MemoryControl_Channel() == 98U && menu.rf_hz == 28200000U);
    now_tick += Scan_Interval(); Scan_Task(); CHECK(MemoryControl_Channel() == 1U && menu.rf_hz == 7100000U);
    return 0;
}

int test_controls_tuner_timeout_vox_and_nr_exclusions(void)
{
    InitControls(); Tap(Key_4_3, 100U); CHECK(panel.vox != 0U);
    Tap(Key_10_1, 2001U); Tap(Key_4_3, 100U); CHECK(panel.vox == 1U);
    Tap(Key_7_3, 2001U); CHECK(panel.tuner == 2U);
    now_tick += 40000U; RadioPanel_Task(); CHECK(panel.tuner == 0U);
    Tap(Key_5_1, 2001U); Tap(Key_5_2, 100U); CHECK(Entry_Active() == 0U && Settings_Page() == 0U);
    return 0;
}

int test_controls_set_nb_menu_and_ts_unchanged(void)
{
    InitControls(); Tap(Key_10_3, 100U); CHECK(steps_hz[menu.step_index] == 1000U);
    Tap(Key_10_3, 2001U); CHECK(steps_hz[menu.step_index] == 10000U);
    Tap(Key_10_3, 2001U); CHECK(steps_hz[menu.step_index] == 1000U);
    Tap(Key_6_3, 100U); CHECK(panel.meter == 1U);
    Tap(Key_6_3, 2001U); CHECK(Settings_Page() == 1U);
    Tap(Key_10_3, 100U); CHECK(steps_hz[menu.step_index] == 1000U);
    Tap(Key_6_3, 100U); CHECK(Settings_Page() == 0U);
    Tap(Key_6_1, 2001U); CHECK(Settings_Page() == 2U && panel.nb == 0U);
    Settings_Encoder(100); CHECK(panel.nb_level == 100U);
    return 0;
}

int test_controls_filter_memory_func_cancel_and_engineering_direction(void)
{
    InitControls(); Tap(Key_10_2, 100U); CHECK(RadioVfo_Filter() == 1U);
    Tap(Key_10_1, 2001U); CHECK(RadioVfo_Filter() == 0U);
    Tap(Key_10_2, 100U); Tap(Key_10_2, 100U); CHECK(RadioVfo_Filter() == 2U);
    Tap(Key_10_1, 2001U); Tap(Key_10_1, 2001U); Tap(Key_10_1, 2001U);
    CHECK(RadioVfo_Mode() == RADIO_LSB && RadioVfo_Filter() == 1U);
    Tap(Key_5_3, 100U); Tap(Key_1_1, 100U); Tap(Key_5_3, 2001U);
    CHECK(Entry_Active() == 0U && menu.rf_hz == 28200000U);
    Tap(Key_6_3, 2001U);
    Tap(Key_9_3, 100U); Tap(Key_9_3, 100U); Tap(Key_9_3, 100U);
    Tap(Key_5_3, 100U); CHECK(Settings_Page() == 3U);
    Settings_Encoder(-1); CHECK(menu.step_index == 2U);
    Tap(Key_9_3, 100U); Settings_Encoder(-1); CHECK(menu.if_index == 2U);
    return 0;
}

int test_encoder_stops_at_each_band_edge_for_every_step(void)
{
    uint8_t band, step;
    InitControls();
    for (band = 0U; band < Radio_BandsCount(); band++)
    {
        for (step = 0U; step < 6U; step++)
        {
            Radio_SetBand(band); menu.step_index = step;
            menu.rf_hz = bands[band].min_hz;
            Radio_TuneSteps(-1); CHECK(menu.rf_hz == bands[band].min_hz && menu.band_index == band);
            Radio_TuneSteps(-2147483647 - 1); CHECK(menu.rf_hz == bands[band].min_hz);
            menu.rf_hz = bands[band].max_hz;
            Radio_TuneSteps(1); CHECK(menu.rf_hz == bands[band].max_hz && menu.band_index == band);
            Radio_TuneSteps(2147483647); CHECK(menu.rf_hz == bands[band].max_hz);
            menu.rf_hz = bands[band].min_hz + 1U;
            Radio_TuneSteps(-10); CHECK(menu.rf_hz == bands[band].min_hz);
            menu.rf_hz = bands[band].max_hz - 1U;
            Radio_TuneSteps(10); CHECK(menu.rf_hz == bands[band].max_hz);
        }
    }
    return 0;
}

int test_explicit_gen_and_channel_numbers_remain_in_range(void)
{
    InitControls(); Radio_SetFrequency(21469780U); Radio_TuneSteps(1);
    CHECK(menu.rf_hz == 21469880U);
    Radio_SetFrequency(RADIO_MAX_HZ); Radio_TuneSteps(2147483647);
    CHECK(menu.rf_hz == RADIO_MAX_HZ);
    Radio_SetFrequency(RADIO_MIN_HZ); Radio_TuneSteps(-2147483647 - 1);
    CHECK(menu.rf_hz == RADIO_MIN_HZ);
    MemoryControl_Move(-1); CHECK(MemoryControl_Channel() == 98U);
    MemoryControl_Move(1); CHECK(MemoryControl_Channel() == 0U);
    MemoryControl_Select(99U); CHECK(MemoryControl_Channel() == 0U);
    MemoryControl_Move(2147483647); CHECK(MemoryControl_Channel() < MEMORY_CHANNEL_COUNT);
    MemoryControl_Move(-2147483647 - 1); CHECK(MemoryControl_Channel() < MEMORY_CHANNEL_COUNT);
    return 0;
}

int test_input_digit_limit_by_band_and_channel(void)
{
    uint8_t band, i;
    char text[10];
    InitControls();
    for (band = 0U; band < Radio_BandsCount(); band++)
    {
        uint8_t expected = (bands[band].max_hz >= 10000000U) ? 7U : 6U;
        Radio_SetBand(band); Entry_Cancel(); Tap(Key_5_3, 100U);
        for (i = 0U; i < 12U; i++) Tap(Key_1_1, 100U);
        CHECK(menu.freq_input_len == expected);
        FreqEntry_Format(text);
        CHECK(text[expected - 5U] == '.' && text[expected - 1U] == '.');
        CHECK(text[expected + 2U] == '\0');
        CHECK(FreqEntry_Append('.') == 0U && menu.freq_input_len == expected);
    }
    Entry_Cancel(); Tap(Key_9_1, 100U); Tap(Key_5_3, 100U);
    for (i = 0U; i < 5U; i++) Tap(Key_1_1, 100U);
    CHECK(menu.freq_input_len == 2U);
    FreqEntry_Format(text); CHECK(strcmp(text, "11") == 0);
    return 0;
}

int test_func_restarts_frequency_after_error_without_waiting(void)
{
    uint8_t i;
    InitControls(); Tap(Key_5_3, 100U);
    for (i = 0U; i < 7U; i++) Tap(Key_4_3, 100U); /* 99.999.99 вне пределов. */
    Tap(Key_5_3, 100U);
    CHECK(strcmp(Entry_Status(), "INPUT ERR") == 0 && menu.rf_hz == 28200000U);
    now_tick += FUNC_TIMEOUT_TICKS - 200U; Entry_Task();
    CHECK(Entry_Active() != 0U);
    Tap(Key_5_3, 100U);
    CHECK(Entry_Active() != 0U && menu.freq_input_len == 0U && menu.freq_input[0] == '\0');
    CHECK(strcmp(Entry_Status(), "FUNC") == 0 && Entry_ChannelTarget() == 0U);
    now_tick += 500U; Entry_Task(); CHECK(Entry_Active() != 0U); /* Новый таймер. */
    Tap(Key_1_2, 100U); Tap(Key_1_1, 100U); Tap(Key_2_1, 100U);
    Tap(Key_2_1, 100U); Tap(Key_4_3, 100U); Tap(Key_4_1, 100U); Tap(Key_4_2, 100U);
    Tap(Key_5_3, 100U);
    CHECK(Entry_Active() == 0U && menu.rf_hz == 21449780U);
    return 0;
}

int test_channel_dot_backspaces_without_error_and_refreshes_timeout(void)
{
    InitControls(); Tap(Key_9_1, 100U); Tap(Key_5_3, 100U);
    Tap(Key_5_1, 100U); /* Пустой буфер: остаёмся в вводе без ошибки. */
    CHECK(Entry_Active() != 0U && menu.freq_input_len == 0U && strcmp(Entry_Status(), "FUNC") == 0);
    Tap(Key_1_1, 100U); Tap(Key_1_2, 100U);
    now_tick += FUNC_TIMEOUT_TICKS - 10U;
    Tap(Key_5_1, 2001U); /* Удержание также удаляет только одну цифру. */
    now_tick += 20U; Entry_Task();
    CHECK(Entry_Active() != 0U && strcmp(menu.freq_input, "1") == 0);
    Tap(Key_4_3, 100U); Tap(Key_5_3, 100U);
    CHECK(Entry_Active() == 0U && MemoryControl_Channel() == 18U);
    Tap(Key_5_3, 100U); Tap(Key_5_2, 100U); Tap(Key_5_2, 100U); Tap(Key_5_3, 100U);
    CHECK(strcmp(Entry_Status(), "INPUT ERR") == 0);
    Tap(Key_5_1, 100U);
    CHECK(strcmp(Entry_Status(), "FUNC") == 0 && strcmp(menu.freq_input, "0") == 0);
    Tap(Key_5_1, 100U); Tap(Key_4_3, 100U); Tap(Key_4_3, 100U); Tap(Key_5_3, 100U);
    CHECK(Entry_Active() == 0U && MemoryControl_Channel() == 98U);
    return 0;
}

int test_func_restarts_channel_after_error_and_preserves_target(void)
{
    InitControls(); Tap(Key_9_1, 100U); Tap(Key_5_3, 100U);
    Tap(Key_5_2, 100U); Tap(Key_5_2, 100U); Tap(Key_5_3, 100U);
    CHECK(strcmp(Entry_Status(), "INPUT ERR") == 0 && MemoryControl_Channel() == 0U);
    Tap(Key_5_3, 100U);
    CHECK(Entry_ChannelTarget() != 0U && menu.freq_input_len == 0U && Entry_Active() != 0U);
    CHECK(strcmp(Entry_Status(), "FUNC") == 0);
    Tap(Key_4_3, 100U); Tap(Key_4_3, 100U); Tap(Key_5_3, 100U);
    CHECK(Entry_Active() == 0U && MemoryControl_Channel() == 98U && menu.rf_hz == 28200000U);
    return 0;
}

/* Полный ввод семи цифр из двухзначного исходного диапазона, включая ведущий ноль. */
static void EnterHz(uint32_t hz)
{
    uint32_t scale;
    for (scale = 1000000U; scale != 0U; scale /= 10U)
        (void)FreqEntry_Append((char)('0' + ((hz / 10U) / scale) % 10U));
    Tap(Key_5_3, 100U);
}

int test_direct_entry_accepts_all_band_edges_and_switches_band(void)
{
    uint8_t band, edge;
    for (band = 0U; band < Radio_BandsCount(); band++)
    {
        for (edge = 0U; edge < 2U; edge++)
        {
            uint32_t hz = (edge == 0U) ? bands[band].min_hz : bands[band].max_hz;
            InitControls(); Tap(Key_5_3, 100U); EnterHz(hz);
            CHECK(Entry_Active() == 0U && menu.rf_hz == hz && menu.band_index == band);
            CHECK(synth_frequency == hz && Radio_FindBand(hz) == band);
        }
    }
    return 0;
}

int test_direct_entry_rejects_gaps_without_changing_tuning(void)
{
    uint8_t band, edge;
    for (band = 0U; band < Radio_BandsCount(); band++)
    {
        for (edge = 0U; edge < 2U; edge++)
        {
            uint32_t hz = (edge == 0U) ? bands[band].min_hz - 10U : bands[band].max_hz + 10U;
            InitControls(); Radio_SetFrequency(28200000U);
            Tap(Key_5_3, 100U); EnterHz(hz);
            CHECK(Entry_Active() != 0U && strcmp(Entry_Status(), "INPUT ERR") == 0);
            CHECK(menu.rf_hz == 28200000U && menu.band_index == 8U && synth_frequency == 28200000U);
            CHECK(Radio_FindBand(hz) == Radio_BandsCount());
        }
    }
    return 0;
}

int test_ten_mhz_error_func_restart_and_valid_ten_point_one_mhz(void)
{
    InitControls(); Radio_SetFrequency(28200000U);
    Tap(Key_5_3, 100U); EnterHz(10000000U);
    CHECK(strcmp(Entry_Status(), "INPUT ERR") == 0 && menu.rf_hz == 28200000U);
    Tap(Key_5_3, 100U); CHECK(menu.freq_input_len == 0U && Entry_Active() != 0U);
    EnterHz(10100000U);
    CHECK(Entry_Active() == 0U && menu.rf_hz == 10100000U && menu.band_index == 3U);
    return 0;
}

int test_invalid_direct_entry_preserves_memory_mode_and_channel(void)
{
    InitControls();
    cache_used[7] = 1U; cache[7] = (radio_preset_t){7100000U, RADIO_LSB, 0U, 0U, 2U};
    MemoryControl_Select(7U); MemoryControl_ToggleMode();
    CHECK(menu.rf_hz == 7100000U && MemoryControl_Mode() != 0U);
    Tap(Key_5_3, 100U);
    (void)FreqEntry_Append('8'); /* 8 МГц не входит ни в один диапазон. */
    Tap(Key_5_3, 100U);
    CHECK(strcmp(Entry_Status(), "INPUT ERR") == 0);
    CHECK(MemoryControl_Mode() != 0U && MemoryControl_Channel() == 7U);
    CHECK(menu.rf_hz == 7100000U && menu.band_index == 2U);
    return 0;
}
