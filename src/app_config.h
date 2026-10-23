#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* Общие константы текущего макета. Частоты заданы в Гц. */
#define VFO_CLK                     1U
#define REF_CLK                     0U
/* Set to 0U to restore the original Terminus main frequency. */
#define MAIN_FREQ_USE_SW2019        1U
#define ADC_VREF_MV                 3300U

#define FREQ_INPUT_MAX_LEN          7U
#define MENU_ITEMS                  5U

/* Каналы 1..99, три условных фильтра на каждый режим. */
#define MEMORY_CHANNEL_COUNT        99U
#define MEMORY_EEPROM_BASE          0x4000U
#define RADIO_FILTER_COUNT          3U
#define SCAN_INTERVAL_TICKS         400U
#define FUNC_TIMEOUT_TICKS          20000U
#define RADIO_MIN_HZ                100000UL
#define RADIO_MAX_HZ                30000000UL

#endif
