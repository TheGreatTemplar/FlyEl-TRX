/* SCN: циклический проход диапазона или занятых каналов; детектор сигнала пока не задан. */
#include "scan_control.h"
#include "app_state.h"
#include "app_time.h"
#include "radio_control.h"
#include "memory_control.h"
#include "memory_store.h"

static uint32_t scan_tick;
static uint32_t interval;
static uint8_t scanning;

void Scan_Init(void) { scanning = 0U; interval = SCAN_INTERVAL_TICKS; }
uint8_t Scan_Active(void) { return scanning; }
uint32_t Scan_Interval(void) { return interval; }
void Scan_Stop(void) { scanning = 0U; menu.need_redraw = 1U; }

void Scan_AdjustInterval(int32_t steps)
{
    int64_t value = (int64_t)interval + (int64_t)steps * 100;
    if (value < 100) value = 100;
    if (value > 4000) value = 4000;
    interval = (uint32_t)value;
}

void Scan_Toggle(void)
{
    if (scanning != 0U) Scan_Stop();
    else
    {
        if (MemoryControl_Mode() != 0U && MemoryStore_Ready() == 0U) return;
        scanning = 1U;
        scan_tick = App_GetTick500us();
        menu.need_redraw = 1U;
    }
}

void Scan_Task(void)
{
    uint8_t i;
    if (scanning == 0U || (uint32_t)(App_GetTick500us() - scan_tick) < interval) return;
    scan_tick = App_GetTick500us();
    if (MemoryControl_Mode() != 0U)
    {
        uint8_t channel = MemoryControl_Channel();
        for (i = 0U; i < MEMORY_CHANNEL_COUNT; i++)
        {
            channel = (uint8_t)((channel + 1U) % MEMORY_CHANNEL_COUNT);
            if (MemoryStore_Get(channel, 0) != 0U)
            { MemoryControl_Select(channel); return; }
        }
        Scan_Stop();
    }
    else
    {
        uint32_t hz = menu.rf_hz;
        uint32_t step = steps_hz[menu.step_index];
        const band_t *band = &bands[menu.band_index];
        if (hz < band->min_hz || hz > band->max_hz || step > band->max_hz - hz)
            hz = band->min_hz;
        else hz += step;
        Radio_SetFrequency(hz);
    }
}
