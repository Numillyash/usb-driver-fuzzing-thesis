# Сводка Linux-кейсов descriptor/enumeration (completed runs)

## 1. Назначение Linux-only тестирования
Цель Linux-части — воспроизводимо проверить устойчивость USB-стека Linux к контролируемым аномалиям дескрипторов на этапе enumeration, не выходя за безопасные рамки descriptor-level экспериментов. Практический критерий: зафиксировать, приводит ли кейс к штатному перечислению (`OK`), частичному перечислению (`PARTIAL_ENUM`) или более тяжёлым эффектам.

## 2. Краткая методика
- Использованы только завершённые Linux-прогоны по 12 кейсам: `010, 011, 012, 013, 020, 021, 022, 030, 031, 032, 040, 041`.
- Источники: `docs/results/*.md`, `logs/runs/*.quick_analysis.txt`, `results/raw/linux/*/runtime_status.txt`, `lsusb*.txt`, `journalctl*.txt`, матрица из `docs/experiment_scope_and_case_matrix.md`.
- Базовый принцип интерпретации: итоговый статус берётся из согласованных артефактов (`wrapper`/`quick_analysis`/`runtime_status`) с проверкой snapshot (`lsusb`, `tty`, `journalctl`).
- Ограничение: без `usbmon`/pcap нельзя строго доказать фактические байты control-transfer в каждом запуске.

## 3. Таксономия классификации результатов
Используется проектная шкала из `docs/experiment_scope_and_case_matrix.md`:
`OK`, `EXPECTED_REJECT`, `PARTIAL_ENUM`, `DRIVER_BIND_ERROR`, `USERSPACE_FAILURE`, `KERNEL_WARNING`, `DRIVER_CRASH`, `SYSTEM_CRASH`.

В этой сводке по completed Linux runs фактически наблюдались только `OK` и `PARTIAL_ENUM`.

## 4. Группированные результаты

### Device descriptor cases
- `010_device_blength_too_short_01` -> `PARTIAL_ENUM`.
- `011_device_blength_too_long_01` -> `PARTIAL_ENUM`.
- `012_device_unknown_class_01` -> `OK`.
- `013_device_zero_vid_pid_01` -> `PARTIAL_ENUM`.

### Configuration descriptor cases
- `020_config_wtotallength_too_small_01` -> `OK`.
- `021_config_wtotallength_too_large_01` -> `OK`.
- `022_config_bnuminterfaces_mismatch_01` -> `OK`.

### Interface descriptor cases
- `030_interface_class_invalid_01` -> `OK`.

### Endpoint descriptor cases
- `031_endpoint_wmaxpacketsize_zero_01` -> `OK`.
- `032_endpoint_direction_mismatch_01` -> `OK`.

### String descriptor cases
- `040_string_invalid_length_01` -> `OK`.
- `041_string_missing_index_01` -> `OK`.

## 5. Основные наблюдения
- Кейсы с `PARTIAL_ENUM`: `010`, `011`, `013`.
- Кейсы с `OK`: `012`, `020`, `021`, `022`, `030`, `031`, `032`, `040`, `041`.
- Подтверждённых `DRIVER_CRASH` не обнаружено.
- Подтверждённых `SYSTEM_CRASH` не обнаружено.
- Слабое подтверждение факта именно целевой мутации в финальных `lsusb` snapshot (видны нормализованные/обычные дескрипторы либо нет явного отражения аномалии):
  - `022_config_bnuminterfaces_mismatch_01`;
  - `030_interface_class_invalid_01`;
  - `031_endpoint_wmaxpacketsize_zero_01`;
  - `032_endpoint_direction_mismatch_01`;
  - `040_string_invalid_length_01`;
  - `041_string_missing_index_01`.

## 6. Пробелы доказательной базы
- Для строгого доказательства фактических payload control-transfer требуется `usbmon`/pcap на каждый прогон.
- Окно `journalctl` местами содержит нерелевантные/фоновые события.
- В части кейсов финальные `lsusb` показывают нормализованные/обычные дескрипторы, поэтому факт экспозиции мутации нельзя переутверждать.

## 7. Следующее Linux-only направление
- Блок `MSC baseline/malformed`.
- Блок `composite/storage weirdness`.
- Блок `timing/state mutations`.

Фокус сохраняется: только безопасные descriptor/enumeration robustness-эксперименты, без утверждений о баге драйвера/ядра без прямого трассировочного подтверждения.

## 8. Дополнение: MSC-блок 050-059
Завершён отдельный Linux-only блок MSC-кейсов `050-059`; подробности вынесены в [docs/results/linux_msc_summary.md](/root/usb-driver-fuzzing-thesis/docs/results/linux_msc_summary.md).

Краткие счётчики по классификации для `050-059`:
- `OK`: 1
- `PARTIAL_ENUM`: 6
- `KERNEL_WARNING`: 4
- `EXPECTED_REJECT`: 0
- `DRIVER_BIND_ERROR`: 0
- `USERSPACE_FAILURE`: 0
- `DRIVER_CRASH`: 0
- `SYSTEM_CRASH`: 0

Интерпретация на верхнем уровне:
- во всех MSC-кейсах подтверждались признаки USB enumeration и bind к `usb-storage`;
- часть кейсов демонстрировала деградацию уже на storage/SCSI/block-слое;
- подтверждённых падений драйвера и системы не обнаружено.
