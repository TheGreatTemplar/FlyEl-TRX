#ifndef MEMORY_CONTROL_H
#define MEMORY_CONTROL_H
#include <stdint.h>

void MemoryControl_Init(void);
void MemoryControl_Task(void);
void MemoryControl_ToggleMode(void);
void MemoryControl_LeaveMode(void);
void MemoryControl_ToggleSelection(void);
void MemoryControl_Select(uint8_t channel);
void MemoryControl_Move(int32_t steps);
void MemoryControl_Write(void);
void MemoryControl_Clear(void);
void MemoryControl_ToVfo(void);
uint8_t MemoryControl_Mode(void);
uint8_t MemoryControl_Selecting(void);
uint8_t MemoryControl_Channel(void);
uint8_t MemoryControl_Blank(void);
/* Видимость номера: три подмигивания по 200 мс после подтверждённой записи MW. */
uint8_t MemoryControl_NumberVisible(void);
#endif
