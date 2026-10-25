#ifndef MEMORY_STORE_H
#define MEMORY_STORE_H
#include "radio_mode.h"

void MemoryStore_Init(void);
/* Один обмен за проход; возвращает запрос перерисовки. */
uint8_t MemoryStore_Task(void);
uint8_t MemoryStore_Ready(void);
uint8_t MemoryStore_Busy(void);
uint8_t MemoryStore_Get(uint8_t channel, radio_preset_t *preset);
/* preset=0 очищает канал. Кэш меняется только после записи и проверки чтением. */
uint8_t MemoryStore_Request(uint8_t channel, const radio_preset_t *preset);
/* Однократно выдаёт номер канала после успешной записи MW и проверки чтением.
 * Очистка и ошибка события не создают; вызывать из основного цикла. */
uint8_t MemoryStore_TakeSavedChannel(uint8_t *channel);
const char *MemoryStore_Status(void);
#endif
