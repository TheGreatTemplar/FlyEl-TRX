/* Независимая от каналов запись UART; операции согласованы с memory_store.c. */
#include "uart_store.h"
#include "uart1.h"
#include "at24c256.h"
#include "memory_store.h"
#include "memory_record.h"
#include "eeprom_startup.h"
#include "app_time.h"
#include <string.h>

enum { UART_WAIT_CRC, UART_FIND, UART_LOAD_A, UART_LOAD_B, UART_IDLE,
       UART_WRITE, UART_WAIT_WRITE, UART_VERIFY, UART_ERROR };
static uint8_t uart_store_state;
static uint8_t uart_store_address;
static uint8_t uart_store_slot;
static uint8_t uart_store_valid;
static uint8_t uart_store_ready;
static uint8_t uart_pending[64];
static uint32_t uart_generation;
static uint32_t uart_wait_tick;
static uint32_t uart_poll_tick;
static uart_config_t uart_previous;
static const char *uart_store_message;

static void UartPut32(uint8_t *data, uint32_t value)
{
    uint8_t i;
    for (i = 0U; i < 4U; i++) data[i] = (uint8_t)(value >> (8U * i));
}
static uint32_t UartGet32(const uint8_t *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8U) |
           ((uint32_t)data[2] << 16U) | ((uint32_t)data[3] << 24U);
}
static uint8_t UartDecode(const uint8_t *data, uart_config_t *config, uint32_t *generation)
{
    uint16_t crc = AT24C256_Crc16Update(0xFFFFU, data, 62U);
    if (data[0] != 'F' || data[1] != 'U' || data[2] != '0' || data[3] != '1' ||
        data[62] != (uint8_t)crc || data[63] != (uint8_t)(crc >> 8U)) return 0U;
    config->baud = UartGet32(&data[8]); config->data_bits = data[12];
    config->parity = data[13]; config->stop = data[14]; config->reserved = 0U;
    *generation = UartGet32(&data[4]);
    return UARTConfig_Valid(config);
}

void UARTStore_Init(void)
{
    uart_store_state = UART_WAIT_CRC; uart_generation = 0U;
    uart_store_valid = 0U; uart_store_ready = 0U; uart_store_slot = 0U;
    uart_store_message = "EEPROM WAIT";
}
uint8_t UARTStore_Busy(void)
{
    return (uint8_t)(uart_store_state == UART_WRITE || uart_store_state == UART_WAIT_WRITE || uart_store_state == UART_VERIFY);
}
uint8_t UARTStore_Ready(void) { return uart_store_ready; }
const char *UARTStore_Status(void) { return uart_store_message; }

static uint8_t UartStoreFail(void)
{
    if (UARTStore_Busy() != 0U) (void)UART1_ApplyConfig(&uart_previous);
    uart_store_state = (uart_store_ready != 0U) ? UART_IDLE : UART_ERROR;
    uart_store_message = "EEPROM ERROR";
    return 1U;
}

uint8_t UARTStore_Task(void)
{
    uint8_t data[64];
    uart_config_t config;
    uint32_t generation;
    uint32_t now = App_GetTick500us();
    i2c2_soft_status_t status;
    if (EEPROM_TestBusy() != 0U || MemoryStore_Busy() != 0U) return 0U;
    switch (uart_store_state)
    {
        case UART_WAIT_CRC: uart_store_state = UART_FIND; break;
        case UART_FIND:
            if (AT24C256_Find(&uart_store_address) != I2C2_SOFT_OK) return UartStoreFail();
            uart_store_state = UART_LOAD_A; break;
        case UART_LOAD_A: case UART_LOAD_B:
        {
            uint8_t slot = (uint8_t)(uart_store_state == UART_LOAD_B);
            if (AT24C256_Read(uart_store_address, (uint16_t)((uint16_t)slot * 64U), data, sizeof(data)) != I2C2_SOFT_OK)
                return UartStoreFail();
            if (UartDecode(data, &config, &generation) != 0U &&
                (uart_store_valid == 0U || MemoryRecord_Newer(generation, uart_generation) != 0U))
            {
                if (UART1_ApplyConfig(&config) == 0U) return UartStoreFail();
                uart_generation = generation; uart_store_slot = slot; uart_store_valid = 1U;
            }
            if (slot == 0U) uart_store_state = UART_LOAD_B;
            else
            {
                uart_store_ready = 1U; uart_store_state = UART_IDLE;
                uart_store_message = (uart_store_valid != 0U) ? "LOADED EEPROM" : "DEFAULT 115200 8N1";
                return 1U;
            }
            break;
        }
        case UART_WRITE:
            if (AT24C256_WritePage(uart_store_address, (uint16_t)((uart_store_slot ^ 1U) * 64U),
                                   uart_pending, sizeof(uart_pending)) != I2C2_SOFT_OK) return UartStoreFail();
            uart_wait_tick = now; uart_poll_tick = now; uart_store_state = UART_WAIT_WRITE; break;
        case UART_WAIT_WRITE:
            if ((uint32_t)(now - uart_wait_tick) >= 40U) return UartStoreFail();
            if ((uint32_t)(now - uart_wait_tick) < 10U || (uint32_t)(now - uart_poll_tick) < 2U) break;
            uart_poll_tick = now; status = I2C2_Soft_Probe(uart_store_address);
            if (status == I2C2_SOFT_OK) uart_store_state = UART_VERIFY;
            else if (status != I2C2_SOFT_NACK) return UartStoreFail();
            break;
        case UART_VERIFY:
            if (AT24C256_Read(uart_store_address, (uint16_t)((uart_store_slot ^ 1U) * 64U), data, sizeof(data)) != I2C2_SOFT_OK ||
                memcmp(data, uart_pending, sizeof(data)) != 0 || UartDecode(data, &config, &generation) == 0U)
                return UartStoreFail();
            uart_generation = generation; uart_store_slot ^= 1U; uart_store_valid = 1U;
            uart_store_state = UART_IDLE; uart_store_message = "SAVED EEPROM"; return 1U;
        default: break;
    }
    return 0U;
}

uint8_t UARTStore_Request(const uart_config_t *config)
{
    uint16_t crc;
    if (UARTConfig_Valid(config) == 0U) { uart_store_message = "INVALID UART"; return 0U; }
    if (uart_store_ready == 0U) { uart_store_message = "EEPROM NOT READY"; return 0U; }
    if (uart_store_state != UART_IDLE || MemoryStore_Busy() != 0U || EEPROM_TestBusy() != 0U)
    { uart_store_message = "EEPROM BUSY"; return 0U; }
    uart_previous = *UART1_GetConfig();
    if (UART1_ApplyConfig(config) == 0U) { uart_store_message = "UART BUSY"; return 0U; }
    memset(uart_pending, 0xFF, sizeof(uart_pending));
    uart_pending[0] = 'F'; uart_pending[1] = 'U'; uart_pending[2] = '0'; uart_pending[3] = '1';
    UartPut32(&uart_pending[4], uart_generation + 1U); UartPut32(&uart_pending[8], config->baud);
    uart_pending[12] = config->data_bits; uart_pending[13] = config->parity; uart_pending[14] = config->stop;
    crc = AT24C256_Crc16Update(0xFFFFU, uart_pending, 62U);
    uart_pending[62] = (uint8_t)crc; uart_pending[63] = (uint8_t)(crc >> 8U);
    uart_store_state = UART_WRITE; uart_store_message = "SAVING EEPROM";
    return 1U;
}
