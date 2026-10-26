#ifndef MEMORY_RECORD_H
#define MEMORY_RECORD_H
#include "radio_mode.h"

#define MEMORY_RECORD_SIZE 64U

/* Байтовый формат FEL1 не зависит от выравнивания C-структур. 0 — tombstone. */
void MemoryRecord_Encode(uint8_t *data, uint8_t channel, uint32_t sequence, const radio_preset_t *preset);
uint8_t MemoryRecord_Decode(const uint8_t *data, uint8_t channel, uint32_t *sequence, radio_preset_t *preset,
                            uint8_t *used);
uint8_t MemoryRecord_Newer(uint32_t a, uint32_t b);
#endif
