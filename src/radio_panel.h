#ifndef RADIO_PANEL_H
#define RADIO_PANEL_H
#include <stdint.h>

typedef struct
{
    uint32_t tuner_tick;
    uint8_t att_mask;
    uint8_t preamp;
    uint8_t nb;
    uint8_t nb_level;
    uint8_t comp;
    uint8_t vox;
    uint8_t tuner;
    uint8_t meter;
} radio_panel_t;

/* Программные состояния; выводы реле/DSP/PTT/тюнера ещё не назначены. */
extern radio_panel_t panel;
void RadioPanel_Init(void);
void RadioPanel_Task(void);
void RadioPanel_CycleAtt(void);
void RadioPanel_TogglePreamp(void);
void RadioPanel_Tuner(uint8_t long_press);
uint8_t RadioPanel_AttDb(uint8_t mask);
#endif
