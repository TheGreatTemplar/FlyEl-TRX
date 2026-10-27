#ifndef FW_VERSION_H
#define FW_VERSION_H
#include "input.h"
/* Только чтение. Буфер строки не менее 24 байт, на экран до 20 символов. */
void FWInfo_Open(void);
void FWInfo_Encoder(int32_t steps);
uint8_t FWInfo_Key(input_key_t key);
uint8_t FWInfo_Page(void);
uint8_t FWInfo_Pages(void);
const char *FWInfo_Title(void);
void FWInfo_Line(uint8_t row, char *text);
#endif
