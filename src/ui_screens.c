/* Композиция рабочего экрана и сервисного меню; без управления периферией. */
#include "ui_screens.h"
#include "app_state.h"
#include "ui_frequency.h"
#include "ui_widgets.h"
#include "ui_smeter.h"
#include "eeprom_startup.h"
#include "gmg12864.h"
#include "radio_vfo.h"
#include "key_layout.h"
#include "entry_controller.h"
#include "radio_panel.h"
#include "ui_status.h"
#include "ui_settings.h"
#include "text_utils.h"

/* Одинаковый шрифт и плоский фон по фактической ширине SET: и введённых символов. */
static void DrawFrequencyEntry(void)
{
    char text[FREQ_INPUT_MAX_LEN + 8U];
    uint8_t width;
    uint8_t x = 36U;
    text[0] = '\0';
    Text_Append(text, (Entry_ChannelTarget() != 0U) ? "CH: " : "SET: ");
    Text_Append(text, (menu.freq_input_len != 0U) ? menu.freq_input : "_");
    width = (uint8_t)(LCD_GetText5x7Width(text, 1U) + 4U);
    if ((uint16_t)x + width > LCD_WIDTH) x = (uint8_t)(LCD_WIDTH - width);
    LCD_FillRectangle(x, 33U, (uint8_t)(width - 1U), 10U, PX_ON);
    LCD_Print5x7((uint8_t)(x + 2U), 35U, text, 1U, CH_INVERT);
    LCD_Print5x7(0U, 56U, "F-INP:OK HOLD:CANCEL", 1U, CH_UNINVERT);
}

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
    const char *mode_text;
    char step_text[5];
    uint8_t tx_width;
    uint8_t mode_width;
    uint8_t band_x;
    uint8_t step_width;
    uint8_t header_fits = 0U;
    const char *key_name;
    const char *band_name;

    LCD_ClearBuffer();

    /* TX stays in the upper-left corner. */
    tx_width = Widgets_Label(0U, 0U, (menu.tx_active != 0U) ? "TX" : "RX", menu.tx_active);

    mode_text = RadioMode_Name(RadioVfo_Mode());

    /* One blank pixel between TX and the LSB/USB frame. */
    mode_width = Widgets_Label((uint8_t)(tx_width + 1U), 0U, mode_text, 0U);

    FormatTopStep(step_text);
    step_width = LCD_GetText5x7Width(step_text, 1U);
    key_name = StatusUI_Header();
    band_name = (menu.rf_hz >= bands[menu.band_index].min_hz && menu.rf_hz <= bands[menu.band_index].max_hz)
                ? bands[menu.band_index].name : "GEN";
    band_x = FrequencyUI_MainLeftX(menu.rf_hz);
    /* Длинные RTTY-R/CW-R не должны перекрывать диапазон и диагностическую строку. */
    if (*key_name != '\0')
    {
        uint8_t band_width = LCD_GetText5x7Width(band_name, 1U);
        uint8_t key_width = LCD_GetText5x7Width(key_name, 1U);
        uint8_t max_band_x = (uint8_t)(LCD_WIDTH - step_width - band_width - key_width - 4U);
        if (band_x > max_band_x)
        {
            band_x = max_band_x;
        }
        if (band_x < (uint8_t)(tx_width + mode_width + 2U))
            band_x = (uint8_t)(tx_width + mode_width + 2U);
        if ((uint16_t)band_x + band_width + key_width + 4U <= LCD_WIDTH - step_width)
        {
            LCD_Print5x7((uint8_t)(band_x + band_width + 2U), 2U, key_name, 1U, CH_UNINVERT);
            header_fits = 1U;
        }
    }
    if (band_x < (uint8_t)(tx_width + mode_width + 2U))
        band_x = (uint8_t)(tx_width + mode_width + 2U);
    LCD_Print5x7(band_x, 2U, band_name, 1U, CH_UNINVERT);
    LCD_Print5x7((uint8_t)(LCD_WIDTH - step_width), 2U, step_text, 1U, CH_UNINVERT);

    /* PREAMP / ATT status. Active gain/attenuation is inverted. */
    Widgets_Gain(0U, 12U);

    /* 11-pixel frames, separated by one blank row. */
    (void)Widgets_Label(0U, 24U, "SPLIT", RadioVfo_Split());
    {
        static const char *const meter[] = { "PO", "ALC", "SWR" };
        (void)Widgets_CompactLabel(0U, 36U, meter[panel.meter]);
    }

    /* Main VFO A frequency. */
    FrequencyUI_DrawMain(menu.rf_hz, 14U);

    /* Главные цифры заканчиваются на y=28, младшие начинаются с видимой y=31:
     * между ними две пустые строки 29/30. Скрываем весь VFO при вводе. */
    if (menu.freq_input_active == 0U)
    {
        const char *label_left = "VFO";
        const char *label_right = (RadioVfo_Active() == 0U) ? "B" : "A";
        uint8_t freq_x;
        uint8_t label_right_x;
        uint8_t label_left_x;
        uint8_t vfo_width;
        uint8_t b_width;

        freq_x = FrequencyUI_SecondLeftX(RadioVfo_OtherHz());
        b_width = LCD_GetText5x7Width(label_right, 1U);
        /* The frequency digit has one empty leading column. */
        label_right_x = (freq_x >= b_width) ? (uint8_t)(freq_x - b_width) : 0U;
        vfo_width = LCD_GetText5x7Width(label_left, 1U);
        /* Keep four blank columns between VFO and B. */
        label_left_x = (label_right_x >= (uint8_t)(vfo_width + 4U))
                     ? (uint8_t)(label_right_x - vfo_width - 4U)
                     : 0U;

        LCD_Print5x7(label_left_x, 33U, label_left, 1U, CH_UNINVERT);
        LCD_Print5x7(label_right_x, 33U, label_right, 1U, CH_UNINVERT);
        FrequencyUI_DrawSecond(RadioVfo_OtherHz(), 30U);
    }

    if (menu.freq_input_active != 0U)
    {
        DrawFrequencyEntry();
    }
    else
    {
        /* For now the lower line is always the receive S-meter (ADC A0). */
        SMeter_Draw(menu.adc0_mv);
    }

    StatusUI_Draw();
    if (header_fits == 0U && *key_name != '\0')
    {
        /* Диагностика не исчезает при длинном имени режима RTTY-R. */
        LCD_FillRectangle(36U, 44U, 91U, 6U, PX_OFF);
        LCD_Print5x7(36U, 44U, key_name, 1U, CH_UNINVERT);
    }

    LCD_Update();
}

/* Рисует текущий экран и сбрасывает запрос перерисовки. */
void Screens_Redraw(void)
{
    if (menu.menu_active != 0U)
    {
        SettingsUI_Draw();
    }
    else
    {
        DrawMainScreen();
    }

    menu.need_redraw = 0U;
}
