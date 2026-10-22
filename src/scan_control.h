#ifndef SCAN_CONTROL_H
#define SCAN_CONTROL_H
#include <stdint.h>
void Scan_Init(void);
void Scan_Toggle(void);
void Scan_Stop(void);
void Scan_Task(void);
uint8_t Scan_Active(void);
uint32_t Scan_Interval(void);
void Scan_AdjustInterval(int32_t steps);
#endif
