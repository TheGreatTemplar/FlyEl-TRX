#ifndef SERVICE_VALUES_H
#define SERVICE_VALUES_H
#include <stdint.h>
/* Две строки минимум по 24 байта. Только фактические данные; запись не выполняется. */
void ServiceValues_Read(uint16_t id, char *value, char *detail);
#endif
