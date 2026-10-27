#include "hmi_config.h"
/* Диагностика всех 27 клавиш, включая TS и сочетания. Прерывания не меняются. */
#include "service_keytest.h"
#include "app_time.h"
#include "app_state.h"

static uint32_t keytest_press_tick[27];
static uint8_t keytest_down[27];
static uint8_t keytest_long[27];
static uint32_t keytest_raw;
static uint32_t keytest_mask;
static uint32_t keytest_duration;
static uint32_t keytest_paint_tick;
static input_key_t keytest_last;
static uint8_t keytest_event;
static uint8_t keytest_ready;

void KeyTest_Open(void)
{
    uint8_t i;
    for (i = 0U; i < 27U; i++)
    {
        keytest_down[i] = 0U;
        keytest_long[i] = 0U;
    }
    keytest_last = INPUT_KEY_COUNT;
    keytest_event = KEYTEST_NONE;
    keytest_ready = 0U;
    keytest_raw = 0U;
    keytest_mask = 0U;
    keytest_duration = 0U;
    keytest_paint_tick = App_GetTick500us();
}


uint8_t KeyTest_Ready(void)
{
    return keytest_ready;
}


input_key_t KeyTest_LastKey(void)
{
    return keytest_last;
}


uint8_t KeyTest_Event(void)
{
    return keytest_event;
}


uint32_t KeyTest_DurationMs(void)
{
    return keytest_duration;
}


uint32_t KeyTest_RawMask(void)
{
    return keytest_raw;
}


uint32_t KeyTest_DownMask(void)
{
    return keytest_mask;
}


uint8_t KeyTest_Poll(void)
{
    uint8_t i, exit = 0U;
    uint32_t now = App_GetTick500us();
    uint32_t mask = 0U;
    uint32_t raw = Input_GetRawKeyMask();
    for (i = 0U; i < 27U; i++)
        if (Input_KeyIsDown((input_key_t)((uint8_t)Key_2_1 + i)) != 0U)
            mask |= 1UL << i;
    if (mask != keytest_mask || raw != keytest_raw)
        menu.need_redraw = 1U;
    keytest_raw = raw;
    keytest_mask = mask;
    if (keytest_ready == 0U)
    {
        /* Не считать отпускание кнопки входа результатом теста. */
        if (mask == 0U)
        {
            keytest_ready = 1U;
            menu.need_redraw = 1U;
        }
        return 0U;
    }
    for (i = 0U; i < 27U; i++)
    {
        input_key_t key = (input_key_t)((uint8_t)Key_2_1 + i);
        uint8_t down = (uint8_t)((mask >> i) & 1U);
        uint32_t elapsed = (uint32_t)(now - keytest_press_tick[i]);
        if (down != 0U && keytest_down[i] == 0U)
        {
            keytest_press_tick[i] = now;
            keytest_long[i] = 0U;
            keytest_last = key;
            keytest_event = KEYTEST_DOWN;
            keytest_duration = 0U;
            menu.need_redraw = 1U;
        }
        else if (keytest_down[i] != 0U)
        {
            if (elapsed > HMI_LongPressTicks() && keytest_long[i] == 0U)
            {
                keytest_long[i] = 1U;
                keytest_last = key;
                keytest_event = KEYTEST_LONG;
                menu.need_redraw = 1U;
            }
            if (keytest_last == key)
                keytest_duration = elapsed / 2U;
            if (down == 0U)
            {
                keytest_last = key;
                keytest_duration = elapsed / 2U;
                keytest_event = (keytest_long[i] != 0U) ? KEYTEST_LONG : KEYTEST_SHORT;
                menu.need_redraw = 1U;
            }
            /* Проверяем и на отпускании, если основной цикл пропустил границу 2 с. */
            if ((key == Key_9_3 || key == Key_5_1) && elapsed >= 4000U)
                exit = 1U;
        }
        keytest_down[i] = down;
    }
    if (mask != 0U && (uint32_t)(now - keytest_paint_tick) >= 40U)
    {
        keytest_paint_tick = now;
        menu.need_redraw = 1U;
    }
    return exit;
}
