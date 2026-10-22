#ifndef BOARD_PINS_H
#define BOARD_PINS_H

/* STM32F103C8T6: JTAG disabled; PA13/PA14 retain SWD.
 * LCD SPI1: PA5/PA7; CS=PB10, RST=PB11, DC=PB12.
 * Encoder: PB13/PB14, four transitions per detent.
 * Software I2C #1: PB6/PB7. I2C #2: PB8/PB9 (AT24C256C).
 * PA2 remains an analog reserve; PA12 and PA15 are free.
 */
#define KEY_CLK_GPIO GPIOA
#define KEY_CLK_PIN 11U
#define KEY_RESET_GPIO GPIOA
#define KEY_RESET_PIN 3U
#define KEY_INT_GPIO GPIOA
#define KEY_INT_PIN 4U
#define KEY_OUT_GPIO GPIOB
#define KEY_OUT_FIRST_PIN 3U

#define I2C2_GPIO GPIOB
#define I2C2_SCL_PIN 8U
#define I2C2_SDA_PIN 9U

#endif
