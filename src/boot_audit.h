#ifndef BOOT_AUDIT_H
#define BOOT_AUDIT_H
#include <stdint.h>

/* Диагностика содержимого без записи; один блок 128 байт из стартового чтения. */
void BootAudit_Init(void);
void BootAudit_Block(uint16_t offset, const uint8_t *data);
uint8_t BootAudit_Used(void);
uint8_t BootAudit_Free(void);
uint8_t BootAudit_Bad(void);
uint16_t BootAudit_Count(void);
/* Строка журнала до 25 символов; буфер вызывающего — не менее 32 байт. */
void BootAudit_Line(uint16_t index, char *text);
/* 1 — замечание к резервной копии при исправной рабочей; 2 — ошибка. */
uint8_t BootAudit_Severity(uint16_t index);
#endif
