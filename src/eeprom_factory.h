#ifndef EEPROM_FACTORY_H
#define EEPROM_FACTORY_H
#include <stdint.h>

enum
{
    EEPROM_FACTORY_IDLE,
    EEPROM_FACTORY_WRITE,
    EEPROM_FACTORY_WAIT,
    EEPROM_FACTORY_VERIFY,
    EEPROM_FACTORY_DONE,
    EEPROM_FACTORY_FAILED
};
/* Стирание всех 32 КБ в FF, постранично с ACK-poll и проверкой чтением.
 * Только основной цикл. После старта хранилища блокируются до перезапуска. */
uint8_t EEPROMFactory_Start(void);
uint8_t EEPROMFactory_Task(void);
uint8_t EEPROMFactory_Locked(void);
uint8_t EEPROMFactory_State(void);
uint16_t EEPROMFactory_Offset(void);
const char *EEPROMFactory_Status(void);
#endif
