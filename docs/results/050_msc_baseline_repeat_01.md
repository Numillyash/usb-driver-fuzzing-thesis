# Результат эксперимента: 050_msc_baseline_repeat_01

## 1. Цель кейса
Подтвердить воспроизводимость baseline-сценария MSC после добавления MSC-aware детекции runtime-статуса: устройство должно корректно перечисляться в Linux и привязываться к `usb-storage` без признаков падения драйвера/ядра.

## 2. Входные артефакты
- Конфигурация кейса: `experiments/cases/050_msc_baseline.json`
- Wrapper log: `logs/runs/20260516_200028_050_msc_baseline_repeat_01.log`
- Quick analysis: `logs/runs/20260516_200028_050_msc_baseline_repeat_01.quick_analysis.txt`
- Raw Linux results: `results/raw/linux/050_msc_baseline_repeat_01_20260516_200059`

## 3. Наблюдаемый результат runtime enumeration
- В wrapper-логе: `Runtime USB enumeration status: normal`.
- В raw-статусе: `runtime_enum_status=normal`.
- В `lsusb` присутствует runtime-устройство `2e8a:4006` (`usb_case_msc_demo MSC baseline`).

Итог: runtime-enumeration успешен.

## 4. Linux evidence (wrapper + quick analysis + raw)
- Wrapper (`...200028...log`):
  - устройство после прошивки определяется как `2e8a:4006`;
  - раннер фиксирует `Runtime USB enumeration status: normal`;
  - serial smoke-test отключён флагом `--no-serial-test` (ожидаемо для MSC).
- Quick analysis (`...200028...quick_analysis.txt`):
  - подтверждает `Runtime USB enumeration status: normal`;
  - подтверждает наличие `2e8a:4006` в snapshot;
  - в `lsusb_tree` отмечен `Class=Mass Storage, Driver=usb-storage`.
- Raw (`...200059`):
  - `runtime_status.txt`: `runtime_enum_status=normal`;
  - `lsusb.txt`/`system_snapshot.txt`: `Bus 001 Device 066: ID 2e8a:4006`;
  - `lsusb_tree.txt`: `Class=Mass Storage, Driver=usb-storage, 12M`;
  - `journalctl_k_tail*.txt`: есть `usb-storage ... detected`, `scsi host9 ...`, `sd ... Attached SCSI removable disk`.

Дополнительно: в kernel-логах встречаются сообщения вида `FAT-fs ... unable to read boot sector...` и `sdb: unable to read partition table`. По текущим артефактам это выглядит как нефатальная особенность layout тестового носителя и не сопровождается признаками crash.

## 5. Классификация
`OK`

Обоснование:
- runtime-статус зафиксирован как `normal` в wrapper и raw;
- Linux видит устройство `2e8a:4006` и привязывает интерфейс к `usb-storage`;
- доказательств `DRIVER_CRASH` или `SYSTEM_CRASH` нет.

## 6. Уровень уверенности
Средний.

Причина: ключевые признаки успешной инициализации совпадают, но tail-логи содержат также события предыдущих прогонов, что ограничивает точную атрибуцию всех warning-сообщений только этому запуску.

## 7. Пробелы в доказательной базе
- `journalctl_k_tail*` и `kernel_usb_storage_scsi_sd.txt` агрегируют несколько сессий, а не исключительно текущий запуск.
- Нет изолированного USB-трейса (usbmon/pcap) для этого конкретного прогона.
- Не выполнена отдельная RO-проверка блочного устройства сразу после текущего запуска.

## 8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|2e8a:4006" logs/runs/20260516_200028_050_msc_baseline_repeat_01.log`
2. `rg -n "runtime_enum_status" results/raw/linux/050_msc_baseline_repeat_01_20260516_200059/runtime_status.txt`
3. `rg -n "2e8a:4006|Class=Mass Storage|Driver=usb-storage" results/raw/linux/050_msc_baseline_repeat_01_20260516_200059/{lsusb.txt,system_snapshot.txt,lsusb_tree.txt}`
4. `rg -n "usb-storage 1-4:1.0|scsi host9|Attached SCSI removable disk|unable to read partition table|unable to read boot sector" results/raw/linux/050_msc_baseline_repeat_01_20260516_200059/{journalctl_k_tail.txt,journalctl_k_tail_300.txt,kernel_usb_storage_scsi_sd.txt}`
5. `rg -n "BUG:|Oops|kernel panic|Call Trace|segfault" results/raw/linux/050_msc_baseline_repeat_01_20260516_200059/{journalctl_k_tail.txt,journalctl_k_tail_300.txt,dmesg_tail_300.txt,dmesg_full.txt}`
