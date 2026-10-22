#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* Общие константы текущего макета. Частоты заданы в Гц. */
#define VFO_CLK                     1U
#define REF_CLK                     0U
/* Set to 0U to restore the original Terminus main frequency. */
#define MAIN_FREQ_USE_SW2019        1U
#define ADC_VREF_MV                 3300U
#define VFO_B_DEMO_HZ              21469780UL

#define FREQ_INPUT_MAX_LEN          11U
#define MENU_ITEMS                  5U

#endif
