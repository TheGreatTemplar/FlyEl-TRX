#include "ts_button.h"

/* Сбрасывает автомат TS и запоминает исходный шаг для возврата из 10 кГц. */
void TS_Button_Init(ts_button_t *button, uint32_t step_hz)
{
    button->press_tick = 0U;
    button->restore_step_hz = (step_hz == 10000U) ? 100U : step_hz;
    button->active = 0U;
    button->handled = 0U;
    button->reserved[0] = 0U;
    button->reserved[1] = 0U;
}


/* Различает короткое нажатие и удержание >1 с; одно удержание выполняет одно действие. */
uint32_t TS_Button_Update(ts_button_t *button, uint8_t down, uint8_t pressed, uint32_t now, uint32_t step_hz)
{
    if (button->active == 0U)
    {
        if (pressed == 0U)
            return step_hz;
        button->active = 1U;
        button->handled = 0U;
        button->press_tick = now;
        if (step_hz == 10000U)
        {
            /* Any press exits 10 kHz. Consume the entire gesture so
             * holding this same press cannot immediately enter it again. */
            step_hz = button->restore_step_hz;
            button->handled = 1U;
        }
    }

    /* Check duration on release too: the main loop may not have run
     * between crossing the threshold and observing the released key. */
    if (button->handled == 0U && (uint32_t)(now - button->press_tick) > 2000U)
    {
        button->restore_step_hz = step_hz;
        step_hz = 10000U;
        button->handled = 1U;
    }
    if (down == 0U)
    {
        if (button->handled == 0U)
        {
            if (step_hz == 10U)
                step_hz = 100U;
            else if (step_hz == 100U)
                step_hz = 1000U;
            else
                step_hz = 10U;
        }
        button->active = 0U;
    }
    return step_hz;
}
