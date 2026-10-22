#include "radio_mode.h"

uint8_t RadioMode_IsSSB(uint8_t mode)
{
    return (uint8_t)(mode == RADIO_LSB || mode == RADIO_USB);
}

void RadioMode_Change(radio_preset_t *preset, uint8_t long_press)
{
    uint8_t mode = preset->mode;
    if (RadioMode_IsSSB(mode) != 0U)
    {
        if (long_press != 0U) preset->mode = RADIO_CW;
        else
        {
            preset->mode = (mode == RADIO_USB) ? RADIO_LSB : RADIO_USB;
            preset->ssb_usb = (uint8_t)(preset->mode == RADIO_USB);
        }
    }
    else if (mode == RADIO_CW || mode == RADIO_CW_R)
        preset->mode = (long_press != 0U) ? RADIO_RTTY
                     : ((mode == RADIO_CW) ? RADIO_CW_R : RADIO_CW);
    else if (mode == RADIO_RTTY || mode == RADIO_RTTY_R)
        preset->mode = (long_press != 0U) ? RADIO_AM
                     : ((mode == RADIO_RTTY) ? RADIO_RTTY_R : RADIO_RTTY);
    else if (mode == RADIO_AM && long_press != 0U)
        preset->mode = (preset->ssb_usb != 0U) ? RADIO_USB : RADIO_LSB;
}

const char *RadioMode_Name(uint8_t mode)
{
    static const char *const names[] = { "LSB", "USB", "CW", "CW-R", "RTTY", "RTTY-R", "AM" };
    return (mode < RADIO_MODE_COUNT) ? names[mode] : "?";
}
