/* Рабочий канал, режим памяти и временный выбор CH отделены от хранения EEPROM. */
#include "memory_control.h"
#include "memory_store.h"
#include "radio_vfo.h"
#include "app_state.h"

static radio_preset_t saved_vfo;
static uint8_t memory_mode;
static uint8_t selecting;
static uint8_t channel;
static uint8_t previously_ready;
static uint8_t previously_busy;

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
    MemoryStore_Init();
}

void MemoryControl_Task(void)
{
    if (MemoryStore_Task() != 0U) menu.need_redraw = 1U;
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
