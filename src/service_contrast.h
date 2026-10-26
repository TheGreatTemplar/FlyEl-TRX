#ifndef SERVICE_CONTRAST_H
#define SERVICE_CONTRAST_H
#include "input.h"
void ServiceContrast_Open(void);
/* Шаг 1, насыщение 0..63. Предпросмотр без записи в EEPROM. */
void ServiceContrast_Encoder(int32_t steps);
uint8_t ServiceContrast_Key(input_key_t key);
uint8_t ServiceContrast_Task(void);
const char *ServiceContrast_Status(void);
#endif
