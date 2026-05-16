# usb_case firmware targets: baseline и custom descriptor layer

## Назначение

В репозитории используются два отдельных firmware-target для безопасных USB descriptor/enumeration экспериментов на RP2040 (Waveshare RP2040 Zero):

- `usb_case_demo` — CDC baseline с `pico_stdio_usb` (команды `help/info/ping/bootloader/rfdiag`, heartbeat, RF bootloader recovery).
- `usb_case_custom_demo` — отдельный target для кастомных TinyUSB descriptor callbacks (включая inert HID/no-CDC сценарии), без `pico_stdio_usb`.

Цель target:

- дать чистую USB CDC baseline-прошивку без зависимостей от RF/control-plane runtime;
- сохранить существующий `portable_demo` неизменным и рабочим;
- обеспечить простую идентификацию активного USB-кейса через compile-time параметры.

## Разделение ответственности target-ов

`usb_case_demo` намеренно не определяет `tud_descriptor_*` callbacks, чтобы не конфликтовать с Pico SDK `pico_stdio_usb/stdio_usb_descriptors.c`.

Кастомные callbacks TinyUSB (`tud_descriptor_device_cb`, `tud_descriptor_configuration_cb`, `tud_descriptor_string_cb`, HID callbacks) собраны только в `usb_case_custom_demo`.

В `usb_case_custom_demo` сохраняется compile-time переключение descriptor persona по `USB_CASE_ID`:

- `000` -> persona `cdc_acm`;
- `001` -> persona `hid_generic_no_input` (инертный HID, без инжекции ввода).
- `002` -> persona `composite_cdc_hid_inert` (композитный CDC+HID, HID инертный, без инжекции ввода).

Для `001` публикуется inert HID-only interface (без инжекции ввода, без выполнения команд на хосте).
Для `002` публикуется безопасный composite descriptor: один configuration, три интерфейса (CDC Comm, CDC Data, HID), без host actions.

Дополнительно в `usb_case_custom_demo` реализован первый блок безопасных негативных кейсов device descriptor:

- `010_device_blength_too_short`: `bLength` намеренно меньше стандартной длины device descriptor.
- `011_device_blength_too_long`: `bLength` намеренно больше стандартной длины device descriptor.
- `012_device_unknown_class`: `bDeviceClass` установлен в необычное/неизвестное значение.
- `013_device_zero_vid_pid`: `idVendor=0x0000`, `idProduct=0x0000`.

Эти кейсы ограничены уровнем descriptor/enumeration и не добавляют host actions. Для malformed descriptor сценариев enumeration может завершиться до появления CDC/tty.

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
- `USB_CASE_BASE_PERSONA` (по умолчанию `"cdc_acm"`)
- `USB_CASE_MUTATION_SUMMARY` (по умолчанию `"none"`)

Дополнительно для слоя выбора persona:

- `USB_CASE_PERSONA_CDC_ACM`
- `USB_CASE_PERSONA_HID_BASELINE`
- `USB_CASE_PERSONA_COMPOSITE_CDC_HID`
- `USB_CASE_PERSONA_ID` (вычисляется из `USB_CASE_ID`: `001` -> HID, `002` -> composite CDC+HID, иначе CDC)

## Runtime диагностический вывод (CDC baseline)

Прошивка периодически выводит:

```text
usb_case_demo: alive
case_id=...
case_name=...
case_group=...
```

Это позволяет фиксировать активный кейс в логах хоста и в артефактах эксперимента.

Дополнительно один раз при старте выводятся:

```text
descriptor_persona_id=...
descriptor_persona_name=...
descriptor_switched=1
descriptor_active_transport=...
```

## Доступность CDC

- `usb_case_demo`: CDC всегда доступен через `pico_stdio_usb`.
- `usb_case_custom_demo`:
  - `000_baseline_cdc`: CDC доступен (через custom TinyUSB callbacks).
  - `001_baseline_hid_no_input`: CDC отсутствует, ожидается HID/hidraw enumeration.
  - `002_baseline_composite_cdc_hid`: CDC доступен, дополнительно присутствует инертный HID интерфейс.

## CDC-командный интерфейс (удалённое управление)

`usb_case_demo` поддерживает безопасный текстовый интерфейс команд через CDC serial.

Поддерживаемые команды:

- `help`
- `info`
- `ping`
- `bootloader`

Особенности:

- парсинг простой и безопасный (строковые команды, без выполнения команд хоста);
- интерфейс не эмулирует клавиатуру и не содержит BadUSB-полезных нагрузок;
- команды обрабатываются только в `usb_case_demo`.

Примеры:

```text
> help
available commands: help info ping bootloader
```

```text
> ping
pong
```

```text
> info
case_id=...
case_name=...
case_group=...
case_base_persona=...
mutation_summary=...
descriptor_persona_name=...
descriptor_switched=...
descriptor_active_transport=...
```

Команда переключения в BOOTSEL/UF2:

```text
> bootloader
usb_case_demo: entering USB bootloader
```

После вывода этой строки прошивка вызывает `reset_usb_boot(0, 0)` и переводит RP2040 в USB bootloader режим.

## Сборка

Базовый quick-start для `portable_demo` сохраняется без изменений.

Для сборки target-ов в уже сконфигурированном build-каталоге:

```bash
cmake --build build --target usb_case_demo
cmake --build build --target usb_case_custom_demo
```

Артефакт:

```text
build/usb_case_demo.uf2
build/usb_case_custom_demo.uf2
```

## Remote runner

`tools/run-remote-linux-case.sh` в текущем виде ориентирован на CDC-путь (`usb_case_demo`, serial smoke-test).

Для no-CDC сценариев `usb_case_custom_demo` требуется отдельный no-serial режим запуска/сбора артефактов (без шага serial smoke-test).

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
