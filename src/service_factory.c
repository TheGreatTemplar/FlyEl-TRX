#include "service_factory.h"
#include "eeprom_factory.h"
#include "app_state.h"
#include "app_time.h"

static uint8_t factory_yes;
static const char *factory_message;

void ServiceFactory_Open(void)
{
    factory_yes = 0U;
    factory_message = "ERASE ALL 32KB?";
}


void ServiceFactory_Encoder(int32_t steps)
{
    if (EEPROMFactory_Locked() != 0U || steps == 0)
        return;
    factory_yes = (uint8_t)(steps > 0);
    menu.need_redraw = 1U;
}


uint8_t ServiceFactory_Confirmed(void)
{
    return factory_yes;
}


const char *ServiceFactory_Status(void)
{
    return factory_message;
}


uint8_t ServiceFactory_Key(input_key_t key)
{
    uint8_t state = EEPROMFactory_State();
    if (state == EEPROM_FACTORY_DONE || state == EEPROM_FACTORY_FAILED)
    {
        if (key == Key_5_3)
            App_Reboot();
        return 0U;
    }
    if (EEPROMFactory_Locked() != 0U)
        return 0U;
    if (key == Key_9_3 || key == Key_5_1)
        return 1U;
    if (key != Key_5_3 && key != Key_9_2)
        return 0U;
    if (factory_yes == 0U)
        return 1U;
    if (menu.tx_active != 0U)
    {
        factory_message = "STOP TX FIRST";
        return 0U;
    }
    if (EEPROMFactory_Start() == 0U)
        factory_message = EEPROMFactory_Status();
    return 0U;
}
