/* Сочетание, отпускание клавиш и всё дерево SERVICE проверяются реальным Thumb-кодом. */
#include "controls_arm.c"

static void PressServiceChord(input_key_t first, input_key_t second)
{
    key_down[first] = 1U;
    Keyboard_Task(first);
    now_tick += 100U;
    key_down[second] = 1U;
    Keyboard_Task(second);
}


static void ServiceChord(input_key_t first, input_key_t second)
{
    PressServiceChord(first, second);
    now_tick += 10000U;
    Keyboard_Task(INPUT_KEY_COUNT);
}


static void ReleaseServiceChord(input_key_t first, input_key_t second)
{
    key_down[first] = 0U;
    Keyboard_Task(INPUT_KEY_COUNT);
    key_down[second] = 0U;
    Keyboard_Task(INPUT_KEY_COUNT);
    Keyboard_Task(INPUT_KEY_COUNT);
}


int test_service_chord_requires_five_seconds_from_second_button(void)
{
    uint8_t order;
    for (order = 0U; order < 2U; order++)
    {
        input_key_t first = (order == 0U) ? Key_6_3 : Key_10_2;
        input_key_t second = (order == 0U) ? Key_10_2 : Key_6_3;
        InitControls();
        PressServiceChord(first, second);
        CHECK(Settings_Page() == 0U);
        now_tick += 9999U;
        Keyboard_Task(INPUT_KEY_COUNT);
        CHECK(Settings_Page() == 0U && RadioVfo_Filter() == 0U && panel.meter == 0U);
        now_tick++;
        Keyboard_Task(INPUT_KEY_COUNT);
        CHECK(Settings_Page() == 3U && Service_Stage() == SERVICE_ROOT);
        now_tick += 40000U;
        Keyboard_Task(INPUT_KEY_COUNT);
        CHECK(Service_Stage() == SERVICE_ROOT);
        ReleaseServiceChord(first, second);
    }
    return 0;
}


int test_service_short_chord_cancel_repress_and_timer_wrap(void)
{
    InitControls();
    PressServiceChord(Key_6_3, Key_10_2);
    now_tick += 9999U;
    key_down[Key_6_3] = 0U;
    Keyboard_Task(INPUT_KEY_COUNT);
    now_tick += 20000U;
    key_down[Key_6_3] = 1U;
    Keyboard_Task(Key_6_3);
    CHECK(Settings_Page() == 0U); /* Без отпускания обеих новый отсчёт не запускается. */
    ReleaseServiceChord(Key_6_3, Key_10_2);
    CHECK(panel.meter == 0U && RadioVfo_Filter() == 0U);
    now_tick = 0xFFFFFF00U;
    PressServiceChord(Key_10_2, Key_6_3);
    now_tick += 9999U;
    Keyboard_Task(INPUT_KEY_COUNT);
    CHECK(Settings_Page() == 0U);
    now_tick++;
    Keyboard_Task(INPUT_KEY_COUNT);
    CHECK(Settings_Page() == 3U);
    ReleaseServiceChord(Key_10_2, Key_6_3);
    return 0;
}


int test_service_tx_during_hold_cancels_entry_until_full_release(void)
{
    InitControls();
    PressServiceChord(Key_6_3, Key_10_2);
    now_tick += 4000U;
    menu.tx_active = 1U;
    Keyboard_Task(INPUT_KEY_COUNT);
    menu.tx_active = 0U;
    now_tick += 10000U;
    Keyboard_Task(INPUT_KEY_COUNT);
    CHECK(Settings_Page() == 0U);
    ReleaseServiceChord(Key_6_3, Key_10_2);
    ServiceChord(Key_6_3, Key_10_2);
    CHECK(Settings_Page() == 3U);
    ReleaseServiceChord(Key_6_3, Key_10_2);
    return 0;
}


int test_service_chord_both_orders_and_release_suppress_single_actions(void)
{
    uint8_t order;
    for (order = 0U; order < 2U; order++)
    {
        input_key_t first = (order == 0U) ? Key_6_3 : Key_10_2;
        input_key_t second = (order == 0U) ? Key_10_2 : Key_6_3;
        InitControls();
        ServiceChord(first, second);
        CHECK(Settings_Page() == 3U && Service_Stage() == SERVICE_ROOT);
        CHECK(RadioVfo_Filter() == 0U && panel.meter == 0U);
        now_tick += 6000U;
        Keyboard_Task(INPUT_KEY_COUNT);
        CHECK(Service_Stage() == SERVICE_ROOT);
        ReleaseServiceChord(first, second);
        CHECK(Service_Stage() == SERVICE_ROOT && Service_Section() == 0U);
        Tap(Key_9_2, 2500U); /* Удержание DN входит только на один уровень. */
        CHECK(Service_Stage() == SERVICE_SECTION && Service_Item() == 0U);
        Tap(Key_9_3, 2500U);
        CHECK(Service_Stage() == SERVICE_ROOT);
        Tap(Key_5_1, 100U);
        CHECK(Settings_Page() == 0U && menu.menu_active == 0U);
        CHECK(RadioVfo_Filter() == 0U && panel.meter == 0U && menu.rf_hz == 28200000U);
        Tap(Key_10_2, 100U);
        CHECK(RadioVfo_Filter() == 1U);
        Tap(Key_6_3, 100U);
        CHECK(panel.meter == 1U);
        Tap(Key_6_3, 2001U);
        CHECK(Settings_Page() == 1U);
    }
    return 0;
}


