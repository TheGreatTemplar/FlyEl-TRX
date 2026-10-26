#ifndef SERVICE_KEYTEST_H
#define SERVICE_KEYTEST_H
#include "input.h"
enum
{
    KEYTEST_NONE,
    KEYTEST_DOWN,
    KEYTEST_SHORT,
    KEYTEST_LONG
};
void KeyTest_Open(void);
/* Основной цикл, без рабочих команд. 1 — выход по удержанию UP/NR более 2 с. */
uint8_t KeyTest_Poll(void);
uint8_t KeyTest_Ready(void);
input_key_t KeyTest_LastKey(void);
uint8_t KeyTest_Event(void);
uint32_t KeyTest_DurationMs(void);
uint32_t KeyTest_RawMask(void);
uint32_t KeyTest_DownMask(void);
#endif
