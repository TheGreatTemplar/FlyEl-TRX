# FLyEl-TRX

**Modular amateur-radio transceiver — hardware and firmware project.**

**Status:** work in progress. This repository contains the firmware, circuit diagrams, PCB designs, measurements, and engineering documentation for the FLyEl transceiver.

## Project overview

The project starts with an HF receiver/transceiver architecture and is designed to evolve through independently tested modules.

- HF coverage under development, with possible future VHF/UHF expansion.
- Si5351-based frequency synthesis.
- Crystal-filter IF architecture: 11.0592 MHz primary option, 8.192 MHz alternative.
- Modular RF input filters, mixer, IF chain, audio, and transmitter/PA sections.
- Service menu with separate user settings and protected calibration data.
- Custom front-panel controls: keyboard matrix and rotary encoders.

The design is evolving; specifications and interfaces may change after prototype testing.

## Repository layout

| Directory | Purpose |
| --- | --- |
| `src/` | MCU source modules |
| [`font/`](font/) | Current and historical bitmap fonts; [font catalog](font/README.md) |
| `firmware/` | Build, flashing, architecture and pinout documentation |
| `scripts/`, `tests/` | Portable build and logic checks |
| [`hardware/`](hardware/) | Schematics, PCBs, assembly and BOM; [relay and I/O map](hardware/CONTROL_MAP.md) |
| `documentation/` | Architecture, user and service documentation |
| `measurements/` | Bench measurements, calibration data and reports |
| `mechanical/` | Enclosure and mechanical designs, 3D models |
| [`tooling/`](tooling/) | Custom measurement fixtures, component-matching and assembly tooling |

## Firmware

STM32F103C8T6 firmware is available in `src/`, with font assets in `font/`. Open the root `gmg12864_v01.uvprojx` in Keil MDK, or use `scripts/build.ps1`. See [build and multi-computer instructions](firmware/README.md), [module map](firmware/ARCHITECTURE.md), and [pinout](firmware/PINOUT.md).

The display, CD4017 keyboard with interrupt-driven wake, TS tuning-step control, Si5351 and non-destructive EEPROM CRC check are implemented. RX/TX switching, RIT/SPLIT, persistent settings and filter shift-register control are still under development.

## License

This repository uses the **MIT License**. See [LICENSE](LICENSE). Third-party material, if added, must retain its own applicable license terms.

---

## О проекте (русский)

**FLyEl-TRX — модульный любительский трансивер, аппаратная часть и программное обеспечение.**

Проект находится в разработке. Прошивка STM32F103C8T6 уже размещена в `src/`, проект Keil — в корне. [Сборка и работа с двух компьютеров](firmware/README.md), [карта модулей](firmware/ARCHITECTURE.md), [распиновка](firmware/PINOUT.md). Аппаратные материалы добавляются по мере готовности.

Основные направления: синтезатор на Si5351, основная ПЧ 11,0592 МГц (альтернативная — 8,192 МГц), модульный ВЧ-тракт, сервисные калибровки, клавиатура и энкодеры передней панели. Реализация модулей и характеристики уточняются по мере испытаний.

**Лицензия:** MIT. См. [LICENSE](LICENSE). Для сторонних материалов могут действовать отдельные условия.
