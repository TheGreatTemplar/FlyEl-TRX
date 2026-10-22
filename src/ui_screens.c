/* Композиция рабочего экрана и сервисного меню; без управления периферией. */
#include "ui_screens.h"
#include "app_state.h"
#include "ui_frequency.h"
#include "ui_widgets.h"
#include "ui_smeter.h"
#include "eeprom_startup.h"
#include "gmg12864.h"

static void FormatTopStep(char *buf)
{
    uint32_t step;

    step = steps_hz[menu.step_index];

    if (step == 1UL)
    {
        buf[0] = '1';
        buf[1] = '\0';
    }
    else if (step == 10UL)
    {
        buf[0] = '1';
        buf[1] = '0';
        buf[2] = '\0';
    }
    else if (step == 50UL)
    {
        buf[0] = '5';
        buf[1] = '0';
        buf[2] = '\0';
    }
    else if (step == 100UL)
    {
        buf[0] = '1';
        buf[1] = '0';
        buf[2] = '0';
        buf[3] = '\0';
    }
    else if (step == 1000UL)
    {
        buf[0] = '1';
        buf[1] = 'k';
        buf[2] = '\0';
    }
    else
    {
        buf[0] = '1';
        buf[1] = '0';
        buf[2] = 'k';
        buf[3] = '\0';
    }
}

/* Собирает основной экран; координаты сохраняют проверенную на макете разметку. */
static void DrawMainScreen(void)
{
    char mode_text[4];
    char step_text[5];
    uint8_t tx_width;
    uint8_t band_x;
    uint8_t step_width;
    const char *key_name;

    LCD_ClearBuffer();

    /* TX stays in the upper-left corner. */
    tx_width = Widgets_Label(0U, 0U, "TX", 1U);

    if (menu.usb_active != 0U)
    {
        mode_text[0] = 'U';
        mode_text[1] = 'S';
        mode_text[2] = 'B';
        mode_text[3] = '\0';
    }
    else
    {
        mode_text[0] = 'L';
        mode_text[1] = 'S';
        mode_text[2] = 'B';
        mode_text[3] = '\0';
    }

    /* One blank pixel between TX and the LSB/USB frame. */
    (void)Widgets_Label((uint8_t)(tx_width + 1U), 0U, mode_text, 0U);

    FormatTopStep(step_text);
    step_width = LCD_GetText5x7Width(step_text, 1U);
    key_name = (EEPROM_TestStatus()[0] != '\0') ? EEPROM_TestStatus()
               : ((menu.last_key == Key_10_3) ? "TS" : Input_KeyName(menu.last_key));
    band_x = FrequencyUI_MainLeftX(menu.rf_hz);
    if (*key_name != '\0')
    {
        uint8_t band_width = LCD_GetText5x7Width(bands[menu.band_index].name, 1U);
        uint8_t key_width = LCD_GetText5x7Width(key_name, 1U);
        uint8_t max_band_x = (uint8_t)(LCD_WIDTH - step_width - band_width - key_width - 4U);
        if (band_x > max_band_x)
        {
            band_x = max_band_x;
        }
        LCD_Print5x7((uint8_t)(band_x + band_width + 2U), 2U, key_name, 1U, CH_UNINVERT);
    }
    LCD_Print5x7(band_x, 2U, bands[menu.band_index].name, 1U, CH_UNINVERT);
    LCD_Print5x7((uint8_t)(LCD_WIDTH - step_width), 2U, step_text, 1U, CH_UNINVERT);

    /* PREAMP / ATT status. Active gain/attenuation is inverted. */
    Widgets_Gain(0U, 12U);

    /* 11-pixel frames, separated by one blank row. */
    (void)Widgets_CompactLabel(0U, 24U, "SPLIT");
    (void)Widgets_CompactLabel(0U, 36U, "RIT");

    /* Main VFO A frequency. */
    FrequencyUI_DrawMain(menu.rf_hz, 14U);

    /* Align the label bottom with the visible bottom of the frequency digits. */
    {
        const char *label_left = "VFO";
        const char *label_right = "B";
        uint8_t freq_x;
        uint8_t label_right_x;
        uint8_t label_left_x;
        uint8_t vfo_width;
        uint8_t b_width;

        freq_x = FrequencyUI_SecondLeftX(VFO_B_DEMO_HZ);
        b_width = LCD_GetText5x7Width(label_right, 1U);
        /* The frequency digit has one empty leading column. */
        label_right_x = (freq_x >= b_width) ? (uint8_t)(freq_x - b_width) : 0U;
        vfo_width = LCD_GetText5x7Width(label_left, 1U);
        /* Keep four blank columns between VFO and B. */
        label_left_x = (label_right_x >= (uint8_t)(vfo_width + 4U))
                     ? (uint8_t)(label_right_x - vfo_width - 4U)
                     : 0U;

        LCD_Print5x7(label_left_x, 42U, label_left, 1U, CH_UNINVERT);
        LCD_Print5x7(label_right_x, 42U, label_right, 1U, CH_UNINVERT);
        FrequencyUI_DrawSecond(VFO_B_DEMO_HZ, 39U);
    }

    if (menu.freq_input_active != 0U)
    {
        LCD_FillRectangle(0U, 43U, 127U, 9U, PX_ON);
        LCD_Print(2U, 43U, "SET:", 1U, CH_INVERT);
        LCD_Print(32U, 43U, menu.freq_input, 1U, CH_INVERT);
    }
    else
    {
        /* For now the lower line is always the receive S-meter (ADC A0). */
        SMeter_Draw(menu.adc0_mv);
    }

    LCD_Update();
}

