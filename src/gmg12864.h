#ifndef GMG12864_H
#define GMG12864_H

#include "stm32f10x.h"
#include <stdint.h>

/*
 * Пины управления взяты из архива:
 * PB10 = LCD_CS
 * PB11 = LCD_RST
 * PB12 = LCD_DC
 *
 * SPI1:
 * PA5 = SCK
 * PA7 = MOSI
 */
#define LCD_CS_GPIO                 GPIOB
#define LCD_RST_GPIO                GPIOB
#define LCD_DC_GPIO                 GPIOB

#define LCD_CS_PIN                  10U
#define LCD_RST_PIN                 11U
#define LCD_DC_PIN                  12U

#define LCD_WIDTH                   128U
#define LCD_HEIGHT                  64U

#define PX_OFF                      0U
#define PX_ON                       1U

#define CH_UNINVERT                 0U
#define CH_INVERT                   1U

void LCD_GPIO_Init(void);
void LCD_Init(void);
void LCD_ClearBuffer(void);
void LCD_Update(void);

void LCD_DrawPixel(uint8_t x, uint8_t y, uint8_t color);
void LCD_InvertBufferRange(uint16_t start, uint16_t end);

void LCD_DrawChar8(uint8_t x, uint8_t y, uint8_t symbol, uint8_t invert);
void LCD_DrawDigit10x15(uint8_t x, uint8_t y, uint8_t symbol, uint8_t invert);
void LCD_PrintDigits10x15(uint8_t x, uint8_t y, const char *text, uint8_t spacing, uint8_t invert);
uint8_t LCD_GetDigits10x15Width(const char *text, uint8_t spacing);

void LCD_DrawDigit7x11(uint8_t x, uint8_t y, uint8_t symbol, uint8_t invert);
/* Отдельный жирный шрифт номера канала, размер 8x9 пикселей, цифры 0..9. */
void LCD_DrawDigit8x9(uint8_t x, uint8_t y, uint8_t symbol, uint8_t invert);
void LCD_PrintDigits7x11(uint8_t x, uint8_t y, const char *text, uint8_t spacing, uint8_t invert);

void LCD_DrawDigit10x19(uint8_t x, uint8_t y, uint8_t symbol, uint8_t invert);
void LCD_PrintDigits10x19(uint8_t x, uint8_t y, const char *text, uint8_t spacing, uint8_t invert);
uint8_t LCD_GetDigits10x19Width(const char *text, uint8_t spacing);
void LCD_Print(uint8_t x, uint8_t y, const char *text, uint8_t spacing, uint8_t invert);
uint8_t LCD_GetTextWidth(const char *text, uint8_t spacing);
void LCD_DrawDigitSW2019(uint8_t x, uint8_t y, uint8_t symbol, uint8_t height, uint8_t invert);

void LCD_Print5x7(uint8_t x, uint8_t y, const char *text, uint8_t spacing, uint8_t invert);
uint8_t LCD_GetText5x7Width(const char *text, uint8_t spacing);

void LCD_DrawDigit4x6(uint8_t x, uint8_t y, uint8_t symbol, uint8_t invert);
void LCD_PrintDigits4x6(uint8_t x, uint8_t y, const char *text, uint8_t spacing, uint8_t invert);
uint8_t LCD_GetDigits4x6Width(const char *text, uint8_t spacing);

void LCD_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color);
void LCD_DrawRectangle(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color);
void LCD_FillRectangle(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color);
void LCD_DrawCircle(uint8_t x, uint8_t y, uint8_t radius, uint8_t color);
void LCD_FillCircle(int8_t x, int8_t y, int8_t radius, uint8_t color);

#endif
