#include "controls_arm.c"

static void OpenServiceItem(uint8_t section, uint8_t item)
{
    (void)Settings_OpenService();
    Service_Encoder(section);
    Service_HandleKey(Key_9_2);
    Service_Encoder(item);
    Service_HandleKey(Key_5_3);
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
    OpenServiceItem(13U, 12U);
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
    OpenServiceItem(13U, 12U);
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
