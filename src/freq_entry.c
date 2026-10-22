/* Ручной ввод частоты с цифровых логических клавиш. */
#include "freq_entry.h"
#include "app_state.h"
#include "radio_control.h"
#include "frequency_parse.h"

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

/* Применяет введённую частоту в пределах диапазона и закрывает ввод. */
void FreqEntry_Apply(void)
{
    uint32_t hz;

    if (Frequency_Parse(menu.freq_input, &hz) != 0U)
    {
        Radio_SetFrequency(hz);
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
