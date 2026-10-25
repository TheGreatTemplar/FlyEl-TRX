/* Снимки настоящего framebuffer для проверки геометрии экрана после ARM-исполнения. */
#include "controls_arm.c"
#include "gmg12864.c"
#include "lcd_text.c"
#include "lcd_digits_large.c"
#include "lcd_digits_small.c"
#include "text_utils.c"
#include "ui_widgets.c"
#include "ui_frequency.c"
#include "ui_smeter.c"
void LCD_TestUpdate(void);
#define LCD_Update LCD_TestUpdate
#include "ui_settings.c"
#include "ui_status.c"
#include "ui_screens.c"
#undef LCD_Update

uint8_t display_snapshots[7][1024];
uint32_t display_count;
static const char *test_eeprom_message;

spi1_status_t SPI1_SendByte(uint8_t data) { (void)data; return SPI1_OK; }
const char *MemoryStore_Status(void) { return ""; }
const char *EEPROM_TestStatus(void) { return test_eeprom_message; }
void LCD_TestUpdate(void)
{
    if (display_count < 7U) memcpy(display_snapshots[display_count++], lcd_buffer, 1024U);
}

int test_display_snapshots(void)
{
    InitControls(); display_count = 0U; test_eeprom_message = "";
    Screens_Redraw();
    Tap(Key_5_3, 100U); Tap(Key_1_2, 100U); Tap(Key_1_1, 100U); Tap(Key_5_1, 100U);
    Screens_Redraw();
    Entry_Cancel();
    panel.att_mask = 5U; menu.gain_db = -30;
    panel.preamp = 1U; panel.nb = 1U; panel.comp = 1U; panel.tuner = 2U;
    RadioVfo_ChangeMode(1U); RadioVfo_ChangeMode(1U); RadioVfo_ChangeMode(0U);
    menu.last_key = Key_10_1;
    Screens_Redraw();
    InitControls(); test_eeprom_message = "";
    MemoryControl_Select(98U); MemoryControl_ToggleSelection(); MemoryControl_ToggleMode();
    Screens_Redraw();
    Tap(Key_6_3, 2001U); Screens_Redraw();
    InitControls(); test_eeprom_message = "CRC:FF00"; Screens_Redraw();
    InitControls(); test_eeprom_message = ""; Tap(Key_7_1, 100U); Screens_Redraw();
    CHECK(display_count == 7U);
    return 0;
}

static uint8_t Pixel(uint8_t x, uint8_t y)
{
    return (uint8_t)((lcd_buffer[(uint16_t)(y / 8U) * 128U + x] >> (y % 8U)) & 1U);
}

/* Проверяет обе фазы мигания, двузначные номера и неподвижность всей строки. */
int test_display_memory_blink_keeps_two_digit_channel_and_status_fixed(void)
{
    static const uint8_t channels[] = {0U, 8U, 9U, 98U};
    static const char *const labels[] = {"01", "09", "10", "99"};
    uint8_t before[1024];
    uint8_t i, occupied, x, y, memo_on, memo_off;
    for (i = 0U; i < 4U; i++)
    {
        for (occupied = 0U; occupied < 2U; occupied++)
        {
            char expected[20] = "";
            InitControls(); test_eeprom_message = "";
            cache_used[channels[i]] = occupied;
            MemoryControl_Select(channels[i]); MemoryControl_ToggleSelection();
            now_tick = 0U; Screens_Redraw();
            memcpy(before, lcd_buffer, sizeof(before));
            now_tick = 1000U; Screens_Redraw();
            memo_on = 0U; memo_off = 0U;
            for (y = 42U; y < 51U; y++)
            {
                for (x = 0U; x < 128U; x++)
                {
                    uint8_t old = (uint8_t)((before[(y / 8U) * 128U + x] >> (y % 8U)) & 1U);
                    if (x >= 47U && x < 70U) { memo_on |= old; memo_off |= Pixel(x, y); }
                    else CHECK(old == Pixel(x, y));
                }
            }
            CHECK(memo_on != 0U && memo_off == 0U);
            Text_Append(expected, labels[i]);
            LCD_ClearBuffer();
            LCD_DrawDigit8x9(111U, 42U, (uint8_t)expected[0], CH_UNINVERT);
            LCD_DrawDigit8x9(120U, 42U, (uint8_t)expected[1], CH_UNINVERT);
            if (occupied == 0U)
            {
                LCD_FillRectangle(73U, 42U, 29U, 8U, PX_ON);
                LCD_Print5x7(74U, 43U, "BLANK", 1U, CH_INVERT);
            }
            for (y = 42U; y < 51U; y++)
                for (x = 70U; x < 128U; x++)
                    CHECK(((before[(y / 8U) * 128U + x] >> (y % 8U)) & 1U) == Pixel(x, y));
        }
    }
    return 0;
}

