# Результат эксперимента: 060_composite_cdc_msc_baseline_01

##
1. Цель кейса
Проверить безопасный baseline composite-устройства (CDC ACM + инертный RAM-backed MSC) и зафиксировать фактическую реакцию Linux USB-стека на этапе runtime enumeration и bind драйверов.

##
2. Входные артефакты
- Конфигурация кейса: `experiments/cases/060_composite_cdc_msc_baseline.json`
- Wrapper log:
 
- `logs/runs/20260518_165850_060_composite_cdc_msc_baseline_01.log`
- Quick analysis:
 
- `logs/runs/20260518_165850_060_composite_cdc_msc_baseline_01.quick_analysis.txt`
- Raw Linux results:
 
- `results/raw/linux/060_composite_cdc_msc_baseline_01_20260518_165923`

##
3. Наблюдаемый результат runtime enumeration
- В wrapper-логе: `Runtime USB enumeration status: normal`.
- В raw-артефакте `runtime_status.txt`: `runtime_enum_status=normal`.
- В `lsusb_tree.txt` одновременно видны интерфейсы:
 
- `Class=Communications, Driver=cdc_acm`;
 
- `Class=CDC Data, Driver=cdc_acm`;
 
- `Class=Mass Storage, Driver=usb-storage`.

Итог: runtime-перечисление прошло успешно, composite CDC+MSC распознан, драйверы привязаны.

##
4. Linux evidence (wrapper + quick analysis + raw)
- Wrapper (`...165850...log`):
 
- запуск с `--no-serial-test --allow-runtime-enum-failure`;
 
- зафиксирован статус `normal`;
 
- snapshot записан в `...060_composite_cdc_msc_baseline_01_20260518_165923`.
- Quick analysis (`...165850...quick_analysis.txt`):
 
- подтверждает `Runtime USB enumeration status: normal`;
 
- подтверждает присутствие `ttyACM0`, `cdc_acm` и `usb-storage` по raw-файлам.
- Raw (`...165923`):
 
- `runtime_status.txt`: `runtime_enum_status=normal`;
 
- `lsusb.txt`/`system_snapshot.txt`: устройство `2e8a:4005` присутствует;
 
- `lsusb_tree.txt`: интерфейсы CDC и MSC привязаны (`cdc_acm`, `usb-storage`);
 
- `journalctl_k_tail.txt`/`journalctl_k_tail_300.txt`: есть строки `cdc_acm ... ttyACM0` и `usb-storage ... detected`, а также предупреждение `FAT-fs (sdb1): unable to read boot sector to mark fs as dirty`.

##
5. Классификация
`KERNEL_WARNING`

Обоснование:
- перечисление и bind прошли успешно (`normal`, `cdc_acm`, `usb-storage`), значит это не `PARTIAL_ENUM` и не `DRIVER_BIND_ERROR`;
- в kernel-логе текущего временного окна присутствует предупреждение уровня ядра/ФС (`FAT-fs ... unable to read boot sector ...`), что соответствует `KERNEL_WARNING`;
- доказательств `DRIVER_CRASH` или `SYSTEM_CRASH` в артефактах нет.

##
6. Уровень уверенности
Средний.

Причина: ключевые признаки успешного перечисления подтверждены напрямую несколькими источниками, но хвосты `journalctl` агрегируют и соседние события, что ограничивает точность атрибуции всех предупреждений только текущему запуску.

##
7. Пробелы в доказательной базе
- Нет полностью изолированного kernel-лога только для интервала запуска кейса 060.
- `dmesg_*` в этом запуске фактически пустые/недоступны (`Operation not permitted`), опора идёт на `journalctl`.
- Нет usbmon/pcap-трейса для пакетного подтверждения BOT-обмена MSC.

##
8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|Writing snapshot|--no-serial-test|allow runtime enumeration failure" logs/runs/20260518_165850_060_composite_cdc_msc_baseline_01.log`
2. `rg -n "runtime_enum_status" results/raw/linux/060_composite_cdc_msc_baseline_01_20260518_165923/runtime_status.txt`
3. `rg -n "2e8a:4005|Class=Communications|Class=CDC Data|Class=Mass Storage|Driver=cdc_acm|Driver=usb-storage|ttyACM0" results/raw/linux/060_composite_cdc_msc_baseline_01_20260518_165923/{lsusb.txt,system_snapshot.txt,lsusb_tree.txt,tty_devices.txt}`
4. `rg -n "usb-storage|scsi host9|ttyACM0|FAT-fs \(sdb1\): unable to read boot sector" results/raw/linux/060_composite_cdc_msc_baseline_01_20260518_165923/{journalctl_k_tail.txt,journalctl_k_tail_300.txt,kernel_usb_storage_scsi_sd.txt}`
5. `rg -n "BUG:|Oops|kernel panic|Call Trace|segfault|soft lockup|hard lockup" logs/runs/20260518_165850_060_composite_cdc_msc_baseline_01.quick_analysis.txt results/raw/linux/060_composite_cdc_msc_baseline_01_20260518_165923/{journalctl_k_tail.txt,journalctl_k_tail_300.txt,kernel_usb_storage_scsi_sd.txt}`
