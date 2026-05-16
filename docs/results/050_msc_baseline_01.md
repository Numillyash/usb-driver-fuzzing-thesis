# Результат эксперимента: 050_msc_baseline_01

## 1. Цель кейса
Проверить базовый безопасный сценарий Mass Storage Class (MSC BOT) с инертным RAM-backed носителем (read-only) и зафиксировать фактический исход runtime-перечисления в Linux без аномальных USB-мутаций.

## 2. Входные артефакты
- Case config: `experiments/cases/050_msc_baseline.json`
- Wrapper log:
  - `logs/runs/20260516_194331_050_msc_baseline_01.log`
- Quick analysis:
  - `logs/runs/20260516_194331_050_msc_baseline_01.quick_analysis.txt`
- Raw Linux results:
  - `results/raw/linux/050_msc_baseline_01_20260516_194402`

## 3. Наблюдаемый результат runtime enumeration
- В wrapper-логе: `Runtime USB enumeration status: partial`.
- В raw-артефакте: `runtime_enum_status=partial` (`runtime_status.txt`).
- Устройство в runtime присутствует как `2e8a:4006` (`usb_case_msc_demo MSC baseline`).

Итог: перечисление не классифицируется как полностью штатное в рамках текущего раннера (`partial`), но устройство обнаруживается хостом.

## 4. Linux evidence (wrapper + quick analysis + raw)
- Wrapper (`...194331...log`):
  - после flash зафиксирован runtime VID:PID `2e8a:4006`;
  - статус рантайма: `partial`;
  - serial smoke-test отключён (`--no-serial-test`), что ожидаемо для MSC-кейса.
- Quick analysis (`...194331...quick_analysis.txt`):
  - подтверждает `Runtime USB enumeration status: partial`;
  - фиксирует присутствие `2e8a:4006` в snapshot;
  - в `lsusb_tree` отмечен интерфейс `Class=Mass Storage, Driver=usb-storage`.
- Raw (`...194402`):
  - `runtime_status.txt`: `runtime_enum_status=partial`;
  - `lsusb.txt`/`system_snapshot.txt`: устройство `2e8a:4006` присутствует;
  - `lsusb_tree.txt`: `Port 004 ... Class=Mass Storage, Driver=usb-storage, 12M`;
  - `lsusb_v_2e8a_4006.txt`: интерфейс `Mass Storage`, `SCSI`, `Bulk-Only`;
  - `journalctl_k_tail.txt` (метки времени запуска 19:43:53-19:44:01):
    - обнаружение `idVendor=2e8a, idProduct=4006`;
    - `usb-storage 1-4:1.0: USB Mass Storage device detected`;
    - `scsi host9: usb-storage 1-4:1.0`.

## 5. Классификация
- Основная: `PARTIAL_ENUM`.

Обоснование:
- оба канала статуса (wrapper + `runtime_status.txt`) явно дают `partial`;
- при этом есть частично успешная инициализация USB-уровня и bind `usb-storage`;
- доказательств `DRIVER_CRASH` или `SYSTEM_CRASH` в предоставленных артефактах нет.

## 6. Уровень уверенности
Средний.

Причина: признаки перечисления и привязки драйвера подтверждаются несколькими источниками, но итоговый статус раннера остаётся `partial`, а в snapshot нет полного подтверждения стабильного финального состояния блочного устройства в момент съёма.

## 7. Пробелы в доказательной базе
- Нет отдельной узкой временной выборки `lsblk`/`/dev/sdX` именно после строки `usb-storage ... detected` для запуска `2026-05-16 19:43:53`.
- Нет usbmon/pcap-трейса BOT-обмена (CBW/CSW), поэтому нельзя подтвердить полноценную I/O-сессию, только факт bind.
- `dmesg` недоступен текущему пользователю в раннере (использован fallback через `journalctl`).

## 8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|runtime_enum_status" logs/runs/20260516_194331_050_msc_baseline_01.log results/raw/linux/050_msc_baseline_01_20260516_194402/runtime_status.txt`
2. `rg -n "2e8a:4006|Mass Storage|usb-storage|scsi host9" results/raw/linux/050_msc_baseline_01_20260516_194402/{lsusb.txt,system_snapshot.txt,lsusb_tree.txt,journalctl_k_tail.txt,journalctl_k_tail_300.txt,lsusb_v_2e8a_4006.txt}`
3. `rg -n "panic|Oops|BUG:|Call Trace|segfault|kernel panic|WARNING:" results/raw/linux/050_msc_baseline_01_20260516_194402/{journalctl_k_tail.txt,journalctl_k_tail_300.txt,dmesg_tail_300.txt,dmesg_full.txt}`
4. `rg -n "sdb|Attached SCSI removable disk|Write Protect" results/raw/linux/050_msc_baseline_01_20260516_194402/{journalctl_k_tail.txt,journalctl_k_tail_300.txt}`