/* Рисует сервисное меню без изменения состояния радиотракта. */
static void DrawMenuScreen(void)
{
    char text[24];

    LCD_ClearBuffer();

    LCD_Print(0U, 0U, "MENU", 1U, CH_UNINVERT);
    LCD_Print(68U, 0U, "ESC - Out", 1U, CH_UNINVERT);
    {
        uint8_t selected;
        uint8_t invert;

        selected = (uint8_t)(menu.selected_item == 0U);

        Widgets_MenuLine(11U, "FREQ", "", selected);

        if (selected != 0U)
        {
            invert = CH_INVERT;
        }
        else
        {
            invert = CH_UNINVERT;
        }

        FrequencyUI_DrawKHz(49U, 12U, menu.rf_hz, invert);
    }

    Widgets_MenuLine(21U, "STEP", "", (uint8_t)(menu.selected_item == 1U));

    if (menu.selected_item == 1U)
    {
        if (steps_hz[menu.step_index] == 1UL)
        {
            LCD_Print(50U, 22U, "1", 1U, CH_INVERT);
            LCD_Print(58U, 22U, "Hz", 1U, CH_INVERT);
        }
        else if (steps_hz[menu.step_index] == 10UL)
        {
            LCD_Print(50U, 22U, "10", 1U, CH_INVERT);
            LCD_Print(65U, 22U, "Hz", 1U, CH_INVERT);
        }
        else if (steps_hz[menu.step_index] == 50UL)
        {
            LCD_Print(50U, 22U, "50", 1U, CH_INVERT);
            LCD_Print(65U, 22U, "Hz", 1U, CH_INVERT);
        }
        else if (steps_hz[menu.step_index] == 100UL)
        {
            LCD_Print(50U, 22U, "100", 1U, CH_INVERT);
            LCD_Print(72U, 22U, "Hz", 1U, CH_INVERT);
        }
        else if (steps_hz[menu.step_index] == 1000UL)
        {
            LCD_Print(50U, 22U, "1", 1U, CH_INVERT);
            LCD_Print(58U, 22U, "kHz", 1U, CH_INVERT);
        }
        else
        {
            LCD_Print(50U, 22U, "10", 1U, CH_INVERT);
            LCD_Print(65U, 22U, "kHz", 1U, CH_INVERT);
        }
    }
    else
    {
        if (steps_hz[menu.step_index] == 1UL)
        {
            LCD_Print(50U, 22U, "1", 1U, CH_UNINVERT);
            LCD_Print(58U, 22U, "Hz", 1U, CH_UNINVERT);
        }
        else if (steps_hz[menu.step_index] == 10UL)
        {
            LCD_Print(50U, 22U, "10", 1U, CH_UNINVERT);
            LCD_Print(65U, 22U, "Hz", 1U, CH_UNINVERT);
        }
        else if (steps_hz[menu.step_index] == 50UL)
        {
            LCD_Print(50U, 22U, "50", 1U, CH_UNINVERT);
            LCD_Print(65U, 22U, "Hz", 1U, CH_UNINVERT);
        }
        else if (steps_hz[menu.step_index] == 100UL)
        {
            LCD_Print(50U, 22U, "100", 1U, CH_UNINVERT);
            LCD_Print(72U, 22U, "Hz", 1U, CH_UNINVERT);
        }
        else if (steps_hz[menu.step_index] == 1000UL)
        {
            LCD_Print(50U, 22U, "1", 1U, CH_UNINVERT);
            LCD_Print(58U, 22U, "kHz", 1U, CH_UNINVERT);
        }
        else
        {
            LCD_Print(50U, 22U, "10", 1U, CH_UNINVERT);
            LCD_Print(65U, 22U, "kHz", 1U, CH_UNINVERT);
        }
    }

    Widgets_MenuLine(31U, "BAND", bands[menu.band_index].name, (uint8_t)(menu.selected_item == 2U));

    FrequencyUI_FormatIF(text);
    Widgets_MenuLine(41U, "IF", text, (uint8_t)(menu.selected_item == 3U));

    Widgets_MenuLine(51U, "OUT", "CLK1", (uint8_t)(menu.selected_item == 4U));

    LCD_Update();
}

/* Рисует текущий экран и сбрасывает запрос перерисовки. */
void Screens_Redraw(void)
{
    if (menu.menu_active != 0U)
    {
        DrawMenuScreen();
    }
    else
    {
        DrawMainScreen();
    }

    menu.need_redraw = 0U;
}
