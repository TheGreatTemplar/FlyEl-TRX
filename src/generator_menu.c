/* Координатор приложения: события ввода, фоновые задачи и выбор экрана. */
#include "generator_menu.h"
#include "app_state.h"
#include "app_time.h"
#include "radio_control.h"
#include "freq_entry.h"
#include "ui_screens.h"
#include "eeprom_startup.h"
#include "si5351.h"
#include "adc1_app.h"
#include "uart1.h"

generator_menu_t menu;

/* Применяет действие к выбранной строке сервисного меню. */
static void ModifySelected(int8_t dir)
{
    if (menu.selected_item == 0U)
    {
        FreqEntry_Start('\0');
        menu.menu_active = 0U;
    }
    else if (menu.selected_item == 1U)
    {
        Radio_CycleStep();
    }
    else if (menu.selected_item == 2U)
    {
        if (dir >= 0)
        {
            Radio_SetBand((uint8_t)(menu.band_index + 1U));
        }
        else
        {
            if (menu.band_index == 0U)
            {
                Radio_SetBand((uint8_t)(Radio_BandsCount() - 1U));
            }
            else
            {
                Radio_SetBand((uint8_t)(menu.band_index - 1U));
            }
        }
    }
    else if (menu.selected_item == 3U)
    {
        Radio_CycleIF();
    }
    else
    {
        Radio_ApplySynth();
        menu.need_redraw = 1U;
    }
}

/* Обновляет измерения каждые 200 мс без задержки основного цикла. */
static void UpdateAdc(void)
{
    uint32_t now;

    now = App_GetTick500us();

    if ((uint32_t)(now - menu.adc_update_tick) >= 400U)
    {
        menu.adc_update_tick = now;

        menu.adc0_mv = ADC1_App_ReadMv(ADC_APP_CH0, ADC_VREF_MV);
        menu.adc1_mv = ADC1_App_ReadMv(ADC_APP_CH1, ADC_VREF_MV);

        if (menu.menu_active == 0U)
        {
            menu.need_redraw = 1U;
        }
    }
}

/* Извлекает одно событие клавиатуры; удержание читается отдельно драйвером ввода. */
static input_key_t GetPressedKey(void)
{
    uint8_t i;

    for (i = 0U; i < (uint8_t)INPUT_KEY_COUNT; i++)
    {
        if (Input_KeyPressed((input_key_t)i) != 0U)
        {
            return (input_key_t)i;
        }
    }

    return INPUT_KEY_COUNT;
}

/* Инициализирует состояние, синтезатор, TS и стартовую проверку EEPROM. */
void GeneratorMenu_Init(void)
{
    si5351_status_t st;

    menu.band_index = 8U;
    menu.step_index = 3U;    /* 100 Hz */
    Radio_InitTS();
    menu.if_index = 0U;
    menu.menu_active = 0U;
    menu.freq_input_active = 0U;
    menu.selected_item = 0U;
    menu.need_redraw = 1U;
    menu.si5351_ok = 0U;
    menu.last_key = INPUT_KEY_COUNT;
    menu.freq_input_len = 0U;
    menu.freq_input_dot = 0U;
    menu.freq_input[0] = '\0';
    menu.tx_active = 1U;
    menu.usb_active = 0U;
    menu.gain_db = -36;

    menu.rf_hz = bands[menu.band_index].start_hz;
    menu.adc_update_tick = App_GetTick500us();
    menu.adc0_mv = 0U;
    menu.adc1_mv = 0U;

    st = Si5351_Init();

    if (st == SI5351_OK)
    {
        menu.si5351_ok = 1U;
        (void)Si5351_SetFreq(REF_CLK, 500000UL);
        Radio_ApplySynth();
    }
    else
    {
        menu.si5351_ok = 0U;
    }

    UART1_SendString("GMG12864 Si5351 generator start\r\n");

    EEPROM_TestInit();

    Screens_Redraw();
}

/* Выполняет один проход: EEPROM, измерения, клавиши, энкодер и перерисовка. */
void GeneratorMenu_Task(void)
{
    int32_t steps;
    input_key_t key;
    char ch;

    if (EEPROM_TestTask() != 0U)
    {
        menu.need_redraw = 1U;
    }

    UpdateAdc();

    key = GetPressedKey();

    if (key != INPUT_KEY_COUNT)
    {
        menu.last_key = key;
        menu.need_redraw = 1U;
    }

    Radio_UpdateTS(key);

    if (key >= Key_2_1 && key < INPUT_KEY_COUNT)
    {
        key = INPUT_KEY_COUNT;
    }

    if (menu.freq_input_active != 0U)
    {
        if (key == INPUT_KEY_ESC)
        {
            FreqEntry_Cancel();
        }
        else if (key == INPUT_KEY_ENTER)
        {
            FreqEntry_Apply();
        }
        else if (FreqEntry_IsCharacterKey(key) != 0U)
        {
            ch = Input_KeyToChar(key);
            FreqEntry_Append(ch);
        }

        if (menu.need_redraw != 0U)
        {
            Screens_Redraw();
        }

        return;
    }

    steps = Input_GetEncoderSteps();

    if (menu.menu_active == 0U)
    {
        if (steps != 0)
        {
            Radio_TuneSteps(steps);
        }

        if (key == INPUT_KEY_STEP)
        {
            Radio_CycleStep();
        }
        else if (key == INPUT_KEY_BAND)
        {
            Radio_SetBand((uint8_t)(menu.band_index + 1U));
        }
        else if (key == INPUT_KEY_MODE)
        {
            Radio_CycleIF();
        }
        else if (key == INPUT_KEY_ENTER)
        {
            menu.menu_active = 1U;
            menu.selected_item = 0U;
            menu.need_redraw = 1U;
        }
        else if (FreqEntry_IsCharacterKey(key) != 0U)
        {
            ch = Input_KeyToChar(key);
            FreqEntry_Start(ch);
        }
    }
    else
    {
        if (steps > 0)
        {
            while (steps > 0)
            {
                menu.selected_item++;

                if (menu.selected_item >= MENU_ITEMS)
                {
                    menu.selected_item = 0U;
                }

                steps--;
            }

            menu.need_redraw = 1U;
        }
        else if (steps < 0)
        {
            while (steps < 0)
            {
                if (menu.selected_item == 0U)
                {
                    menu.selected_item = MENU_ITEMS - 1U;
                }
                else
                {
                    menu.selected_item--;
                }

                steps++;
            }

            menu.need_redraw = 1U;
        }

        if (key == INPUT_KEY_ENTER)
        {
            ModifySelected(1);
        }
        else if (key == INPUT_KEY_ESC)
        {
            menu.menu_active = 0U;
            menu.need_redraw = 1U;
        }
        else if (key == INPUT_KEY_STEP)
        {
            ModifySelected(1);
        }
        else if (key == INPUT_KEY_BAND)
        {
            ModifySelected(-1);
        }
    }

    if (menu.need_redraw != 0U)
    {
        Screens_Redraw();
    }
}
