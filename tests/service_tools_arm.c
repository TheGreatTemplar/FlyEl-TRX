#include "controls_arm.c"

static void OpenServiceItem(uint8_t section, uint8_t item)
{
    (void)Settings_OpenService();
    Service_Encoder(section);
    Service_HandleKey(Key_9_2);
    Service_Encoder(item);
    Service_HandleKey(Key_5_3);
}


int test_lcd_test_navigation_timing_pause_resume_and_no_radio_changes(void)
{
    uint8_t page;
    uint32_t saves;
    uint8_t contrast;
    InitControls();
    saves = mock_uart_saves;
    contrast = LCD_GetContrast();
    OpenServiceItem(13U, 1U);
    CHECK(Service_Stage() == SERVICE_LCD_TEST && LCDTest_Page() == LCD_TEST_ON);
    now_tick += 1999U;
    CHECK(LCDTest_Task() == 0U && LCDTest_Page() == LCD_TEST_ON);
    now_tick++;
    CHECK(LCDTest_Task() != 0U && LCDTest_Page() == LCD_TEST_OFF);
    for (page = LCD_TEST_CHECK; page < LCD_TEST_COUNT; page++)
    {
        now_tick += (LCDTest_Page() == LCD_TEST_SWEEP_X) ? 5120U : (LCDTest_Page() == LCD_TEST_SWEEP_Y) ? 2560U : 2000U;
        CHECK(LCDTest_Task() != 0U && LCDTest_Page() == page);
    }
    now_tick += 2000U;
    CHECK(LCDTest_Task() != 0U && LCDTest_Page() == LCD_TEST_ON);
    Service_Encoder(-1);
    CHECK(LCDTest_Page() == LCD_TEST_FONT_OTHER && LCDTest_Auto() == 0U);
    now_tick += 100000U;
    CHECK(LCDTest_Task() == 0U && LCDTest_Page() == LCD_TEST_FONT_OTHER);
    Service_Encoder(2147483647);
    CHECK(LCDTest_Page() < LCD_TEST_COUNT);
    Service_Encoder(-2147483647 - 1);
    CHECK(LCDTest_Page() < LCD_TEST_COUNT);
    Service_HandleKey(Key_5_3);
    CHECK(LCDTest_Auto() != 0U);
    Tap(Key_10_1, 2001U);
    Tap(Key_10_3, 2001U);
    CHECK(menu.rf_hz == 28200000U && menu.step_index == 3U && RadioVfo_Mode() == RADIO_LSB);
    CHECK(mock_uart_saves == saves && store_calls == 0U && LCD_GetContrast() == contrast);
    Service_HandleKey(Key_9_3);
    CHECK(Service_Stage() == SERVICE_SECTION && Service_Item() == 1U);
    now_tick += 10000U;
    CHECK(LCDTest_Task() == 0U);
    Service_HandleKey(Key_9_2);
    CHECK(LCDTest_Page() == LCD_TEST_ON && LCDTest_Auto() != 0U);
    Service_HandleKey(Key_5_1);
    CHECK(Service_Stage() == SERVICE_SECTION);
    return 0;
}


int test_lcd_test_sweep_complete_endpoints_manual_animation_and_tick_wrap(void)
{
    InitControls();
    now_tick = 0xFFFFFFF0U;
    OpenServiceItem(13U, 1U);
    Service_Encoder(LCD_TEST_SWEEP_X);
    CHECK(LCDTest_Position() == 0U && LCDTest_Auto() == 0U);
    now_tick += 39U;
    CHECK(LCDTest_Task() == 0U);
    now_tick++;
    CHECK(LCDTest_Task() != 0U && LCDTest_Position() == 1U);
    now_tick += 126U * 40U;
    CHECK(LCDTest_Task() != 0U && LCDTest_Position() == 127U);
    now_tick += 40U;
    CHECK(LCDTest_Task() != 0U && LCDTest_Position() == 0U && LCDTest_Page() == LCD_TEST_SWEEP_X);
    Service_HandleKey(Key_9_2);
    now_tick += 127U * 40U;
    CHECK(LCDTest_Task() != 0U && LCDTest_Position() == 127U);
    now_tick += 40U;
    CHECK(LCDTest_Task() != 0U && LCDTest_Page() == LCD_TEST_SWEEP_Y && LCDTest_Position() == 0U);
    now_tick += 63U * 40U;
    CHECK(LCDTest_Task() != 0U && LCDTest_Position() == 63U);
    now_tick += 40U;
    CHECK(LCDTest_Task() != 0U && LCDTest_Page() == LCD_TEST_FONT_MAIN);
    Service_HandleKey(Key_5_1);
    return 0;
}


