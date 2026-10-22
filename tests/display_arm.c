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
