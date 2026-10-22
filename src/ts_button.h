#ifndef TS_BUTTON_H
#define TS_BUTTON_H

#include <stdint.h>

typedef struct
{
    uint32_t press_tick;
    uint32_t restore_step_hz;
    uint8_t active;
    uint8_t handled;
    uint8_t reserved[2];
} ts_button_t;

void TS_Button_Init(ts_button_t *button, uint32_t step_hz);
/* Tick unit is 500 us. A short press is applied on release;
 * a hold is applied once, strictly after 2000 ticks (one second).
 */
uint32_t TS_Button_Update(ts_button_t *button, uint8_t down, uint8_t pressed,
                          uint32_t now, uint32_t step_hz);

#endif