/* Сравнивает фрагмент кадра с эталонной надписью, сохраняя остальной кадр. */
static uint8_t MatchesText(uint8_t x, uint8_t y, const char *text)
{
    uint8_t actual[1024];
    uint8_t col, row, equal = 1U;
    uint8_t width = LCD_GetText5x7Width(text, 1U);
    memcpy(actual, lcd_buffer, sizeof(actual));
    LCD_ClearBuffer(); LCD_Print5x7(x, y, text, 1U, CH_UNINVERT);
    for (row = y; row < (uint8_t)(y + 7U); row++)
        for (col = x; col < (uint8_t)(x + width); col++)
            if (((actual[(row / 8U) * 128U + col] >> (row % 8U)) & 1U) != Pixel(col, row)) equal = 0U;
    memcpy(lcd_buffer, actual, sizeof(actual));
    return equal;
}

static uint8_t MatchesChannel99(void)
{
    uint8_t actual[1024];
    uint8_t x, y, equal = 1U;
    memcpy(actual, lcd_buffer, sizeof(actual));
    LCD_ClearBuffer();
    LCD_DrawDigit8x9(111U, 42U, '9', CH_UNINVERT);
    LCD_DrawDigit8x9(120U, 42U, '9', CH_UNINVERT);
    for (y = 42U; y < 51U; y++)
        for (x = 111U; x < 128U; x++)
            if (((actual[(y / 8U) * 128U + x] >> (y % 8U)) & 1U) != Pixel(x, y)) equal = 0U;
    memcpy(lcd_buffer, actual, sizeof(actual));
    return equal;
}

int test_display_permanent_channel_vfo_selection_memory_and_input(void)
{
    uint8_t x, y, i;
    InitControls(); test_eeprom_message = "";
    cache_used[98] = 1U; cache[98] = (radio_preset_t){7100000U, RADIO_LSB, 0U, 0U, 2U};
    MemoryControl_Select(98U); Screens_Redraw();
    CHECK(MatchesChannel99() != 0U && MatchesText(47U, 44U, "F1") != 0U);
    Tap(Key_9_1, 100U); now_tick = 0U; Screens_Redraw();
    CHECK(menu.rf_hz == 28200000U && MemoryControl_Mode() == 0U);
    CHECK(MatchesText(47U, 44U, "MEMO") != 0U && MatchesChannel99() != 0U);
    now_tick = 1000U; Screens_Redraw();
    for (y = 44U; y < 51U; y++)
        for (x = 47U; x < 70U; x++) CHECK(Pixel(x, y) == 0U);
    CHECK(MatchesChannel99() != 0U);
    Tap(Key_9_1, 100U); Tap(Key_2_1, 2001U); Screens_Redraw();
    CHECK(MemoryControl_Mode() == 0U && cache[98].hz == 28200000U && store_calls == 1U);
    CHECK(MatchesChannel99() != 0U);
    Tap(Key_1_1, 100U); now_tick = 1000U; Screens_Redraw();
    CHECK(MatchesText(47U, 44U, "MEMO") != 0U && MatchesChannel99() != 0U);
    Tap(Key_1_1, 100U); Tap(Key_5_3, 100U);
    for (i = 0U; i < 7U; i++) Tap(Key_4_3, 100U);
    Screens_Redraw(); CHECK(MatchesChannel99() != 0U);
    Tap(Key_5_3, 100U); Screens_Redraw(); /* INPUT ERR переносится под VFO. */
    CHECK(strcmp(Entry_Status(), "INPUT ERR") == 0 && MatchesChannel99() != 0U);
    CHECK(menu.rf_hz == 28200000U);
    return 0;
}

int test_display_saved_channel_blinks_three_times_without_moving_screen(void)
{
    uint8_t baseline[1024], actual[1024];
    uint8_t phase, x, y;
    InitControls(); test_eeprom_message = "";
    MemoryControl_Select(98U); Screens_Redraw();
    memcpy(baseline, lcd_buffer, sizeof(baseline));
    mock_saved_channel = 98U;
    for (phase = 0U; phase <= 6U; phase++)
    {
        now_tick = (uint32_t)phase * 400U;
        MemoryControl_Task();
        CHECK(menu.need_redraw != 0U);
        CHECK(MemoryControl_NumberVisible() == (uint8_t)(phase == 6U || (phase & 1U) != 0U));
        Screens_Redraw(); memcpy(actual, lcd_buffer, sizeof(actual));
        memcpy(lcd_buffer, baseline, sizeof(baseline));
        if (phase < 6U && (phase & 1U) == 0U)
            for (y = 42U; y < 51U; y++)
                for (x = 111U; x < 128U; x++) LCD_DrawPixel(x, y, PX_OFF);
        CHECK(memcmp(actual, lcd_buffer, sizeof(actual)) == 0);
    }
    menu.need_redraw = 0U; now_tick += 4000U; MemoryControl_Task();
    CHECK(MemoryControl_NumberVisible() != 0U && menu.need_redraw == 0U);
    return 0;
}

