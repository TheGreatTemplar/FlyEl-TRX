#ifndef EEPROM_STARTUP_H
#define EEPROM_STARTUP_H
#include <stdint.h>

uint8_t EEPROM_TestBusy(void);
/* Наблюдатель получает каждый блок 128 байт либо NULL при ошибке чтения.
 * Вызывается из основного цикла; не должен обращаться к EEPROM. */
void EEPROM_TestObserve(void (*observer)(uint16_t offset, const uint8_t *data));
uint16_t EEPROM_TestProgress(void);
void EEPROM_TestHideResult(void);

/* Начать тест; вызывать один раз после инициализации I2C2 и TIM2. */
void EEPROM_TestInit(void);
/* Прочитать один блок; возвращает 1 при изменении текста диагностики. */
uint8_t EEPROM_TestTask(void);
/* MEM..., CRC:XXXX или MEM ERR; через три секунды после результата — пустая строка. */
const char *EEPROM_TestStatus(void);
#endif
