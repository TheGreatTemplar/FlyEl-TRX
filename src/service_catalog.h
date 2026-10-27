#ifndef SERVICE_CATALOG_H
#define SERVICE_CATALOG_H
#include <stdint.h>

enum
{
    SERVICE_SECTION_COUNT = 17U,
    SERVICE_ITEM_COUNT = 189U
};
enum
{
    SERVICE_READ_ONLY,
    SERVICE_ENUM,
    SERVICE_VALUE,
    SERVICE_TEXT,
    SERVICE_TABLE,
    SERVICE_TEST,
    SERVICE_ACTION,
    SERVICE_MONITOR,
    SERVICE_BOOL
};

typedef struct
{
    const char *label;
    uint16_t first;
    uint8_t count;
    uint8_t number;
} service_section_t;

typedef struct
{
    const char *label;
    uint16_t id;
    uint8_t type;
    uint8_t reserved;
} service_item_t;

/* Постоянные ID и названия из спецификации V1.0; каталог не выполняет команды. */
extern const service_section_t service_sections[SERVICE_SECTION_COUNT];
extern const service_item_t service_items[SERVICE_ITEM_COUNT];
const char *ServiceCatalog_TypeName(uint8_t type);
#endif
