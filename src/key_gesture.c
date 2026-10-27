#include "hmi_config.h"
#include "key_gesture.h"

/* База времени 500 мкс: начало повтора через 600 мс, период 200 мс. */
#define KEY_REPEAT_DELAY_TICKS 1200U
#define KEY_REPEAT_PERIOD_TICKS 400U

key_gesture_event_t KeyGesture_Update(key_gesture_t *state, uint8_t down, uint8_t pressed, uint8_t repeat, uint32_t now)
{
    key_gesture_event_t event = KEY_GESTURE_NONE;
    if (state->active == 0U)
    {
        if (pressed == 0U)
            return event;
        state->active = 1U;
        state->handled = 0U;
        state->press_tick = now;
        state->repeat_tick = now;
        if (repeat != 0U)
        {
            state->handled = 1U;
            return KEY_GESTURE_SHORT;
        }
    }

    /* Проверяем время и на отпускании: основной цикл мог пропустить порог. */
    if (repeat == 0U && state->handled == 0U && (uint32_t)(now - state->press_tick) > HMI_LongPressTicks())
    {
        state->handled = 1U;
        event = KEY_GESTURE_LONG;
    }
    if (repeat != 0U && down != 0U && (uint32_t)(now - state->press_tick) >= KEY_REPEAT_DELAY_TICKS &&
        (uint32_t)(now - state->repeat_tick) >= KEY_REPEAT_PERIOD_TICKS)
    {
        state->repeat_tick = now;
        event = KEY_GESTURE_REPEAT;
    }
    if (down == 0U)
    {
        if (state->handled == 0U)
            event = KEY_GESTURE_SHORT;
        state->active = 0U;
    }
    return event;
}
