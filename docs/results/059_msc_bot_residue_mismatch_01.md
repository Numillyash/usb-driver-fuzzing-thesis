# Результат эксперимента: 059_msc_bot_residue_mismatch_01

## 1. Цель кейса
Проверить реакцию Linux USB/MSC-стека на безопасную class-level мутацию BOT (контролируемый short-data эквивалент рассогласования residue для READ(10)) в inert/read-only профиле устройства.

## 2. Входные артефакты
- Конфигурация кейса: `experiments/cases/059_msc_bot_residue_mismatch.json`
- Wrapper log:
  - `logs/runs/20260517_012449_059_msc_bot_residue_mismatch_01.log`
- Quick analysis:
  - `logs/runs/20260517_012449_059_msc_bot_residue_mismatch_01.quick_analysis.txt`
- Raw Linux results:
  - `results/raw/linux/059_msc_bot_residue_mismatch_01_20260517_012516`
  - `results/raw/linux/059_msc_bot_residue_mismatch_01_20260517_012517`

## 3. Наблюдаемый результат runtime enumeration
- В wrapper-логе зафиксировано: `Runtime USB enumeration status: normal`.
- В raw-артефакте `runtime_status.txt`: `runtime_enum_status=normal`.
- В `lsusb.txt` и `lsusb_tree.txt` устройство видно как `2e8a:4006`, класс Mass Storage, драйвер `usb-storage`.

Итог: перечисление и bind `usb-storage` в текущем запуске произошли.

## 4. Linux evidence (wrapper + quick analysis + raw)
- Wrapper (`...012449...log`):
  - запуск с `--no-serial-test --allow-runtime-enum-failure`;
  - runtime-статус `normal`;
  - snapshot-директории `...012516` и fallback `...012517`.
- Quick analysis (`...012449...quick_analysis.txt`):
  - подтверждает `Runtime USB enumeration status: normal`;
  - подтверждает наличие `2e8a:4006` в snapshot;
  - показывает kernel-сообщения уровня `usb-storage/scsi/sd` и I/O предупреждения в хвостах журналов.
- Raw (`...012517`):
  - `lsusb_tree.txt`: `Class=Mass Storage, Driver=usb-storage`;
  - `journalctl_k_tail.txt` содержит окно текущего запуска (`2026-05-17 01:24:59`–`01:25:16`) с детектом `usb-storage`, созданием `sdb/sdb1` и предупреждением `FAT-fs (sdb1): unable to read boot sector to mark fs as dirty`;
  - `journalctl_k_tail_300.txt` и `kernel_usb_storage_scsi_sd.txt` также содержат более ранние события (включая `2026-05-17 00:31:52` UBSAN trace и серии I/O ошибок), которые не изолированы только текущим запуском.

## 5. Классификация
`KERNEL_WARNING`

Обоснование:
- bind драйвера прошёл успешно (`runtime_enum_status=normal`, `Driver=usb-storage`), значит это не `EXPECTED_REJECT` и не `DRIVER_BIND_ERROR`;
- в окне текущего запуска присутствуют kernel-level предупреждения файловой подсистемы (`FAT-fs ... unable to read boot sector ...`), что соответствует `KERNEL_WARNING`;
- явных признаков `DRIVER_CRASH` или `SYSTEM_CRASH` для текущего запуска не зафиксировано.

## 6. Уровень уверенности
Средний.

Причина: ключевые признаки перечисления и привязки драйвера подтверждаются напрямую, но хвосты `journalctl` агрегируют события нескольких прогонов, что ограничивает точную атрибуцию всех ошибок именно кейсу 059.

## 7. Пробелы в доказательной базе
- Нет строгой временной изоляции логов только на интервал запуска `2026-05-17T01:24:49+03:00`.
- В артефактах присутствуют более ранние kernel-события (в т.ч. UBSAN trace), и без отдельного time-window их нельзя однозначно приписывать кейсу 059.
- Отсутствует usbmon/pcap-трейс BOT-транзакций, поэтому подтверждение short-data/residue-эффекта основано не на пакетном уровне, а на косвенных системных симптомах.

## 8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|runtime_enum_status|allow-runtime-enum-failure|--no-serial-test" logs/runs/20260517_012449_059_msc_bot_residue_mismatch_01.log results/raw/linux/059_msc_bot_residue_mismatch_01_20260517_012517/runtime_status.txt`
2. `rg -n "2e8a:4006|Class=Mass Storage|Driver=usb-storage" results/raw/linux/059_msc_bot_residue_mismatch_01_20260517_012517/{lsusb.txt,lsusb_tree.txt}`
3. `rg -n "2026-05-17 01:24:59|2026-05-17 01:25:0|2026-05-17 01:25:1|usb-storage 1-4:1.0|scsi host9|FAT-fs" results/raw/linux/059_msc_bot_residue_mismatch_01_20260517_012517/{journalctl_k_tail.txt,journalctl_k_tail_300.txt,kernel_usb_storage_scsi_sd.txt}`
4. `rg -n "UBSAN|shift-out-of-bounds|invalid wMaxPacketSize|I/O error|unable to read partition table" results/raw/linux/059_msc_bot_residue_mismatch_01_20260517_012517/{journalctl_k_tail.txt,journalctl_k_tail_300.txt,kernel_usb_storage_scsi_sd.txt}`
5. `rg -n "BUG:|Oops|kernel panic|soft lockup|hard lockup|watchdog" logs/runs/20260517_012449_059_msc_bot_residue_mismatch_01.log logs/runs/20260517_012449_059_msc_bot_residue_mismatch_01.quick_analysis.txt results/raw/linux/059_msc_bot_residue_mismatch_01_20260517_012517/*`