int test_service_rejects_chord_during_tx_and_preserves_func(void)
{
    InitControls();
    menu.tx_active = 1U;
    ServiceChord(Key_6_3, Key_10_2);
    ReleaseServiceChord(Key_10_2, Key_6_3);
    CHECK(Service_Active() == 0U && menu.menu_active == 0U);
    CHECK(panel.meter == 0U && RadioVfo_Filter() == 0U);
    menu.tx_active = 0U;
    Tap(Key_5_3, 100U);
    Tap(Key_1_2, 100U);
    CHECK(Entry_Active() != 0U && strcmp(menu.freq_input, "2") == 0);
    Tap(Key_5_3, 2001U);
    CHECK(Entry_Active() == 0U);
    ServiceChord(Key_6_3, Key_10_2);
    ReleaseServiceChord(Key_6_3, Key_10_2);
    Tap(Key_5_3, 100U);
    CHECK(Service_Stage() == SERVICE_SECTION && Entry_Active() == 0U);
    Tap(Key_5_3, 100U);
    CHECK(Service_Stage() == SERVICE_VIEW && Entry_Active() == 0U);
    Tap(Key_10_3, 100U);
    CHECK(steps_hz[menu.step_index] == 100U);
    Tap(Key_10_1, 2001U);
    CHECK(RadioVfo_Mode() == RADIO_LSB);
    return 0;
}


int test_service_catalog_and_navigation_visit_all_192_items_without_rf_changes(void)
{
    uint8_t section, item;
    uint16_t visited = 0U;
    InitControls();
    CHECK(Settings_OpenService() != 0U);
    for (section = 0U; section < SERVICE_SECTION_COUNT; section++)
    {
        Settings_Encoder((int32_t)section - Service_Section());
        CHECK(Service_Section() == section && service_sections[section].first == visited);
        Service_HandleKey(Key_9_2);
        CHECK(Service_Stage() == SERVICE_SECTION);
        for (item = 0U; item < service_sections[section].count; item++)
        {
            const service_item_t *current;
            Settings_Encoder((int32_t)item - Service_Item());
            current = Service_Current();
            CHECK(current->id == ((uint16_t)section + 1U) * 100U + item + 1U);
            CHECK(current->label[0] != '\0' && current->type <= SERVICE_BOOL);
            Service_HandleKey(Key_5_3);
            if (current->id == 1404U)
            {
                CHECK(Service_Stage() == SERVICE_LCD_TEST && LCDTest_Auto() != 0U);
                Service_HandleKey(Key_5_1);
                visited++;
                continue;
            }
            if (current->id == 1402U)
            {
                CHECK(Service_Stage() == SERVICE_CONTRAST);
                Service_HandleKey(Key_5_1);
                visited++;
                continue;
            }
            if (current->id == 1703U)
            {
                CHECK(Service_Stage() == SERVICE_FACTORY && ServiceFactory_Confirmed() == 0U);
                Service_HandleKey(Key_5_1);
                CHECK(mock_factory_starts == 0U);
                visited++;
                continue;
            }
            if (current->id == 1504U)
            {
                CHECK(Service_Stage() == SERVICE_UART);
                Service_HandleKey(Key_5_1);
                visited++;
                continue;
            }
            if (current->id == 1413U || current->id == 1611U)
            {
                CHECK(Service_Stage() == SERVICE_KEY_TEST);
                Service_EndKeyTest();
                visited++;
                continue;
            }
            if (Service_BandScoped() != 0U)
            {
                CHECK(Service_Stage() == SERVICE_BAND);
                Settings_Encoder(1234567);
                CHECK(Service_Band() < Radio_BandsCount());
                Service_HandleKey(Key_9_2);
            }
            CHECK(Service_Stage() == SERVICE_VIEW);
            Settings_Encoder(-2147483647 - 1);
            Service_HandleKey(Key_9_2);
            CHECK(Service_Current() == current && Service_Stage() == SERVICE_VIEW);
            Service_HandleKey(Key_5_1);
            if (Service_BandScoped() != 0U)
                Service_HandleKey(Key_9_3);
            CHECK(Service_Stage() == SERVICE_SECTION && Service_Item() == item);
            visited++;
        }
        Service_HandleKey(Key_9_3);
        CHECK(Service_Stage() == SERVICE_ROOT);
    }
    CHECK(visited == SERVICE_ITEM_COUNT);
    CHECK(menu.rf_hz == 28200000U && menu.if_index == 0U && menu.step_index == 3U && store_calls == 0U);
    Service_HandleKey(Key_9_3);
    CHECK(Service_Active() == 0U);
    return 0;
}


int test_service_encoder_wrap_and_new_entry_resets_navigation(void)
{
    InitControls();
    CHECK(Settings_OpenService() != 0U);
    Settings_Encoder(-1);
    CHECK(Service_Section() == 16U);
    Settings_Encoder(1);
    CHECK(Service_Section() == 0U);
    Settings_Encoder(2147483647);
    CHECK(Service_Section() < SERVICE_SECTION_COUNT);
    Settings_Encoder(-2147483647 - 1);
    CHECK(Service_Section() < SERVICE_SECTION_COUNT);
    Service_HandleKey(Key_9_2);
    Settings_Encoder(-1);
    CHECK(Service_Item() == service_sections[Service_Section()].count - 1U);
    Service_HandleKey(Key_9_3);
    Service_HandleKey(Key_9_3);
    CHECK(Settings_OpenService() != 0U && Service_Section() == 0U && Service_Stage() == SERVICE_ROOT);
    return 0;
}