int test_factory_menu_confirmation_cancel_tx_and_restart(void)
{
    InitControls();
    OpenServiceItem(16U, 2U);
    CHECK(Service_Stage() == SERVICE_FACTORY && mock_factory_starts == 0U);
    Service_HandleKey(Key_5_3);
    CHECK(Service_Stage() == SERVICE_SECTION && mock_factory_starts == 0U);
    Service_HandleKey(Key_5_3);
    Service_Encoder(1);
    CHECK(ServiceFactory_Confirmed() != 0U);
    Service_HandleKey(Key_9_3);
    CHECK(Service_Stage() == SERVICE_SECTION && mock_factory_starts == 0U);
    Service_HandleKey(Key_5_3);
    CHECK(ServiceFactory_Confirmed() == 0U);
    Service_Encoder(1);
    menu.tx_active = 1U;
    Service_HandleKey(Key_5_3);
    CHECK(mock_factory_starts == 0U && strcmp(ServiceFactory_Status(), "STOP TX FIRST") == 0);
    menu.tx_active = 0U;
    Service_HandleKey(Key_5_3);
    CHECK(mock_factory_starts == 1U && Service_Stage() == SERVICE_FACTORY);
    Service_Encoder(-1);
    Service_HandleKey(Key_5_1);
    CHECK(Service_Stage() == SERVICE_FACTORY && ServiceFactory_Confirmed() != 0U);
    mock_factory_state = EEPROM_FACTORY_DONE;
    Service_HandleKey(Key_9_3);
    CHECK(mock_reboots == 0U);
    Service_HandleKey(Key_5_3);
    CHECK(mock_reboots == 1U);
    mock_factory_state = EEPROM_FACTORY_IDLE;
    return 0;
}


int test_contrast_live_preview_limits_cancel_and_verified_save(void)
{
    uint32_t before;
    InitControls();
    before = mock_uart_saves;
    OpenServiceItem(13U, 0U);
    CHECK(Service_Stage() == SERVICE_CONTRAST && LCD_GetContrast() == 15U);
    Service_Encoder(5);
    CHECK(LCD_GetContrast() == 20U && mock_uart_saves == before);
    Service_HandleKey(Key_5_1);
    CHECK(Service_Stage() == SERVICE_SECTION && LCD_GetContrast() == 15U);
    Service_HandleKey(Key_9_2);
    Service_Encoder(2147483647);
    CHECK(LCD_GetContrast() == 63U);
    Service_Encoder(-2147483647 - 1);
    CHECK(LCD_GetContrast() == 0U);
    Service_HandleKey(Key_9_3);
    CHECK(LCD_GetContrast() == 15U);
    Service_HandleKey(Key_5_3);
    Service_Encoder(22);
    Service_HandleKey(Key_5_3);
    CHECK(mock_uart_saves == before + 1U && Service_Stage() == SERVICE_CONTRAST);
    CHECK(ServiceContrast_Task() == 1U);
    CHECK(strcmp(ServiceContrast_Status(), "SAVED EEPROM") == 0);
    Service_Encoder(1);
    CHECK(LCD_GetContrast() == 38U);
    Service_HandleKey(Key_5_1);
    CHECK(LCD_GetContrast() == 37U && Service_Stage() == SERVICE_SECTION);
    return 0;
}


int test_service_uart_editor_cancel_validation_and_apply(void)
{
    char value[16];
    InitControls();
    OpenServiceItem(14U, 3U);
    CHECK(Service_Stage() == SERVICE_UART);
    Tap(Key_9_2, 100U);
    Settings_Encoder(1);
    ServiceUart_Value(0U, value);
    CHECK(strcmp(value, "1200") == 0);
    Tap(Key_9_3, 100U);
    ServiceUart_Value(0U, value);
    CHECK(strcmp(value, "115200") == 0);
    Tap(Key_5_3, 100U);
    Settings_Encoder(-1);
    Tap(Key_5_3, 100U);
    Settings_Encoder(1);
    Tap(Key_9_2, 100U);
    Settings_Encoder(1);
    Tap(Key_9_2, 100U);
    Settings_Encoder(1);
    Tap(Key_9_2, 100U);
    Settings_Encoder(1);
    Tap(Key_9_2, 100U);
    Settings_Encoder(2);
    Tap(Key_9_2, 100U);
    CHECK(strcmp(ServiceUart_Status(), "9 DATA: NO PARITY") == 0 && mock_uart_saves == 0U);
    Settings_Encoder(-2);
    Tap(Key_9_2, 100U);
    Settings_Encoder(-1);
    Tap(Key_9_2, 100U);
    Settings_Encoder(1);
    Tap(Key_9_2, 100U);
    Settings_Encoder(1);
    Tap(Key_9_2, 100U);
    Settings_Encoder(1);
    Tap(Key_5_3, 100U);
    CHECK(mock_uart_saves == 1U && mock_uart_config.baud == 57600U);
    CHECK(mock_uart_config.data_bits == 9U && mock_uart_config.parity == UART_PARITY_NONE &&
          mock_uart_config.stop == UART_STOP_TWO);
    Tap(Key_5_1, 100U);
    CHECK(Service_Stage() == SERVICE_SECTION && Entry_Active() == 0U);
    return 0;
}


