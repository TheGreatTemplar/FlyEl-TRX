/* Ручной ввод частоты с цифровых логических клавиш. */
#include "freq_entry.h"
#include "app_state.h"
#include "radio_control.h"

/* Проверяет, относится ли логическая клавиша к ручному вводу частоты. */
uint8_t FreqEntry_IsCharacterKey(input_key_t key)
{
    if (Input_KeyToChar(key) != '\0')
    {
        return 1U;
    }

    return 0U;
}

/* Открывает ручной ввод; необязательный первый символ сразу добавляется в буфер. */
void FreqEntry_Start(char ch)
{
    menu.freq_input_active = 1U;
    menu.freq_input_len = 0U;
    menu.freq_input_dot = 0U;
    menu.freq_input[0] = '\0';

    if (ch != '\0')
    {
        if (ch == '.')
        {
            menu.freq_input_dot = 1U;
        }

        menu.freq_input[0] = ch;
        menu.freq_input[1] = '\0';
        menu.freq_input_len = 1U;
    }

    menu.need_redraw = 1U;
}

/* Добавляет символ, соблюдая размер буфера и запрет второй точки. */
void FreqEntry_Append(char ch)
{
    if (menu.freq_input_len >= FREQ_INPUT_MAX_LEN)
    {
        return;
    }

    if (ch == '.')
    {
        if (menu.freq_input_dot != 0U)
        {
            return;
        }

        menu.freq_input_dot = 1U;
    }

    menu.freq_input[menu.freq_input_len] = ch;
    menu.freq_input_len++;
    menu.freq_input[menu.freq_input_len] = '\0';

    menu.need_redraw = 1U;
}

/* Разбирает ввод: с точкой — МГц, без точки — кГц. */
static uint8_t FreqInputParse(uint32_t *hz_out)
{
    uint8_t i;
    uint8_t after_dot;
    uint32_t whole;
    uint32_t frac;
    uint32_t frac_scale;
    uint32_t hz;
    char ch;

    if ((hz_out == 0) || (menu.freq_input_len == 0U))
    {
        return 0U;
    }

    after_dot = 0U;
    whole = 0U;
    frac = 0U;
    frac_scale = 1U;

    for (i = 0U; i < menu.freq_input_len; i++)
    {
        ch = menu.freq_input[i];

        if (ch == '.')
        {
            after_dot = 1U;
        }
        else if ((ch >= '0') && (ch <= '9'))
        {
            if (after_dot == 0U)
            {
                whole = (whole * 10U) + (uint32_t)(ch - '0');
            }
            else
            {
                if (frac_scale < 1000000UL)
                {
                    frac = (frac * 10U) + (uint32_t)(ch - '0');
                    frac_scale *= 10U;
                }
            }
        }
        else
        {
            return 0U;
        }
    }

    if (menu.freq_input_dot != 0U)
    {
        /*
         * С точкой считаем ввод в MHz:
         * 3.650 -> 3650000 Hz.
         */
        hz = (whole * 1000000UL) + ((frac * 1000000UL) / frac_scale);
    }
    else
    {
        /*
         * Без точки считаем ввод в kHz:
         * 3650 -> 3650000 Hz.
         */
        hz = whole * 1000UL;
    }

    *hz_out = hz;

    return 1U;
}

/* Применяет введённую частоту в пределах диапазона и закрывает ввод. */
void FreqEntry_Apply(void)
{
    uint32_t hz;

    if (FreqInputParse(&hz) != 0U)
    {
        menu.rf_hz = hz;
        Radio_ClampRf();
        Radio_ApplySynth();
    }

    menu.freq_input_active = 0U;
    menu.need_redraw = 1U;
}

/* Закрывает ввод, оставляя прежнюю частоту. */
void FreqEntry_Cancel(void)
{
    menu.freq_input_active = 0U;
    menu.need_redraw = 1U;
}
