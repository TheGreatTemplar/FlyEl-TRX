/* Навигация SERVICE отделена от редакторов, сохранения и аппаратных драйверов. */
#include "service_control.h"
#include "app_state.h"
#include "radio_control.h"
#include "scan_control.h"
#include "service_uart.h"
#include "service_keytest.h"

static uint8_t service_active;
static uint8_t service_stage;
static uint8_t service_section;
static uint8_t service_item;
static uint8_t service_band;

void Service_Init(void)
{
    service_active = 0U; service_stage = SERVICE_ROOT; service_section = 0U; service_item = 0U; service_band = 0U;
}

uint8_t Service_Open(void)
{
    if (menu.tx_active != 0U) return 0U;
    Service_Init(); service_active = 1U; service_band = menu.band_index;
    menu.menu_active = 1U; menu.need_redraw = 1U;
    Scan_Stop();
    return 1U;
}

uint8_t Service_Active(void) { return service_active; }
uint8_t Service_Stage(void) { return service_stage; }
uint8_t Service_Section(void) { return service_section; }
uint8_t Service_Item(void) { return service_item; }
uint8_t Service_Band(void) { return service_band; }

const service_item_t *Service_Current(void)
{
    return &service_items[service_sections[service_section].first + service_item];
}

uint8_t Service_BandScoped(void)
{
    uint16_t id = Service_Current()->id;
    return (uint8_t)(id == 306U || id == 402U || id == 806U || id == 1004U);
}

static uint8_t ServiceMove(uint8_t current, uint8_t count, int32_t steps)
{
    int32_t next = ((int32_t)current + steps % count) % count;
    if (next < 0) next += count;
    return (uint8_t)next;
}

void Service_Encoder(int32_t steps)
{
    if (service_active != 0U && service_stage == SERVICE_UART) { ServiceUart_Encoder(steps); return; }
    if (service_stage == SERVICE_KEY_TEST) return;
    if (service_active == 0U || steps == 0 || service_stage == SERVICE_VIEW) return;
    if (service_stage == SERVICE_ROOT)
    {
        service_section = ServiceMove(service_section, SERVICE_SECTION_COUNT, steps);
        service_item = 0U;
    }
    else if (service_stage == SERVICE_SECTION)
        service_item = ServiceMove(service_item, service_sections[service_section].count, steps);
    else if (service_stage == SERVICE_BAND)
        service_band = ServiceMove(service_band, Radio_BandsCount(), steps);
    menu.need_redraw = 1U;
}

void Service_HandleKey(input_key_t key)
{
    if (service_active == 0U) return;
    if (service_stage == SERVICE_UART)
    {
        if (ServiceUart_Key(key) != 0U) service_stage = SERVICE_SECTION;
        menu.need_redraw = 1U; return;
    }
    if (service_stage == SERVICE_KEY_TEST) return;
    if (key == Key_9_2 || key == Key_5_3)
    {
        if (service_stage == SERVICE_ROOT) service_stage = SERVICE_SECTION;
        else if (service_stage == SERVICE_SECTION)
        {
            uint16_t id = Service_Current()->id;
            if (id == 1504U) { ServiceUart_Open(); service_stage = SERVICE_UART; }
            else if (id == 1413U || id == 1611U) { KeyTest_Open(); service_stage = SERVICE_KEY_TEST; }
            else service_stage = (Service_BandScoped() != 0U) ? SERVICE_BAND : SERVICE_VIEW;
        }
        else if (service_stage == SERVICE_BAND) service_stage = SERVICE_VIEW;
        /* Параметры без согласованных пределов не редактируются и не выполняются. */
    }
    else if (key == Key_9_3 || key == Key_5_1)
    {
        if (service_stage == SERVICE_VIEW) service_stage = (Service_BandScoped() != 0U) ? SERVICE_BAND : SERVICE_SECTION;
        else if (service_stage == SERVICE_BAND) service_stage = SERVICE_SECTION;
        else if (service_stage == SERVICE_SECTION) service_stage = SERVICE_ROOT;
        else { service_active = 0U; menu.menu_active = 0U; }
    }
    else return;
    menu.need_redraw = 1U;
}

void Service_EndKeyTest(void)
{
    if (service_stage == SERVICE_KEY_TEST) service_stage = SERVICE_SECTION;
    menu.need_redraw = 1U;
}
