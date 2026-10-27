#ifndef SERVICE_LCD_TEST_H
#define SERVICE_LCD_TEST_H
#include "input.h"

enum
{
    LCD_TEST_ON,
    LCD_TEST_OFF,
    LCD_TEST_CHECK,
    LCD_TEST_CHECK_INV,
    LCD_TEST_VERTICAL,
    LCD_TEST_VERTICAL_INV,
    LCD_TEST_HORIZONTAL,
    LCD_TEST_HORIZONTAL_INV,
    LCD_TEST_GEOMETRY,
    LCD_TEST_SWEEP_X,
    LCD_TEST_SWEEP_Y,
    LCD_TEST_FONT_MAIN,
    LCD_TEST_FONT_SMALL,
    LCD_TEST_FONT_OTHER,
    LCD_TEST_COUNT
};
/* Тест не меняет контраст, ориентацию и EEPROM; основной контекст, тик 500 мкс. */
void LCDTest_Open(void);
void LCDTest_Close(void);
uint8_t LCDTest_Task(void);
void LCDTest_Encoder(int32_t steps);
/* 1 — вернуться к пункту SERVICE. */
uint8_t LCDTest_Key(input_key_t key);
uint8_t LCDTest_Page(void);
uint8_t LCDTest_Auto(void);
uint8_t LCDTest_Position(void);
#endif
