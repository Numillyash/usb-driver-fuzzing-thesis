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
./tools/run-remote-linux-case.sh [--uf2 <path>] [--no-serial-test] <case-json> <result-name>
```

- `--uf2 <path>`: путь к локальному UF2 для прошивки.
  - По умолчанию: `build/usb_case_demo.uf2`.
- `--no-serial-test`: отключает удалённый запуск `~/vkr-usb/scripts/test_usb_case_serial_linux.py`.
  - По умолчанию serial smoke-test включён.

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
