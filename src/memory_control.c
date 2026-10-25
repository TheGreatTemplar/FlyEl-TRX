/* Рабочий канал, режим памяти и временный выбор CH отделены от хранения EEPROM. */
#include "memory_control.h"
#include "memory_store.h"
#include "radio_vfo.h"
#include "app_state.h"
#include "app_time.h"

/* 500 мкс на тик: шесть фаз по 200 мс дают три погасания номера. */
#define SAVE_BLINK_PHASE_TICKS 400U
#define SAVE_BLINK_PHASE_COUNT 6U

static radio_preset_t saved_vfo;
static uint8_t memory_mode;
static uint8_t selecting;
static uint8_t channel;
static uint8_t previously_ready;
static uint8_t previously_busy;
static uint32_t save_blink_tick;
static uint8_t save_blink_active;
static uint8_t save_blink_channel;
static uint8_t save_blink_phase;

static void Recall(void)
{
    radio_preset_t preset;
    if (memory_mode != 0U && MemoryStore_Get(channel, &preset) != 0U)
        RadioVfo_Apply(&preset);
    menu.need_redraw = 1U;
}

void MemoryControl_Init(void)
{
    memory_mode = 0U; selecting = 0U; channel = 0U;
    previously_ready = 0U; previously_busy = 0U;
    save_blink_active = 0U;
    MemoryStore_Init();
}

void MemoryControl_Task(void)
{
    uint8_t saved_channel;
    if (MemoryStore_Task() != 0U) menu.need_redraw = 1U;
    if (MemoryStore_TakeSavedChannel(&saved_channel) != 0U)
    {
        save_blink_tick = App_GetTick500us();
        save_blink_channel = saved_channel;
        save_blink_active = 1U; save_blink_phase = 0U;
        menu.need_redraw = 1U;
    }
    if (save_blink_active != 0U)
    {
        uint32_t phase = (uint32_t)(App_GetTick500us() - save_blink_tick) / SAVE_BLINK_PHASE_TICKS;
        if (phase >= SAVE_BLINK_PHASE_COUNT)
        { save_blink_active = 0U; menu.need_redraw = 1U; }
        else if (phase != save_blink_phase)
        { save_blink_phase = (uint8_t)phase; menu.need_redraw = 1U; }
    }
    if ((previously_ready == 0U && MemoryStore_Ready() != 0U) ||
        (previously_busy != 0U && MemoryStore_Busy() == 0U)) Recall();
    previously_ready = MemoryStore_Ready();
    previously_busy = MemoryStore_Busy();
}

void MemoryControl_LeaveMode(void)
{
    if (memory_mode != 0U)
    {
        memory_mode = 0U;
        RadioVfo_Apply(&saved_vfo);
    }
}

void MemoryControl_ToggleMode(void)
{
    if (memory_mode != 0U) MemoryControl_LeaveMode();
    else
    {
        saved_vfo = RadioVfo_Capture();
        memory_mode = 1U;
        Recall();
    }
    menu.need_redraw = 1U;
}

void MemoryControl_ToggleSelection(void)
{
    selecting ^= 1U;
    menu.need_redraw = 1U;
}

void MemoryControl_Select(uint8_t next)
{
    if (next >= MEMORY_CHANNEL_COUNT) return;
    channel = next;
    Recall();
}

void MemoryControl_Move(int32_t steps)
{
    int32_t next = ((int32_t)channel + steps % (int32_t)MEMORY_CHANNEL_COUNT) % (int32_t)MEMORY_CHANNEL_COUNT;
    if (next < 0) next += (int32_t)MEMORY_CHANNEL_COUNT;
    MemoryControl_Select((uint8_t)next);
}

void MemoryControl_Write(void)
{
    radio_preset_t preset = RadioVfo_Capture();
    (void)MemoryStore_Request(channel, &preset);
    menu.need_redraw = 1U;
}

void MemoryControl_Clear(void)
{
    if (selecting != 0U) (void)MemoryStore_Request(channel, 0);
    menu.need_redraw = 1U;
}

void MemoryControl_ToVfo(void)
{
    radio_preset_t preset;
    if (MemoryStore_Get(channel, &preset) != 0U)
    {
        MemoryControl_LeaveMode();
        RadioVfo_Apply(&preset);
    }
    menu.need_redraw = 1U;
}

uint8_t MemoryControl_Mode(void) { return memory_mode; }
uint8_t MemoryControl_Selecting(void) { return selecting; }
uint8_t MemoryControl_Channel(void) { return channel; }
uint8_t MemoryControl_Blank(void) { return (uint8_t)(MemoryStore_Get(channel, 0) == 0U); }

uint8_t MemoryControl_NumberVisible(void)
{
    return (uint8_t)(save_blink_active == 0U || channel != save_blink_channel ||
                     (save_blink_phase & 1U) != 0U);
}
