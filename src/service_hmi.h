#ifndef SERVICE_HMI_H
#define SERVICE_HMI_H
#include "input.h"
/* Черновик одного пункта 14.06..14.12. Действует после проверенной записи. */
void ServiceHMI_Open(uint16_t id);
void ServiceHMI_Encoder(int32_t steps);
uint8_t ServiceHMI_Key(input_key_t key);
uint8_t ServiceHMI_Task(void);
const char *ServiceHMI_Status(void);
void ServiceHMI_Value(char *text);
const char *ServiceHMI_Range(void);
#endif