int test_save_blink_handles_tick_wrap_channel_change_and_repeat_save(void)
{
    InitControls();
    now_tick = 0xFFFFFF00U; mock_saved_channel = 0U; MemoryControl_Task();
    CHECK(MemoryControl_NumberVisible() == 0U);
    MemoryControl_Select(1U); CHECK(MemoryControl_NumberVisible() != 0U);
    MemoryControl_Select(0U); now_tick += 400U; MemoryControl_Task();
    CHECK(MemoryControl_NumberVisible() != 0U);
    now_tick += 400U; MemoryControl_Task(); CHECK(MemoryControl_NumberVisible() == 0U);
    mock_saved_channel = 0U; MemoryControl_Task();
    now_tick += 2000U; MemoryControl_Task(); CHECK(MemoryControl_NumberVisible() != 0U);
    now_tick += 400U; MemoryControl_Task(); CHECK(MemoryControl_NumberVisible() != 0U);
    return 0;
}

int test_display_band_vfo_and_filter_share_fixed_column(void)
{
    uint8_t band;
    InitControls(); test_eeprom_message = "";
    panel.nb = 1U; panel.comp = 1U; panel.vox = 1U; panel.tuner = 2U;
    RadioVfo_CycleFilter(); RadioVfo_CycleFilter();
    for (band = 0U; band < Radio_BandsCount(); band++)
    {
        Radio_SetBand(band); RadioVfo_CopyAToB(); Screens_Redraw();
        CHECK(MatchesText(47U, 2U, bands[band].name) != 0U);
        CHECK(MatchesText(47U, 33U, "VFOB") != 0U);
        CHECK(MatchesText(47U, 44U, "F3") != 0U);
    }
    RadioVfo_Switch(); Screens_Redraw();
    CHECK(MatchesText(47U, 33U, "VFOA") != 0U);
    return 0;
}

int test_display_filter_hidden_in_memory_with_or_without_channel_selection(void)
{
    InitControls(); test_eeprom_message = "";
    MemoryControl_ToggleMode(); Screens_Redraw();
    CHECK(MatchesText(47U, 44U, "MEMO") != 0U);
    CHECK(Pixel(112U, 42U) != 0U && Pixel(123U, 42U) != 0U);
    CHECK(Pixel(73U, 42U) != 0U && Pixel(102U, 50U) != 0U);
    CHECK(MatchesText(103U, 44U, "  ") != 0U);
    MemoryControl_ToggleMode(); Screens_Redraw();
    CHECK(MatchesText(47U, 44U, "F1") != 0U);
    return 0;
}

int test_display_exact_two_pixel_frequency_gap(void)
{
    uint8_t x, main_pixels = 0U, second_pixels = 0U;
    InitControls(); test_eeprom_message = ""; Screens_Redraw();
    for (x = 45U; x < 128U; x++)
    {
        CHECK(Pixel(x, 29U) == 0U && Pixel(x, 30U) == 0U);
        main_pixels |= Pixel(x, 28U);
        second_pixels |= Pixel(x, 31U);
    }
    CHECK(main_pixels != 0U && second_pixels != 0U);
    return 0;
}

int test_display_input_background_is_flat_and_fits_maximum_length(void)
{
    uint8_t x, first = 128U, last = 0U, short_width = 0U;
    const char *digits = "12345678901";
    InitControls(); test_eeprom_message = ""; Tap(Key_5_3, 100U); Screens_Redraw();
    for (x = 34U; x < 128U; x++) short_width = (uint8_t)(short_width + Pixel(x, 30U));
    while (*digits != '\0') FreqEntry_Append(*digits++);
    Screens_Redraw();
    for (x = 34U; x < 128U; x++)
    {
        CHECK(Pixel(x, 30U) == Pixel(x, 40U));
        if (Pixel(x, 30U) != 0U)
        { if (first == 128U) first = x; last = x; }
    }
    CHECK(first >= 34U && first < last && last < 128U);
    CHECK((uint8_t)(last - first + 1U) == short_width);
    CHECK((uint8_t)(first + 2U) == FrequencyUI_MainLeftX(menu.rf_hz));
    for (x = first; x <= last; x++) CHECK(Pixel(x, 30U) != 0U);
    return 0;
}

int test_display_set_aligns_with_main_frequency_on_every_band(void)
{
    uint8_t band, x, first;
    InitControls(); test_eeprom_message = "";
    for (band = 0U; band < Radio_BandsCount(); band++)
    {
        Radio_SetBand(band); Entry_Cancel(); Tap(Key_5_3, 100U); Screens_Redraw();
        first = 128U;
        for (x = 34U; x < 128U; x++)
            if (Pixel(x, 30U) != 0U) { first = x; break; }
        CHECK((uint8_t)(first + 2U) == FrequencyUI_MainLeftX(menu.rf_hz));
    }
    return 0;
}
