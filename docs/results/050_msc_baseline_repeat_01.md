# Результат эксперимента: 050_msc_baseline_repeat_01

## 1. Цель кейса
Подтвердить повторяемость baseline-сценария MSC после внедрения MSC-aware runtime status detection: Linux должен корректно перечислить устройство, привязать интерфейс к `usb-storage` и не демонстрировать признаков падения драйвера/ядра/системы.

## 2. Входные артефакты
- Конфигурация кейса: `experiments/cases/050_msc_baseline.json`
- Wrapper logs:
  - `logs/runs/20260516_200028_050_msc_baseline_repeat_01.log`
  - `logs/runs/20260516_200717_050_msc_baseline_repeat_01.log`
- Quick analysis:
  - `logs/runs/20260516_200028_050_msc_baseline_repeat_01.quick_analysis.txt`
  - `logs/runs/20260516_200717_050_msc_baseline_repeat_01.quick_analysis.txt`
- Raw Linux results:
  - `results/raw/linux/050_msc_baseline_repeat_01_20260516_200059`
  - `results/raw/linux/050_msc_baseline_repeat_01_20260516_200747`
  - `results/raw/linux/050_msc_baseline_repeat_01_20260516_200748`

## 3. Наблюдаемый результат runtime enumeration
- В обоих wrapper-логах: `Runtime USB enumeration status: normal`.
- В `runtime_status.txt` для `...200059` и `...200748`: `runtime_enum_status=normal`.
- В snapshot/`lsusb` присутствует runtime-устройство `2e8a:4006` (`usb_case_msc_demo MSC baseline`).

Итог: runtime enumeration в повторном baseline-прогоне фиксируется как успешный (`normal`).

## 4. Linux evidence (wrapper + quick analysis + raw)
- Wrapper (`...200028...log`, `...200717...log`):
  - одинаковая команда запуска с `--allow-runtime-enum-failure` и `--no-serial-test`;
  - оба запуска фиксируют `Runtime USB enumeration status: normal`;
  - в обоих запусках runtime-USB идентифицируется как `2e8a:4006`.
- Quick analysis (`...200028...quick_analysis.txt`, `...200717...quick_analysis.txt`):
  - подтверждает статус `normal`;
  - подтверждает видимость `2e8a:4006`;
  - по raw-артефактам показывает bind к `usb-storage`.
- Raw (`...200059`, `...200748`; частично `...200747`):
  - `lsusb_tree.txt`: `Class=Mass Storage, Driver=usb-storage`;
  - `kernel_usb_storage_scsi_sd.txt` / `journalctl_k_tail*.txt`: есть `usb-storage ... detected`, `scsi host...`, `sd ... Attached SCSI removable disk`;
  - в `...200748/journalctl_k_tail_300.txt` присутствуют строки около времени `20:07:31-20:07:32` с детектированием `usb-storage` и attach диска.

Наблюдаемые предупреждения (`FAT-fs ... unable to read boot sector`, `sdb: unable to read partition table`) присутствуют, но по текущим данным не сопровождаются crash-сигнатурами.

## 5. Классификация
`OK`

Обоснование:
- в wrapper и raw статус запуска — `normal`;
- Linux перечисляет устройство и привязывает его к `usb-storage`;
- отсутствуют явные признаки `DRIVER_CRASH` или `SYSTEM_CRASH`.

## 6. Уровень уверенности
Средний.

Причина: ключевые признаки успешной инициализации повторены в двух последовательных запусках, но kernel-tail файлы содержат также события предыдущих сессий, что ограничивает строгую атрибуцию каждого warning только одному запуску.

## 7. Пробелы в доказательной базе
- `journalctl_k_tail*` и `kernel_usb_storage_scsi_sd.txt` агрегируют несколько запусков на хосте.
- Для `results/raw/linux/050_msc_baseline_repeat_01_20260516_200747` отсутствуют `runtime_status.txt`, `lsusb.txt`, `lsusb_tree.txt` (есть только частичный fallback-снимок).
- Нет отдельного usbmon/pcap-трейса только для этого прогона.

## 8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|Writing snapshot to" logs/runs/*050_msc_baseline_repeat_01*.log`
2. `for f in results/raw/linux/050_msc_baseline_repeat_01_*/runtime_status.txt; do echo "== $f"; cat "$f"; done`
3. `rg -n "2e8a:4006|Class=Mass Storage|Driver=usb-storage" results/raw/linux/050_msc_baseline_repeat_01_*/{lsusb.txt,lsusb_tree.txt,system_snapshot.txt}`
4. `rg -n "usb-storage|scsi host|Attached SCSI removable disk|unable to read partition table|FAT-fs" results/raw/linux/050_msc_baseline_repeat_01_*/{journalctl_k_tail.txt,journalctl_k_tail_300.txt,kernel_usb_storage_scsi_sd.txt}`
5. `rg -n "BUG:|Oops|kernel panic|Call Trace|segfault" results/raw/linux/050_msc_baseline_repeat_01_*/{journalctl_k_tail.txt,journalctl_k_tail_300.txt,dmesg_tail_300.txt,dmesg_full.txt}`
