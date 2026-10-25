/* Редактор UART: черновик, отмена поля, явное APPLY с проверяемой записью EEPROM. */
#include "service_uart.h"
#include "uart1.h"
#include "app_state.h"
#include "text_utils.h"
#include "uart_store.h"

static uart_config_t uart_draft;
static uart_config_t uart_field_backup;
static uint8_t uart_item;
static uint8_t uart_editing;
static const char *uart_message;

void ServiceUart_Open(void)
{
    uart_draft = *UART1_GetConfig(); uart_item = 0U; uart_editing = 0U; uart_message = "EDIT THEN APPLY";
}
uint8_t ServiceUart_Item(void) { return uart_item; }
uint8_t ServiceUart_Editing(void) { return uart_editing; }
const char *ServiceUart_Status(void) { return (*uart_message != '\0') ? uart_message : UARTStore_Status(); }
const char *ServiceUart_Label(uint8_t item)
{
    static const char *const labels[] = {"BAUD", "DATA", "PARITY", "STOP", "APPLY", "BACK"};
    return (item < 6U) ? labels[item] : "";
}
void ServiceUart_Value(uint8_t item, char *text)
{
    text[0] = '\0';
    if (item == 0U) Text_U32ToDec(text, uart_draft.baud, 1U);
    else if (item == 1U) Text_U32ToDec(text, uart_draft.data_bits, 1U);
    else if (item == 2U) Text_Append(text, UARTConfig_ParityName(uart_draft.parity));
    else if (item == 3U) Text_Append(text, UARTConfig_StopName(uart_draft.stop));
}
static uint8_t UartMove(uint8_t value, uint8_t count, int32_t steps)
{
    int32_t next = ((int32_t)value + steps % count) % count;
    if (next < 0) next += count;
    return (uint8_t)next;
}
void ServiceUart_Encoder(int32_t steps)
{
    uint8_t index;
    if (steps == 0) return;
    if (uart_editing != 0U) uart_message = "CHANGED";
    if (uart_editing == 0U) uart_item = UartMove(uart_item, 6U, steps);
    else if (uart_item == 0U)
    {
        for (index = 0U; index < UART_BAUD_COUNT; index++)
            if (uart_baud_rates[index] == uart_draft.baud) break;
        uart_draft.baud = uart_baud_rates[UartMove(index, UART_BAUD_COUNT, steps)];
    }
    else if (uart_item == 1U) uart_draft.data_bits = (uint8_t)(8U + UartMove((uint8_t)(uart_draft.data_bits - 8U), 2U, steps));
    else if (uart_item == 2U) uart_draft.parity = UartMove(uart_draft.parity, 3U, steps);
    else if (uart_item == 3U) uart_draft.stop = UartMove(uart_draft.stop, 4U, steps);
    menu.need_redraw = 1U;
}
uint8_t ServiceUart_Key(input_key_t key)
{
    uint8_t enter = (uint8_t)(key == Key_9_2 || key == Key_5_3);
    uint8_t back = (uint8_t)(key == Key_9_3 || key == Key_5_1);
    if (enter == 0U && back == 0U) return 0U;
    if (uart_editing != 0U)
    {
        if (back != 0U) uart_draft = uart_field_backup;
        uart_editing = 0U;
    }
    else if (back != 0U || uart_item == 5U) return 1U;
    else if (uart_item < 4U) { uart_field_backup = uart_draft; uart_editing = 1U; }
    else if (UARTConfig_Valid(&uart_draft) == 0U) uart_message = "9 DATA: NO PARITY";
    else { (void)UARTStore_Request(&uart_draft); uart_message = ""; }
    menu.need_redraw = 1U;
    return 0U;
}
