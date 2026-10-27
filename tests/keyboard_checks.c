#include "key_gesture.c"
#include "key_layout.c"
#include "radio_mode.c"
uint32_t HMI_LongPressTicks(void)
{
    return 2000U;
}


#define CHECK(expr)                                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(expr))                                                                                                   \
            return __LINE__;                                                                                           \
    } while (0)

const char *Input_KeyName(input_key_t key)
{
    (void)key;
    return "";
}


int test_key_short_and_long_exclusive(void)
{
    key_gesture_t key = {0};
    CHECK(KeyGesture_Update(&key, 1, 1, 0, 10) == KEY_GESTURE_NONE);
    CHECK(KeyGesture_Update(&key, 1, 0, 0, 2010) == KEY_GESTURE_NONE);
    CHECK(KeyGesture_Update(&key, 1, 0, 0, 2011) == KEY_GESTURE_LONG);
    CHECK(KeyGesture_Update(&key, 1, 0, 0, 4000) == KEY_GESTURE_NONE);
    CHECK(KeyGesture_Update(&key, 0, 0, 0, 4100) == KEY_GESTURE_NONE);
    CHECK(KeyGesture_Update(&key, 1, 1, 0, 4200) == KEY_GESTURE_NONE);
    CHECK(KeyGesture_Update(&key, 0, 0, 0, 6200) == KEY_GESTURE_SHORT);
    return 0;
}


int test_key_release_duration_and_tick_wrap(void)
{
    key_gesture_t key = {0};
    CHECK(KeyGesture_Update(&key, 1, 1, 0, 0xffffff00U) == KEY_GESTURE_NONE);
    CHECK(KeyGesture_Update(&key, 0, 0, 0, 0x6d1U) == KEY_GESTURE_LONG);
    CHECK(KeyGesture_Update(&key, 0, 0, 0, 0x800U) == KEY_GESTURE_NONE);
    return 0;
}


int test_key_arrow_repeat(void)
{
    key_gesture_t key = {0};
    CHECK(KeyGesture_Update(&key, 1, 1, 1, 0) == KEY_GESTURE_SHORT);
    CHECK(KeyGesture_Update(&key, 1, 0, 1, 1199) == KEY_GESTURE_NONE);
    CHECK(KeyGesture_Update(&key, 1, 0, 1, 1200) == KEY_GESTURE_REPEAT);
    CHECK(KeyGesture_Update(&key, 1, 0, 1, 1599) == KEY_GESTURE_NONE);
    CHECK(KeyGesture_Update(&key, 1, 0, 1, 1600) == KEY_GESTURE_REPEAT);
    CHECK(KeyGesture_Update(&key, 0, 0, 1, 2000) == KEY_GESTURE_NONE);
    return 0;
}


int test_key_numeric_layout(void)
{
    CHECK(KeyLayout_Character(Key_1_1) == '1');
    CHECK(KeyLayout_Character(Key_1_3) == '3');
    CHECK(KeyLayout_Character(Key_2_1) == '4');
    CHECK(KeyLayout_Character(Key_2_3) == '6');
    CHECK(KeyLayout_Character(Key_4_1) == '7');
    CHECK(KeyLayout_Character(Key_4_3) == '9');
    CHECK(KeyLayout_Character(Key_5_1) == '.');
    CHECK(KeyLayout_Character(Key_5_2) == '0');
    CHECK(KeyLayout_Character(Key_5_3) == '\0');
    CHECK(KeyLayout_Character(Key_10_3) == '\0');
    CHECK(KeyLayout_Character(Key_11_1) == '\0');
    return 0;
}


int test_mode_cycle_restores_ssb(void)
{
    radio_preset_t vfo = {28200000U, RADIO_LSB, 0, 0, 8};
    RadioMode_Change(&vfo, 0);
    CHECK(vfo.mode == RADIO_USB && vfo.ssb_usb == 1);
    RadioMode_Change(&vfo, 1);
    CHECK(vfo.mode == RADIO_CW);
    RadioMode_Change(&vfo, 0);
    CHECK(vfo.mode == RADIO_CW_R);
    RadioMode_Change(&vfo, 0);
    CHECK(vfo.mode == RADIO_CW);
    RadioMode_Change(&vfo, 1);
    CHECK(vfo.mode == RADIO_RTTY);
    RadioMode_Change(&vfo, 0);
    CHECK(vfo.mode == RADIO_RTTY_R);
    RadioMode_Change(&vfo, 1);
    CHECK(vfo.mode == RADIO_AM);
    RadioMode_Change(&vfo, 0);
    CHECK(vfo.mode == RADIO_AM);
    RadioMode_Change(&vfo, 1);
    CHECK(vfo.mode == RADIO_USB);
    CHECK(vfo.hz == 28200000U);
    return 0;
}


int test_mode_lsb_memory(void)
{
    radio_preset_t vfo = {7100000U, RADIO_USB, 1, 0, 2};
    RadioMode_Change(&vfo, 0);
    CHECK(vfo.mode == RADIO_LSB && vfo.ssb_usb == 0);
    RadioMode_Change(&vfo, 1);
    RadioMode_Change(&vfo, 1);
    RadioMode_Change(&vfo, 1);
    RadioMode_Change(&vfo, 1);
    CHECK(vfo.mode == RADIO_LSB);
    return 0;
}
