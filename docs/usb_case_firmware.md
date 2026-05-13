# usb_case_demo: безопасный baseline для USB descriptor-экспериментов

## Назначение

`usb_case_demo` — отдельный firmware-target для безопасных экспериментов уровня USB descriptor/enumeration на RP2040 (Waveshare RP2040 Zero).

Цель target:

- дать чистую USB CDC baseline-прошивку без зависимостей от RF/control-plane runtime;
- сохранить существующий `portable_demo` неизменным и рабочим;
- обеспечить простую идентификацию активного USB-кейса через compile-time параметры.

## Границы безопасности

`usb_case_demo` предназначен только для безопасных USB-экспериментов:

- без BadUSB payloads;
- без эмуляции ввода команд с клавиатуры;
- без автоматического выполнения команд на хосте.

## Compile-time идентичность кейса

Файл: `firmware/include/usb_case_config.h`

Доступные макросы:

- `USB_CASE_ID` (по умолчанию `0`)
- `USB_CASE_NAME` (по умолчанию `"baseline_cdc"`)
- `USB_CASE_GROUP` (по умолчанию `"baseline"`)

## Runtime диагностический вывод (CDC)

Прошивка периодически выводит:

```text
usb_case_demo: alive
case_id=...
case_name=...
case_group=...
```

Это позволяет фиксировать активный кейс в логах хоста и в артефактах эксперимента.

## Сборка

Базовый quick-start для `portable_demo` сохраняется без изменений.

Для сборки `usb_case_demo` в уже сконфигурированном build-каталоге:

```bash
cmake --build build --target usb_case_demo
```

Артефакт:

```text
build/usb_case_demo.uf2
```
