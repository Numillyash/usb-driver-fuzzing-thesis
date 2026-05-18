# Результат эксперимента: 061_composite_hid_msc_baseline_01

##
1. Цель кейса
Проверить безопасный baseline composite-устройства (инертный generic HID + read-only RAM-backed MSC) и зафиксировать фактическую реакцию Linux USB-стека на этапе runtime enumeration и bind драйверов.

##
2. Входные артефакты
- Конфигурация кейса: `experiments/cases/061_composite_hid_msc_baseline.json`
- Wrapper log:

- `logs/runs/20260518_175848_061_composite_hid_msc_baseline_01.log`
- Quick analysis:

- `logs/runs/20260518_175848_061_composite_hid_msc_baseline_01.quick_analysis.txt`
- Raw Linux results:

- `results/raw/linux/061_composite_hid_msc_baseline_01_20260518_175916`

##
3. Наблюдаемый результат runtime enumeration
- В wrapper-логе: `Runtime USB enumeration status: normal`.
- В raw-артефакте `runtime_status.txt`: `runtime_enum_status=normal`.
- В `lsusb_tree.txt` для `Dev 090` видны оба интерфейса:

- `If 0, Class=Human Interface Device, Driver=usbhid`;
- `If 1, Class=Mass Storage, Driver=usb-storage`.

Итог: runtime-перечисление прошло успешно, composite HID+MSC распознан, драйверы `usbhid` и `usb-storage` привязаны.

##
4. Linux evidence (wrapper + quick analysis + raw)
- Wrapper (`...175848...log`):

- запуск с `--no-serial-test --allow-runtime-enum-failure`;
- статус перечисления: `normal`;
- snapshot записан в `...061_composite_hid_msc_baseline_01_20260518_175916`.
- Quick analysis (`...175848...quick_analysis.txt`):

- подтверждает `Runtime USB enumeration status: normal`;
- фиксирует присутствие устройства `2e8a:4005` в raw snapshot;
- фиксирует bind HID (`hid-generic`) и MSC (`usb-storage`) в kernel log.
- Raw (`...175916`):

- `runtime_status.txt`: `runtime_enum_status=normal`;
- `lsusb.txt` и `system_snapshot.txt`: устройство `2e8a:4005 RP2040 USB Research usb_case_composite_hid_msc_demo` присутствует;
- `lsusb_tree.txt`: одновременное присутствие HID+MSC интерфейсов с драйверами `usbhid`/`usb-storage`;
- `journalctl_k_tail.txt` (временное окно запуска 17:58:59-17:59:16):

- `hid-generic ... USB HID v1.11 Device`;
- `usb-storage 1-4:1.1: USB Mass Storage device detected`;
- `FAT-fs (sdb1): unable to read boot sector to mark fs as dirty`.

##
5. Классификация
`KERNEL_WARNING`

Обоснование:
- перечисление и bind успешны (`normal`, `usbhid`, `usb-storage`), поэтому это не `PARTIAL_ENUM` и не `DRIVER_BIND_ERROR`;
- в kernel-логе текущего запуска присутствует предупреждение `FAT-fs ... unable to read boot sector ...`, что соответствует `KERNEL_WARNING`;
- подтверждений `DRIVER_CRASH` или `SYSTEM_CRASH` в артефактах нет.

##
6. Уровень уверенности
Средний.

Причина: ключевые признаки успешного перечисления и bind подтверждены несколькими независимыми артефактами, но `journalctl`-хвосты содержат также события соседних запусков, что ограничивает строгость атрибуции части сообщений к одному кейсу.

##
7. Пробелы в доказательной базе
- Нет полностью изолированного kernel-лога только для интервала выполнения кейса 061.
- `dmesg` в данном прогоне недоступен (`Operation not permitted`), опора идёт на `journalctl`.
- Нет usbmon/pcap-трейса для пакетного подтверждения BOT-обмена MSC и источника `FAT-fs` предупреждения.

##
8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|Writing snapshot|allow runtime enumeration failure|--no-serial-test" logs/runs/20260518_175848_061_composite_hid_msc_baseline_01.log`
2. `rg -n "runtime_enum_status" results/raw/linux/061_composite_hid_msc_baseline_01_20260518_175916/runtime_status.txt`
3. `rg -n "2e8a:4005|Class=Human Interface Device|Class=Mass Storage|Driver=usbhid|Driver=usb-storage" results/raw/linux/061_composite_hid_msc_baseline_01_20260518_175916/{lsusb.txt,system_snapshot.txt,lsusb_tree.txt,lsusb_v_2e8a_4005.txt}`
4. `rg -n "17:58:59|17:59:08|17:59:16|hid-generic|usb-storage|FAT-fs|unable to read boot sector" results/raw/linux/061_composite_hid_msc_baseline_01_20260518_175916/{journalctl_k_tail.txt,journalctl_k_tail_300.txt}`
5. `rg -n "BUG:|Oops|kernel panic|Call Trace|general protection fault|soft lockup|hard lockup" logs/runs/20260518_175848_061_composite_hid_msc_baseline_01.quick_analysis.txt results/raw/linux/061_composite_hid_msc_baseline_01_20260518_175916/{journalctl_k_tail.txt,journalctl_k_tail_300.txt,kernel_usb_storage_scsi_sd.txt}`
