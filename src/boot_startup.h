#ifndef BOOT_STARTUP_H
#define BOOT_STARTUP_H
/* Только основной контекст, после LCD/TIM2. Begin — до GeneratorMenu_Init. */
void BootStartup_Begin(void);
/* Завершает проверки и журнал до заставки. Энкодер листает, F-INP продолжает. */
void BootStartup_Run(void);
#endif
