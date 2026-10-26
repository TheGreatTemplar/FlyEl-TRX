#include "eeprom_factory.h"
#include "at24c256.h"
#include "memory_store.h"
#include "uart_store.h"
#include "eeprom_startup.h"
#include "app_time.h"
#include <string.h>

static uint8_t factory_state;
static uint8_t factory_address;
static uint16_t factory_offset;
static uint32_t factory_wait;
static uint32_t factory_poll;
static const char *factory_status = "NOT STARTED";

uint8_t EEPROMFactory_State(void)
{
    return factory_state;
}


uint16_t EEPROMFactory_Offset(void)
{
    return factory_offset;
}


const char *EEPROMFactory_Status(void)
{
    return factory_status;
}


uint8_t EEPROMFactory_Locked(void)
{
    return (uint8_t)(factory_state != EEPROM_FACTORY_IDLE);
}


uint8_t EEPROMFactory_Start(void)
{
    i2c2_soft_status_t status;
    if (EEPROMFactory_Locked() != 0U || EEPROM_TestBusy() != 0U || MemoryStore_Busy() != 0U || UARTStore_Busy() != 0U)
    {
        factory_status = "EEPROM BUSY";
        return 0U;
    }
    status = AT24C256_Find(&factory_address);
    if (status != I2C2_SOFT_OK)
    {
        factory_status = (status == I2C2_SOFT_NACK) ? "EEPROM NOT FOUND" : "EEPROM BUS ERROR";
        return 0U;
    }
    factory_offset = 0U;
    factory_state = EEPROM_FACTORY_WRITE;
    factory_status = "ERASING EEPROM";
    return 1U;
}


static uint8_t FactoryFail(const char *status)
{
    factory_status = status;
    factory_state = EEPROM_FACTORY_FAILED;
    return 1U;
}


/* Один обмен за проход. Время внутренней записи отсчитывается от STOP. */
uint8_t EEPROMFactory_Task(void)
{
    uint8_t data[AT24C256_PAGE_SIZE];
    uint8_t i;
    uint32_t now = App_GetTick500us();
    i2c2_soft_status_t status;
    switch (factory_state)
    {
        case EEPROM_FACTORY_WRITE:
            memset(data, 0xFF, sizeof(data));
            if (AT24C256_WritePage(factory_address, factory_offset, data, sizeof(data)) != I2C2_SOFT_OK)
                return FactoryFail("EEPROM WRITE FAIL");
            factory_wait = App_GetTick500us();
            factory_poll = factory_wait;
            factory_state = EEPROM_FACTORY_WAIT;
            break;
        case EEPROM_FACTORY_WAIT:
            if ((uint32_t)(now - factory_wait) < 10U || (uint32_t)(now - factory_poll) < 2U)
                break;
            factory_poll = now;
            status = I2C2_Soft_Probe(factory_address);
            if (status == I2C2_SOFT_OK)
                factory_state = EEPROM_FACTORY_VERIFY;
            else if (status != I2C2_SOFT_NACK)
                return FactoryFail("EEPROM BUS ERROR");
            else if ((uint32_t)(now - factory_wait) >= 40U)
                return FactoryFail("EEPROM ACK TIMEOUT");
            break;
        case EEPROM_FACTORY_VERIFY:
            if (AT24C256_Read(factory_address, factory_offset, data, sizeof(data)) != I2C2_SOFT_OK)
                return FactoryFail("EEPROM READ FAIL");
            for (i = 0U; i < sizeof(data); i++)
                if (data[i] != 0xFFU)
                    return FactoryFail("EEPROM VERIFY FAIL");
            factory_offset = (uint16_t)(factory_offset + sizeof(data));
            if (factory_offset == AT24C256_CAPACITY)
            {
                factory_state = EEPROM_FACTORY_DONE;
                factory_status = "FACTORY RESET OK";
            }
            else
                factory_state = EEPROM_FACTORY_WRITE;
            return 1U;
        default:
            break;
    }
    return 0U;
}
