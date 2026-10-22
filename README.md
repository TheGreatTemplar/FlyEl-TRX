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
| `firmware/` | MCU firmware, build and flashing instructions |
| `hardware/` | Schematics, PCBs, assembly and BOM |
| `documentation/` | Architecture, user and service documentation |
| `measurements/` | Bench measurements, calibration data and reports |
| `mechanical/` | Enclosure and mechanical designs, 3D models |
| [`tooling/`](tooling/) | Custom measurement fixtures, component-matching and assembly tooling |

## Firmware

The first firmware version will be imported during development. Refer to the files in `firmware/` for the actual target MCU, build environment and programming instructions once available.

## License

This repository uses the **MIT License**. See [LICENSE](LICENSE). Third-party material, if added, must retain its own applicable license terms.

---

## О проекте (русский)

**FLyEl-TRX — модульный любительский трансивер, аппаратная часть и программное обеспечение.**

Проект находится в разработке. Здесь будут размещаться прошивка, электрические схемы и печатные платы, результаты измерений, конструкторская документация и технологическая оснастка для изготовления и подбора компонентов.

Основные направления: синтезатор на Si5351, основная ПЧ 11,0592 МГц (альтернативная — 8,192 МГц), модульный ВЧ-тракт, сервисные калибровки, клавиатура и энкодеры передней панели. Реализация модулей и характеристики уточняются по мере испытаний.

**Лицензия:** MIT. См. [LICENSE](LICENSE). Для сторонних материалов могут действовать отдельные условия.
