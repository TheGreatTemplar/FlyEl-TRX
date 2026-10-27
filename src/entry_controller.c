#include "hmi_config.h"
/* F-INP: одна операция, подтверждение/отмена и тайм-аут без блокирующего ожидания. */
#include "entry_controller.h"
#include "app_state.h"
#include "app_time.h"
#include "key_layout.h"
#include "freq_entry.h"
#include "frequency_parse.h"
#include "memory_control.h"
#include "radio_control.h"

static uint32_t activity_tick;
static uint8_t active;
static uint8_t channel_target;
static uint8_t error;

void Entry_Init(void)
{
    active = 0U;
    error = 0U;
    channel_target = 0U;
}


uint8_t Entry_Active(void)
{
    return active;
}


uint8_t Entry_ChannelTarget(void)
{
    return channel_target;
}


const char *Entry_Status(void)
{
    return (error != 0U) ? "INPUT ERR" : "FUNC";
}


void Entry_Cancel(void)
{
    active = 0U;
    error = 0U;
    FreqEntry_Cancel();
}


void Entry_Task(void)
{
    if (active != 0U && (uint32_t)(App_GetTick500us() - activity_tick) >= HMI_FuncTicks())
        Entry_Cancel();
}


static void Confirm(void)
{
    uint32_t hz;
    uint8_t channel;
    if (menu.freq_input_len == 0U)
    {
        Entry_Cancel();
        return;
    }
    if (channel_target != 0U)
    {
        if (Channel_Parse(menu.freq_input, &channel) == 0U)
        {
            error = 1U;
            return;
        }
        MemoryControl_Select(channel);
    }
    else
    {
        /* Сначала вся проверка: ошибочная частота не меняет ни RF, ни режим памяти. */
        if (FreqEntry_Parse(&hz) == 0U || Radio_FindBand(hz) >= Radio_BandsCount())
        {
            error = 1U;
            return;
        }
        MemoryControl_LeaveMode();
        Radio_SetFrequency(hz);
    }
    Entry_Cancel();
}


uint8_t Entry_HandleKey(input_key_t key)
{
    char ch;
    if (key == Key_5_3)
    {
        activity_tick = App_GetTick500us();
        if (active != 0U)
        {
            /* После ошибки FUNC начинает чистый ввод, сохраняя цель: частота/канал. */
            if (error != 0U)
            {
                error = 0U;
                FreqEntry_Start(channel_target);
            }
            else
                Confirm();
        }
        else
        {
            active = 1U;
            error = 0U;
            channel_target = MemoryControl_Selecting();
            FreqEntry_Start(channel_target);
        }
        menu.need_redraw = 1U;
        return 1U;
    }
    if (active == 0U)
        return 0U;
    ch = KeyLayout_Character(key);
    if (ch != '\0')
    {
        if (channel_target != 0U && ch == '.')
        {
            FreqEntry_Backspace();
            activity_tick = App_GetTick500us();
            error = 0U;
            return 1U;
        }
        if (FreqEntry_Append(ch) != 0U)
        {
            activity_tick = App_GetTick500us();
            error = 0U;
        }
        return 1U;
    }
    /* Нечисловая команда использует FUNC однократно; набранные цифры защищаем от неё. */
    if (menu.freq_input_len != 0U)
        return 1U;
    Entry_Cancel();
    return 0U;
}
