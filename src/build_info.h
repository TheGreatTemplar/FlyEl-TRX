#ifndef BUILD_INFO_H
#define BUILD_INFO_H
/* Генератор общий для Keil и scripts/build.ps1. Отсутствие метаданных — ошибка сборки. */
#include "../build/build_info_generated.h"
#ifndef FW_GIT_HASH
#error "Build metadata missing: run scripts/update-build-info.ps1 before compiling."
#endif
#endif
