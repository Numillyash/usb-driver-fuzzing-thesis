# Удалённый Linux workflow для USB-кейсов

Документ описывает запуск USB-экспериментов на удалённом Linux-стенде через `tools/run-remote-linux-case.sh`.

## Назначение

Скрипт выполняет единый pipeline:

1. Генерация `usb_case_config.generated.h` из JSON-кейса.
2. Сборка прошивки в Docker.
3. Копирование выбранного UF2 на удалённый хост.
4. Перевод RP2040 в BOOTSEL через RF-команду.
5. Подготовка USB-устройства на удалённом хосте.
6. Прошивка UF2 на RP2040.
7. Снятие Linux USB snapshot (обязательно во всех режимах).

## CLI-параметры

```bash
./tools/run-remote-linux-case.sh [--uf2 <path>] [--no-serial-test] [--allow-runtime-enum-failure] [--runtime-wait-seconds <N>] <case-json> <result-name>
```

- `--uf2 <path>`: путь к локальному UF2 для прошивки.
  - По умолчанию: `build/usb_case_demo.uf2`.
- `--no-serial-test`: отключает удалённый запуск `~/vkr-usb/scripts/test_usb_case_serial_linux.py`.
  - По умолчанию serial smoke-test включён.
- `--allow-runtime-enum-failure`: разрешает негативный исход runtime-enumeration после прошивки.
  - В этом режиме отсутствие `/dev/ttyACM*` не считается ошибкой.
  - Serial smoke-test автоматически пропускается, если runtime-enumeration не `normal`.
  - Скрипт всё равно сохраняет Linux USB логи (`lsusb`, `lsusb -t`, `journalctl -k`, `dmesg`, `/dev/tty*`, `/sys/bus/usb/devices`).
- `--runtime-wait-seconds <N>`: ожидание runtime-enumeration после прошивки (целое число, по умолчанию `8`).

Скрипт печатает выбранный UF2 и состояние serial smoke-test перед копированием на удалённый хост.

## Поток 1: CDC baseline

Используется для baseline `usb_case_demo` (CDC доступен, `/dev/ttyACM0` ожидается).

Пример:

```bash
./tools/run-remote-linux-case.sh \
  experiments/cases/000_baseline_cdc.json \
  000_baseline_cdc_repeat_05
```

Поведение:

- прошивается `build/usb_case_demo.uf2`;
- выполняется удалённый serial smoke-test;
- затем сохраняется Linux USB snapshot.

## Поток 2: custom descriptor / no-CDC (например HID-only)

Используется для отдельного target (например `usb_case_custom_demo`), где CDC может отсутствовать.

Пример:

```bash
./tools/run-remote-linux-case.sh \
  --uf2 build/usb_case_custom_demo.uf2 \
  --no-serial-test \
  experiments/cases/001_baseline_hid_no_input.json \
  001_baseline_hid_no_input_01
```

Поведение:

- прошивается указанный через `--uf2` артефакт;
- serial smoke-test пропускается;
- Linux USB snapshot всё равно снимается.

## Почему нужен `--no-serial-test`

Для HID-only/no-CDC кейсов устройство может не создавать `/dev/ttyACM0`. В таком сценарии обязательный serial smoke-test даёт ложное падение pipeline, хотя USB enumeration/descriptor эксперимент выполнен корректно. Опция `--no-serial-test` отключает только CDC-проверку, сохраняя прошивку и сбор артефактов USB.

Для `usb_case_custom_demo` с persona `002_baseline_composite_cdc_hid` CDC присутствует, поэтому `--no-serial-test` обычно не требуется.

Для негативных device descriptor кейсов `010`/`011`/`012`/`013` обычно следует использовать:

```bash
./tools/run-remote-linux-case.sh \
  --uf2 build/usb_case_custom_demo.uf2 \
  --no-serial-test \
  --allow-runtime-enum-failure \
  --runtime-wait-seconds 10 \
  experiments/cases/<case>.json \
  <result-name>
```

Причина: при malformed descriptor кейсах устройство может не дойти до стадии, где создаётся `/dev/ttyACM0`, и это может быть ожидаемым экспериментальным результатом.

Для кейсов:
- `010_device_desc_blength_short.json`
- `011_device_desc_bnumconfig_zero.json`
- `012_device_desc_maxpkt_zero.json`
- `013_device_desc_ep0_mismatch.json`

нормальным наблюдением может быть один из статусов runtime USB enumeration:
- `normal`: устройство перечислилось штатно (обычно есть `/dev/ttyACM*`).
- `partial`: USB-активность видна в `lsusb`, но штатный CDC runtime не сформирован.
- `failed`: runtime-устройство не перечислилось в рабочем виде.

Важный момент для интерпретации результатов: отсутствие `ttyACM` и/или ошибка конфигурации в kernel log для malformed descriptor кейсов может быть ожидаемым outcome эксперимента, а не ошибкой пайплайна.
