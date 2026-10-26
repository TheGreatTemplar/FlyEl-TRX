#ifndef SERVICE_FACTORY_H
#define SERVICE_FACTORY_H
#include "input.h"
void ServiceFactory_Open(void);
void ServiceFactory_Encoder(int32_t steps);
/* 1 — выйти до начала стирания. Завершение требует перезапуска, не выхода с устаревшим кэшем. */
uint8_t ServiceFactory_Key(input_key_t key);
uint8_t ServiceFactory_Confirmed(void);
const char *ServiceFactory_Status(void);
#endif
