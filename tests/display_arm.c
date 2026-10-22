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

uint8_t display_snapshots[6][1024];
uint32_t display_count;
static const char *test_eeprom_message;

spi1_status_t SPI1_SendByte(uint8_t data) { (void)data; return SPI1_OK; }
const char *MemoryStore_Status(void) { return ""; }
const char *EEPROM_TestStatus(void) { return test_eeprom_message; }
void LCD_TestUpdate(void)
{
    if (display_count < 6U) memcpy(display_snapshots[display_count++], lcd_buffer, 1024U);
}

int test_display_snapshots(void)
{
    InitControls(); display_count = 0U; test_eeprom_message = "";
    Screens_Redraw();
    Tap(Key_5_3, 100U); Tap(Key_1_2, 100U); Tap(Key_1_1, 100U); Tap(Key_5_1, 100U);
    Screens_Redraw();
    Entry_Cancel();
    panel.att_mask = 7U; menu.gain_db = -42;
    panel.preamp = 1U; panel.nb = 1U; panel.comp = 1U; panel.tuner = 2U;
    RadioVfo_ChangeMode(1U); RadioVfo_ChangeMode(1U); RadioVfo_ChangeMode(0U);
    menu.last_key = Key_10_1;
    Screens_Redraw();
    InitControls(); test_eeprom_message = "";
    MemoryControl_Select(99U); MemoryControl_ToggleSelection(); MemoryControl_ToggleMode();
    Screens_Redraw();
    Tap(Key_6_3, 2001U); Screens_Redraw();
    InitControls(); test_eeprom_message = "CRC:FF00"; Screens_Redraw();
    CHECK(display_count == 6U);
    return 0;
}

static uint8_t Pixel(uint8_t x, uint8_t y)
{
    return (uint8_t)((lcd_buffer[(uint16_t)(y / 8U) * 128U + x] >> (y % 8U)) & 1U);
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
    for (x = 34U; x < 128U; x++) short_width = (uint8_t)(short_width + Pixel(x, 33U));
    while (*digits != '\0') FreqEntry_Append(*digits++);
    Screens_Redraw();
    for (x = 34U; x < 128U; x++)
    {
        CHECK(Pixel(x, 33U) == Pixel(x, 43U));
        if (Pixel(x, 33U) != 0U)
        { if (first == 128U) first = x; last = x; }
    }
    CHECK(first >= 34U && first < last && last < 128U);
    CHECK((uint8_t)(last - first + 1U) == short_width);
    CHECK((uint8_t)(first + 2U) == FrequencyUI_MainLeftX(menu.rf_hz));
    for (x = first; x <= last; x++) CHECK(Pixel(x, 33U) != 0U);
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
            if (Pixel(x, 33U) != 0U) { first = x; break; }
        CHECK((uint8_t)(first + 2U) == FrequencyUI_MainLeftX(menu.rf_hz));
    }
    return 0;
}
