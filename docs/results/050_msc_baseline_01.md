# Результат эксперимента: 050_msc_baseline_01

## 1. Цель кейса
Проверить базовый безопасный сценарий Mass Storage Class (MSC BOT) с инертным RAM-backed носителем и зафиксировать фактический исход runtime-enumeration в Linux.

## 2. Входные артефакты
- Конфигурация кейса: `experiments/cases/050_msc_baseline.json`
- Wrapper logs:
  - `logs/runs/20260516_194331_050_msc_baseline_01.log`
  - `logs/runs/20260516_194919_050_msc_baseline_01.log`
- Quick analysis:
  - `logs/runs/20260516_194331_050_msc_baseline_01.quick_analysis.txt`
  - `logs/runs/20260516_194919_050_msc_baseline_01.quick_analysis.txt`
- Raw Linux results:
  - `results/raw/linux/050_msc_baseline_01_20260516_194402`
  - `results/raw/linux/050_msc_baseline_01_20260516_194949`

## 3. Наблюдаемый результат runtime enumeration
- В обоих wrapper-логах: `Runtime USB enumeration status: partial`.
- В обоих `runtime_status.txt`: `runtime_enum_status=partial`.
- В snapshot присутствует runtime-устройство `2e8a:4006` (`usb_case_msc_demo MSC baseline`).

Вывод по runtime enumeration: устройство определяется и привязывается к `usb-storage`, но итоговый статус раннера остаётся `partial`.

## 4. Linux evidence (wrapper + quick analysis + raw)
- Wrapper (`...194331...log`, `...194919...log`):
  - зафиксирован режим `allow runtime enumeration failure`;
  - в обоих прогонах зафиксирован статус `partial`;
  - serial smoke-test отключён (`--no-serial-test`), что ожидаемо для MSC-кейса.
- Quick analysis (`...194331...quick_analysis.txt`, `...194919...quick_analysis.txt`):
  - подтверждает `Runtime USB enumeration status: partial`;
  - фиксирует `2e8a:4006` в `lsusb/system_snapshot`;
  - в `lsusb_tree` отмечен `Class=Mass Storage, Driver=usb-storage`.
- Raw (`...194402`, `...194949`):
  - `runtime_status.txt`: `runtime_enum_status=partial`;
  - `lsusb.txt`/`system_snapshot.txt`: `2e8a:4006` присутствует;
  - `lsusb_tree.txt`: `Class=Mass Storage, Driver=usb-storage, 12M`;
  - `journalctl_k_tail*.txt`: есть строки `usb-storage ... detected`, `scsi host9 ...`, `sd ... Attached SCSI removable disk`.

Дополнительно по каталогу `...194949`: присутствуют сообщения `sdb: unable to read partition table` и `partition table beyond EOD`; это подтверждает проблемы на уровне разметки/носителя, но не является доказательством падения драйвера или ядра.

## 5. Классификация
`PARTIAL_ENUM`

Обоснование:
- в обоих независимых запусках и в wrapper, и в raw-статусе зафиксирован `partial`;
- при этом устройство обнаруживается, а `usb-storage`/SCSI-инициализация происходят;
- подтверждений `DRIVER_CRASH` или `SYSTEM_CRASH` в артефактах нет.

## 6. Уровень уверенности
Средний.

Причина: статус `partial` воспроизводится в двух прогонах, но точная причина неполной классификации раннера (при успешном bind `usb-storage`) из имеющихся артефактов не декомпозирована до одного фактора.

## 7. Пробелы в доказательной базе
- Нет изолированного post-enumeration снимка только для текущего запуска (в `journalctl_k_tail*` присутствуют события предыдущих запусков).
- Нет usbmon/pcap BOT-трейса для подтверждения полного цикла обмена CBW/CSW.
- Не выполнена отдельная проверка блочного устройства (`fdisk -l`/`blkid`/контролируемое RO-чтение) сразу после каждого запуска.

## 8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|Writing snapshot to" logs/runs/*050_msc_baseline_01*.log`
2. `rg -n "runtime_enum_status" results/raw/linux/050_msc_baseline_01_*/runtime_status.txt`
3. `rg -n "2e8a:4006|Class=Mass Storage|Driver=usb-storage" results/raw/linux/050_msc_baseline_01_*/{lsusb.txt,system_snapshot.txt,lsusb_tree.txt}`
4. `rg -n "usb-storage 1-4:1.0|scsi host9|Attached SCSI removable disk|unable to read partition table|partition table beyond EOD" results/raw/linux/050_msc_baseline_01_*/journalctl_k_tail*.txt`
5. `rg -n "BUG:|Oops|kernel panic|Call Trace|segfault" results/raw/linux/050_msc_baseline_01_*/{journalctl_k_tail.txt,journalctl_k_tail_300.txt,dmesg_tail_300.txt,dmesg_full.txt}`

## 9. Примечание по корректировке раннера
- Изначально раннер классифицировал запуск как `partial`, так как эвристика `normal` была ориентирована на `ttyACM` (CDC).
- По артефактам этого кейса устройство `2e8a:4006` успешно доходило до `Class=Mass Storage, Driver=usb-storage` и SCSI-attach (`usb-storage/scsi/sd` в kernel-логах).
- `tools/run-remote-linux-case.sh` обновлён: для MSC-кейсов наличие RP2040-устройства и признаков bind к `usb-storage` теперь считается `normal`.
- Сообщения о разметке носителя (`unable to read partition table`, `partition table beyond EOD`) трактуются как особенности media layout, а не падение драйвера/системы.
