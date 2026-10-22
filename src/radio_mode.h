#ifndef RADIO_MODE_H
#define RADIO_MODE_H

#include <stdint.h>

typedef enum
{
    RADIO_LSB,
    RADIO_USB,
    RADIO_CW,
    RADIO_CW_R,
    RADIO_RTTY,
    RADIO_RTTY_R,
    RADIO_AM,
    RADIO_MODE_COUNT
} radio_mode_t;

/* Параметры одного VFO или канала памяти; частота — Гц, band — индекс bands[]. */
typedef struct
{
    uint32_t hz;
    uint8_t mode;
    uint8_t ssb_usb;
    uint8_t filter;
    uint8_t band;
} radio_preset_t;

/* SSB -> CW -> RTTY -> AM -> SSB при удержании; полоса SSB запоминается. */
void RadioMode_Change(radio_preset_t *preset, uint8_t long_press);
const char *RadioMode_Name(uint8_t mode);
uint8_t RadioMode_IsSSB(uint8_t mode);

#endif
