#ifndef APP_STATE_H
#define APP_STATE_H

#include "app_config.h"
#include "input.h"

/* Внутреннее состояние приложения. ISR обновляют только драйвер input;
 * эта структура изменяется исключительно из основного цикла.
 */
typedef struct
{
    const char *name;
    uint32_t min_hz;
    uint32_t max_hz;
    uint32_t start_hz;
} band_t;

typedef struct
{
	uint32_t rf_hz;
    uint32_t adc_update_tick;

    uint16_t adc0_mv;
    uint16_t adc1_mv;

	char freq_input[FREQ_INPUT_MAX_LEN + 1U];

	uint8_t band_index;
    uint8_t step_index;
    uint8_t if_index;
    uint8_t menu_active;
    uint8_t freq_input_active;
    uint8_t selected_item;
    uint8_t need_redraw;
    uint8_t si5351_ok;
    input_key_t last_key;

    uint8_t freq_input_len;
    uint8_t freq_input_dot;
    uint8_t tx_active;
    uint8_t usb_active;
    int8_t gain_db;
    uint8_t reserved[2];
} generator_menu_t;

extern generator_menu_t menu;
extern const band_t bands[9];
extern const uint32_t steps_hz[6];
extern const uint32_t if_list_hz[3];

#endif
