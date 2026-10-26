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
#include "keyboard.h"
#include "radio_vfo.h"
#include "radio_panel.h"
#include "memory_control.h"
#include "entry_controller.h"
#include "settings_control.h"
#include "scan_control.h"
#include "service_control.h"
#include "uart_store.h"
#include "eeprom_factory.h"

generator_menu_t menu;

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

        if (menu.menu_active == 0U || Service_Active() != 0U)
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
    menu.step_index = 3U; /* 100 Hz */
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
    menu.tx_active = 0U;
    menu.usb_active = 0U;
    menu.gain_db = 0;

    menu.rf_hz = bands[menu.band_index].start_hz;
    RadioVfo_Init();
    Keyboard_Init();
    RadioPanel_Init();
    MemoryControl_Init();
    UARTStore_Init();
    Entry_Init();
    Settings_Init();
    Scan_Init();
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

    menu.need_redraw = 1U;
}


/* Кооперативный проход: ввод и UI не ждут окончания записи EEPROM. */
void GeneratorMenu_Task(void)
{
    int32_t steps;
    input_key_t key;
    if (EEPROMFactory_Task() != 0U)
        menu.need_redraw = 1U;
    if (EEPROM_TestTask() != 0U)
        menu.need_redraw = 1U;
    if (UARTStore_Task() != 0U)
        menu.need_redraw = 1U;
    MemoryControl_Task();
    UpdateAdc();
    Entry_Task();
    RadioPanel_Task();
    key = GetPressedKey();
    if (key != INPUT_KEY_COUNT)
    {
        menu.last_key = key;
        menu.need_redraw = 1U;
    }
    Keyboard_Task(key);
    steps = Input_GetEncoderSteps();
    if (steps != 0)
    {
        if (Scan_Active() != 0U)
            Scan_Stop();
        if (Settings_Page() != 0U)
            Settings_Encoder(steps);
        else if (Entry_Active() == 0U)
        {
            if (MemoryControl_Selecting() != 0U)
                MemoryControl_Move(steps);
            else
                Radio_TuneSteps(steps);
        }
    }
    Scan_Task();
    if (menu.need_redraw != 0U)
        Screens_Redraw();
}
