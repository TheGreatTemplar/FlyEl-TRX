#ifndef SERVICE_CONTROL_H
#define SERVICE_CONTROL_H
#include <stdint.h>
#include "input.h"
#include "service_catalog.h"

enum
{
    SERVICE_ROOT,
    SERVICE_SECTION,
    SERVICE_BAND,
    SERVICE_VIEW,
    SERVICE_UART,
    SERVICE_KEY_TEST,
    SERVICE_FACTORY,
    SERVICE_CONTRAST
};
void Service_Init(void);
/* Открывает SERVICE только вне TX. Вызывается из основного цикла после сочетания. */
uint8_t Service_Open(void);
uint8_t Service_Active(void);
uint8_t Service_Stage(void);
uint8_t Service_Section(void);
uint8_t Service_Item(void);
uint8_t Service_Band(void);
uint8_t Service_BandScoped(void);
/* Энкодер прокручивает список; в редакторе UART меняет только черновик. */
void Service_Encoder(int32_t steps);
void Service_HandleKey(input_key_t key);
const service_item_t *Service_Current(void);
/* Возврат из изолированного теста клавиатуры к его пункту в списке. */
void Service_EndKeyTest(void);
#endif
