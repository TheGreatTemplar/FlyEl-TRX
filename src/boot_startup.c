#include "boot_startup.h"
#include "boot_audit.h"
#include "ui_boot.h"
#include "eeprom_startup.h"
#include "memory_store.h"
#include "uart_store.h"
#include "input.h"
#include "app_time.h"
#include "app_state.h"
#include "text_utils.h"
#include <string.h>

static void FlushInput(void)
{
    input_key_t key;
    (void)Input_GetEncoderSteps();
    for (key = Key_2_1; key < INPUT_KEY_COUNT; key++)
        while (Input_KeyPressed(key) != 0U)
        {
        }
}


void BootStartup_Begin(void)
{
    BootAudit_Init();
    EEPROM_TestObserve(BootAudit_Block);
    BootUI_Clear();
    BootUI_Line(0U, "FLYEL BOOT", 0U);
    BootUI_Line(1U, "SI5351 INIT", 0U);
    BootUI_Spinner(1U, App_GetTick500us());
    BootUI_Update();
}


static void DrawSummary(uint8_t read_ok, uint32_t elapsed)
{
    char line[32];
    char number[12];
    BootUI_Clear();
    BootUI_Line(0U, "FLYEL BOOT", 0U);
    BootUI_Line(1U, menu.si5351_ok ? "SI5351 I2C OK" : "SI5351 I2C FAIL", (uint8_t)(menu.si5351_ok == 0U));
    strcpy(line, read_ok ? "EEPROM OK " : "EEPROM FAIL ");
    Text_U32ToDec(number, elapsed / 2000U, 1U);
    Text_Append(line, number);
    Text_Append(line, ".");
    Text_U32ToDec(number, (elapsed % 2000U) / 20U, 2U);
    Text_Append(line, number);
    Text_Append(line, " S");
    BootUI_Line(2U, line, (uint8_t)(read_ok == 0U));
    BootUI_Line(3U, EEPROM_TestStatus(), (uint8_t)(read_ok == 0U));
    strcpy(line, "USED ");
    Text_U32ToDec(number, BootAudit_Used(), 2U);
    Text_Append(line, number);
    Text_Append(line, " FREE ");
    Text_U32ToDec(number, BootAudit_Free(), 2U);
    Text_Append(line, number);
    BootUI_Line(4U, EEPROM_TestProgress() == 0U ? "CHANNELS NOT READ" : line, 0U);
    strcpy(line, "BAD ");
    Text_U32ToDec(number, BootAudit_Bad(), 2U);
    Text_Append(line, number);
    BootUI_Line(5U, line, (uint8_t)(BootAudit_Bad() != 0U));
    BootUI_Line(6U, "UART RESTORE", 0U);
    BootUI_Update();
}


/* Общие ошибки добавляются к компактному журналу EEPROM без лимита на число строк. */
static void ShowJournal(uint8_t read_ok, uint8_t uart_ok, uint8_t channels_ok)
{
    uint16_t records = BootAudit_Count();
    const char *extra[3];
    uint8_t extras = 0U;
    uint16_t top = 0U;
    uint16_t total;
    uint8_t armed = 0U;
    if (menu.si5351_ok == 0U)
        extra[extras++] = "SI5351 I2C FAIL";
    if (read_ok == 0U && records == 0U)
        extra[extras++] = "EEPROM NOT FOUND";
    if (uart_ok == 0U)
        extra[extras++] = UARTStore_Status();
    /* Ошибки каналов уже представлены по адресам; общий статус нужен при сбое повторной загрузки. */
    total = (uint16_t)(records + extras + (channels_ok == 0U ? 1U : 0U));
    if (total == 0U)
    {
        BootUI_Line(0U, "FLYEL BOOT READY", 0U);
        BootUI_Update();
        App_DelayMs(600U);
        return;
    }
    FlushInput();
    for (;;)
    {
        char line[32];
        char number[12];
        uint8_t row;
        int32_t movement;
        BootUI_Clear();
        strcpy(line, "BOOT LOG ");
        Text_U32ToDec(number, (uint32_t)top + 1U, 1U);
        Text_Append(line, number);
        Text_Append(line, "/");
        Text_U32ToDec(number, total, 1U);
        Text_Append(line, number);
        BootUI_Line(0U, line, 0U);
        for (row = 0U; row < 5U && top + row < total; row++)
        {
            uint16_t index = (uint16_t)(top + row);
            if (index < records)
                BootAudit_Line(index, line);
            else if (index < records + extras)
                strcpy(line, extra[index - records]);
            else
                strcpy(line, "CHANNEL LOAD FAIL");
            BootUI_Line((uint8_t)(row + 1U), line, 1U);
        }
        BootUI_Line(6U, "ENC:LOG F-INP:GO", 0U);
        BootUI_Update();
        if (Input_KeyIsDown(Key_5_3) == 0U)
            armed = 1U;
        if (Input_KeyPressed(Key_5_3) != 0U && armed != 0U)
            break;
        movement = Input_GetEncoderSteps();
        if (movement != 0)
        {
            int64_t next = (int64_t)top + movement;
            if (next < 0)
                next = 0;
            if (next >= total)
                next = (int64_t)total - 1;
            top = (uint16_t)next;
        }
        App_DelayMs(20U);
    }
}


void BootStartup_Run(void)
{
    uint32_t start = App_GetTick500us();
    uint32_t redraw = start - 200U;
    uint32_t elapsed;
    uint8_t read_ok;
    uint16_t pass;
    BootUI_Line(1U, menu.si5351_ok ? "SI5351 I2C OK" : "SI5351 I2C FAIL", (uint8_t)(menu.si5351_ok == 0U));
    while (EEPROM_TestBusy() != 0U)
    {
        (void)EEPROM_TestTask();
        if ((uint32_t)(App_GetTick500us() - redraw) >= 200U)
        {
            char line[32] = "EEPROM ";
            char number[12];
            Text_U32ToDec(number, (uint32_t)EEPROM_TestProgress() * 100U / 32768U, 1U);
            Text_Append(line, number);
            Text_Append(line, "%");
            BootUI_Line(2U, line, 0U);
            BootUI_Spinner(2U, App_GetTick500us());
            BootUI_Update();
            redraw = App_GetTick500us();
        }
    }
    elapsed = App_GetTick500us() - start;
    read_ok = (uint8_t)(EEPROM_TestStatus()[0] == 'C');
    DrawSummary(read_ok, elapsed);
    /* Автоматы загрузки требуют не более 201 прохода (99 каналов по две копии).
     * Ограничение не даёт зависнуть при ошибке; записей в EEPROM здесь нет. */
    for (pass = 0U; pass < 210U; pass++)
    {
        (void)UARTStore_Task();
        (void)MemoryStore_Task();
        if (UARTStore_Ready() != 0U && MemoryStore_Ready() != 0U)
            break;
        if (pass % 8U == 0U)
        {
            BootUI_Spinner(6U, App_GetTick500us());
            BootUI_Update();
        }
    }
    BootUI_Line(6U,
                UARTStore_Ready()
                    ? (strcmp(UARTStore_Status(), "LOADED EEPROM") == 0 ? "UART EEPROM OK" : "UART DEFAULT")
                    : "UART LOAD FAIL",
                (uint8_t)(UARTStore_Ready() == 0U));
    BootUI_Update();
    ShowJournal(read_ok, UARTStore_Ready(), MemoryStore_Ready());
    EEPROM_TestObserve(0);
    EEPROM_TestHideResult();
    FlushInput();
}
