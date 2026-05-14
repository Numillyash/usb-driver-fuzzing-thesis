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

## Генерация compile-time конфигурации из JSON-кейса

Для `usb_case_demo` можно автоматически сгенерировать заголовок `usb_case_config.generated.h` из файла кейса `experiments/cases/*.json`.

Скрипт: `tools/gen_usb_case_config.py` (только Python standard library).

### Что генерируется

В generated-header задаются макросы:

- `USB_CASE_ID`
- `USB_CASE_NAME`
- `USB_CASE_GROUP`
- `USB_CASE_BASE_PERSONA`
- `USB_CASE_MUTATION_SUMMARY`

Если generated-header отсутствует, `firmware/include/usb_case_config.h` автоматически использует безопасные значения по умолчанию, и сборка `usb_case_demo` остаётся рабочей.

### Примеры использования

Список доступных кейсов:

```bash
python3 tools/gen_usb_case_config.py --list
```

Генерация в путь по умолчанию (`firmware/include/usb_case_config.generated.h`):

```bash
python3 tools/gen_usb_case_config.py experiments/cases/020_config_wtotallength_too_small.json
```

Генерация в произвольный путь:

```bash
python3 tools/gen_usb_case_config.py \
  experiments/cases/000_baseline_cdc.json \
  --output /tmp/usb_case_config.generated.h
```