int test_keyboard_diagnostic_all_27_keys_and_exit_without_commands(void)
{
    uint8_t i;
    InitControls();
    OpenServiceItem(13U, 9U);
    CHECK(Service_Stage() == SERVICE_KEY_TEST);
    Keyboard_Task(INPUT_KEY_COUNT);
    CHECK(KeyTest_Ready() != 0U);
    for (i = 0U; i < 27U; i++)
    {
        input_key_t key = (input_key_t)((uint8_t)Key_2_1 + i);
        key_down[key] = 1U;
        Keyboard_Task(key);
        CHECK(KeyTest_LastKey() == key && KeyTest_Event() == KEYTEST_DOWN);
        CHECK((KeyTest_DownMask() & (1UL << i)) != 0U);
        now_tick += 200U;
        key_down[key] = 0U;
        Keyboard_Task(INPUT_KEY_COUNT);
        CHECK(KeyTest_Event() == KEYTEST_SHORT && KeyTest_DurationMs() == 100U);
    }
    CHECK(store_calls == 0U && menu.step_index == 3U && RadioVfo_Mode() == RADIO_LSB && RadioVfo_Filter() == 0U);
    CHECK(MemoryControl_Mode() == 0U && panel.preamp == 0U && panel.nb == 0U && panel.tuner == 0U);
    key_down[Key_10_3] = 1U;
    Keyboard_Task(Key_10_3);
    now_tick += 2001U;
    Keyboard_Task(INPUT_KEY_COUNT);
    CHECK(KeyTest_Event() == KEYTEST_LONG);
    key_down[Key_10_3] = 0U;
    Keyboard_Task(INPUT_KEY_COUNT);
    CHECK(menu.step_index == 3U && KeyTest_Event() == KEYTEST_LONG);
    key_down[Key_9_3] = 1U;
    Keyboard_Task(Key_9_3);
    now_tick += 4000U;
    Keyboard_Task(INPUT_KEY_COUNT);
    CHECK(Service_Stage() == SERVICE_SECTION);
    key_down[Key_9_3] = 0U;
    Keyboard_Task(INPUT_KEY_COUNT);
    CHECK(Service_Stage() == SERVICE_SECTION); /* Отпускание не возвращает ещё на уровень. */
    return 0;
}


int test_keyboard_diagnostic_exit_on_long_release_without_intermediate_poll(void)
{
    InitControls();
    OpenServiceItem(13U, 9U);
    Keyboard_Task(INPUT_KEY_COUNT);
    key_down[Key_5_1] = 1U;
    Keyboard_Task(Key_5_1);
    now_tick += 5000U;
    key_down[Key_5_1] = 0U;
    Keyboard_Task(INPUT_KEY_COUNT);
    CHECK(Service_Stage() == SERVICE_SECTION && KeyTest_Event() == KEYTEST_LONG);
    Keyboard_Task(INPUT_KEY_COUNT);
    CHECK(Service_Stage() == SERVICE_SECTION);
    return 0;
}


int test_keyboard_diagnostic_alias_chord_and_entry_release_guard(void)
{
    InitControls();
    OpenServiceItem(15U, 10U);
    CHECK(Service_Stage() == SERVICE_KEY_TEST);
    key_down[Key_5_3] = 1U;
    Keyboard_Task(Key_5_3);
    CHECK(KeyTest_Ready() == 0U);
    key_down[Key_5_3] = 0U;
    Keyboard_Task(INPUT_KEY_COUNT);
    CHECK(KeyTest_Ready() != 0U);
    CHECK(KeyTest_LastKey() == INPUT_KEY_COUNT);
    now_tick = 0xFFFFFF00U;
    key_down[Key_6_3] = 1U;
    key_down[Key_10_2] = 1U;
    Keyboard_Task(Key_6_3);
    now_tick += 12000U;
    Keyboard_Task(INPUT_KEY_COUNT);
    CHECK(Service_Stage() == SERVICE_KEY_TEST && KeyTest_Event() == KEYTEST_LONG);
    CHECK(KeyTest_RawMask() == KeyTest_DownMask());
    key_down[Key_6_3] = 0U;
    key_down[Key_10_2] = 0U;
    Keyboard_Task(INPUT_KEY_COUNT);
    key_down[Key_5_1] = 1U;
    Keyboard_Task(Key_5_1);
    now_tick += 4000U;
    Keyboard_Task(INPUT_KEY_COUNT);
    CHECK(Service_Stage() == SERVICE_SECTION && Entry_Active() == 0U);
    return 0;
}
