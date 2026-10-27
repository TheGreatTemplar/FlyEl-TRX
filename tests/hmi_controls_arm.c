#include "controls_arm.c"

int test_hmi_remaining_editors_cancel_save_and_ranges(void)
{
    uint16_t id;
    char value[24];
    InitControls();
    CHECK(Settings_OpenService() != 0U);
    Service_Encoder(13);
    Service_HandleKey(Key_9_2);
    CHECK(service_sections[13].count == 10U && SERVICE_ITEM_COUNT == 189U);
    for (id = 1406U; id <= 1412U; id++)
    {
        hmi_config_t before = *HMI_Get();
        uint32_t saves = mock_uart_saves;
        Service_Encoder((int32_t)(id - 1404U) - Service_Item());
        CHECK(Service_Current()->id == id);
        Service_HandleKey(Key_5_3);
        CHECK(Service_Stage() == SERVICE_HMI);
        Service_Encoder(2147483647);
        ServiceHMI_Value(value);
        CHECK(value[0] != '\0' && memcmp(&before, HMI_Get(), sizeof(before)) == 0);
        Service_HandleKey(Key_5_1);
        CHECK(Service_Stage() == SERVICE_SECTION && mock_uart_saves == saves);
        Service_HandleKey(Key_5_3);
        Service_Encoder(-2147483647 - 1);
        Service_HandleKey(Key_9_2);
        CHECK(mock_uart_saves == saves + 1U && HMI_Valid(HMI_Get()) != 0U);
        CHECK(ServiceHMI_Task() != 0U);
        CHECK(strcmp(ServiceHMI_Status(), "SAVED EEPROM") == 0);
        Service_HandleKey(Key_9_3);
        CHECK(Service_Stage() == SERVICE_SECTION);
    }
    return 0;
}


int test_hmi_long_press_applies_to_gestures_ts_and_func_timeout(void)
{
    hmi_config_t config;
    key_gesture_t key = {0};
    ts_button_t ts;
    uint32_t step = 100U;
    InitControls();
    config = *HMI_Get();
    config.long_ms = 500U;
    config.func_seconds = 5U;
    CHECK(HMI_Apply(&config) != 0U);
    CHECK(KeyGesture_Update(&key, 1U, 1U, 0U, 0U) == KEY_GESTURE_NONE);
    CHECK(KeyGesture_Update(&key, 1U, 0U, 0U, 1000U) == KEY_GESTURE_NONE);
    CHECK(KeyGesture_Update(&key, 1U, 0U, 0U, 1001U) == KEY_GESTURE_LONG);
    CHECK(KeyGesture_Update(&key, 0U, 0U, 0U, 1002U) == KEY_GESTURE_NONE);
    TS_Button_Init(&ts, step);
    step = TS_Button_Update(&ts, 1U, 1U, 0U, step);
    CHECK(TS_Button_Update(&ts, 1U, 0U, 1000U, step) == 100U);
    CHECK(TS_Button_Update(&ts, 0U, 0U, 1001U, step) == 10000U);
    CHECK(Entry_HandleKey(Key_5_3) != 0U);
    now_tick += 9999U;
    Entry_Task();
    CHECK(Entry_Active() != 0U);
    now_tick++;
    Entry_Task();
    CHECK(Entry_Active() == 0U);
    return 0;
}
