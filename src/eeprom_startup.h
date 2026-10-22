#ifndef EEPROM_STARTUP_H
#define EEPROM_STARTUP_H
#include <stdint.h>

uint8_t EEPROM_TestBusy(void);

/* Начать тест; вызывать один раз после инициализации I2C2 и TIM2. */
void EEPROM_TestInit(void);
/* Прочитать один блок; возвращает 1 при изменении текста диагностики. */
uint8_t EEPROM_TestTask(void);
/* MEM..., CRC:XXXX или MEM ERR; через три секунды после результата — пустая строка. */
const char *EEPROM_TestStatus(void);
#endif
