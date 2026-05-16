# Результат эксперимента: 011_device_blength_too_long_01

## 1. Цель кейса
Проверить устойчивость Linux USB-стека к аномалии `device_descriptor.bLength` (значение больше нормативных 18 байт) и определить, приводит ли это к ожидаемому отказу/частичному перечислению без признаков аварии драйвера или системы.

## 2. Входные артефакты
- Case JSON: `experiments/cases/011_device_blength_too_long.json`
- Wrapper logs:
  - `logs/runs/20260516_045009_011_device_blength_too_long_01.log`
  - `logs/runs/20260516_045510_011_device_blength_too_long_01.log`
- Quick analysis:
  - `logs/runs/20260516_045009_011_device_blength_too_long_01.quick_analysis.txt`
  - `logs/runs/20260516_045510_011_device_blength_too_long_01.quick_analysis.txt`
- Raw Linux results:
  - `results/raw/linux/011_device_blength_too_long_01_20260516_045038`
  - `results/raw/linux/011_device_blength_too_long_01_20260516_045539`

## 3. Наблюдаемый результат runtime enumeration
- В основном прогоне (`20260516_045510`) wrapper зафиксировал:
  - `Runtime USB enumeration status: partial` (`...045510_...log:98`).
- В соответствующем raw-снимке:
  - `runtime_enum_status=partial` (`...045539/runtime_status.txt`).

Итог по прогону: перечисление неполное.

## 4. Linux evidence (wrapper, quick analysis, raw)
- Wrapper (`...045510...log`):
  - после прошивки и ожидания runtime устройство не стабилизировалось в полноценный runtime-девайс, статус отмечен как `partial`.
- Quick analysis (`...045510...quick_analysis.txt`):
  - подтверждён `Runtime USB enumeration status: partial`;
  - в snapshot не видно целевого runtime USB-устройства в `lsusb` (только root hub и штатные HID устройства), что согласуется с неполным перечислением.
- Raw (`...045539`):
  - `runtime_status.txt`: `partial`;
  - `tty_devices.txt`: отсутствуют `ttyACM/ttyUSB`;
  - `journalctl_k_tail.txt`: циклы подключения RP2 Boot (`2e8a:0003`) и последующего runtime VID:PID `2e8a:4005` с быстрым `USB disconnect`, без признаков kernel panic/oops.

## 5. Классификация
- Основная: `PARTIAL_ENUM`.
- Сопутствующая: `KERNEL_WARNING` (по сообщениям уровня `FAT-fs ... unable to read boot sector ...` в хвосте kernel-журнала).

Что **не подтверждено** текущими артефактами:
- `DRIVER_CRASH` — нет явных признаков падения драйвера;
- `SYSTEM_CRASH` — нет признаков panic/oops/перезагрузки хоста.

## 6. Уровень уверенности
Средний.

Причина: совпадающие индикаторы `partial` в wrapper и raw-статусе дают устойчивый вывод по перечислению, но kernel tail включает исторические записи, а не строго изолированное окно только одного запуска.

## 7. Пробелы в доказательной базе
- `journalctl_k_tail*` содержит события из более широкого временного окна, не только текущей попытки.
- В wrapper fallback-сборе `dmesg` ограничен правами (`операция не позволена`), поэтому часть контекста ядра может отсутствовать.
- Нет отдельной временной корреляции `start/end` запуска в raw-папке для строгой фильтрации kernel-событий.

## 8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|partial|Done" logs/runs/*011_device_blength_too_long_01*.log`
2. `cat results/raw/linux/011_device_blength_too_long_01_20260516_045539/runtime_status.txt`
3. `rg -n "usb 1-4|USB disconnect|idVendor=2e8a|idProduct=4005|unable|error" results/raw/linux/011_device_blength_too_long_01_20260516_045539/journalctl_k_tail.txt`
4. `cat results/raw/linux/011_device_blength_too_long_01_20260516_045539/tty_devices.txt`
5. `bash -n tools/codex-analyze-log.sh`
