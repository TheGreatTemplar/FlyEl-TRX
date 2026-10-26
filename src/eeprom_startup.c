/* Стартовая проверка чтения 32 КБ EEPROM и CRC-16; содержимое не изменяется. */
#include "eeprom_startup.h"
#include "at24c256.h"
#include "app_time.h"
#include "text_utils.h"

static char eeprom_status[12];
static uint8_t eeprom_check_running;
static uint8_t eeprom_address;
static uint8_t eeprom_result_visible;
static uint16_t eeprom_offset;
static uint16_t eeprom_crc;
static uint32_t eeprom_result_tick;


static uint8_t redraw_requested;

static void EepromTest_Finish(uint8_t ok)
{
    static const char hex[] = "0123456789ABCDEF";
    uint8_t i;
    if (ok != 0U)
    {
        eeprom_status[0] = 'C';
        eeprom_status[1] = 'R';
        eeprom_status[2] = 'C';
        eeprom_status[3] = ':';
        for (i = 0U; i < 4U; i++)
            eeprom_status[4U + i] = hex[(eeprom_crc >> (12U - 4U * i)) & 0x0FU];
        eeprom_status[8] = '\0';
    }
    else
    {
        eeprom_status[0] = '\0';
        Text_Append(eeprom_status, "MEM ERR");
    }
    eeprom_check_running = 0U;
    eeprom_result_visible = 1U;
    eeprom_result_tick = App_GetTick500us();
    redraw_requested = 1U;
}


void EEPROM_TestInit(void)
{
    eeprom_status[0] = '\0';
    Text_Append(eeprom_status, "MEM...");
    eeprom_address = 0U;
    eeprom_offset = 0U;
    eeprom_crc = 0xFFFFU;
    eeprom_check_running = 1U;
    eeprom_result_visible = 0U;
}


static void RunTest(void)
{
    uint8_t block[128];
    if (eeprom_check_running != 0U)
    {
        if (eeprom_address == 0U && AT24C256_Find(&eeprom_address) != I2C2_SOFT_OK)
        {
            EepromTest_Finish(0U);
            return;
        }
        if (AT24C256_Read(eeprom_address, eeprom_offset, block, sizeof(block)) != I2C2_SOFT_OK)
        {
            EepromTest_Finish(0U);
            return;
        }
        eeprom_crc = AT24C256_Crc16Update(eeprom_crc, block, sizeof(block));
        eeprom_offset = (uint16_t)(eeprom_offset + sizeof(block));
        if (eeprom_offset == AT24C256_CAPACITY)
            EepromTest_Finish(1U);
    }
    else if (eeprom_result_visible != 0U && (uint32_t)(App_GetTick500us() - eeprom_result_tick) >= 6000U)
    {
        eeprom_result_visible = 0U;
        eeprom_status[0] = '\0';
        redraw_requested = 1U;
    }
}


/* Сообщает, изменилось ли поле диагностики с предыдущего прохода. */
uint8_t EEPROM_TestTask(void)
{
    uint8_t changed;
    RunTest();
    changed = redraw_requested;
    redraw_requested = 0U;
    return changed;
}


const char *EEPROM_TestStatus(void)
{
    return eeprom_status;
}


/* Загрузка/запись каналов начинается только после неизменяемого стартового CRC. */
uint8_t EEPROM_TestBusy(void)
{
    return eeprom_check_running;
}
