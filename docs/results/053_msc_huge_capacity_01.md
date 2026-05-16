# Результат эксперимента: 053_msc_huge_capacity_01

## 1. Цель кейса
Проверить реакцию Linux USB/MSC-стека на безопасную мутацию `Mass Storage Class`, при которой устройство остаётся корректно перечисляемым как MSC, но сообщает нереалистично большую ёмкость носителя (очень большой `last LBA`).

## 2. Входные артефакты
- Конфигурация кейса: `experiments/cases/053_msc_huge_capacity.json`
- Wrapper log:
  - `logs/runs/20260516_214546_053_msc_huge_capacity_01.log`
- Quick analysis:
  - `logs/runs/20260516_214546_053_msc_huge_capacity_01.quick_analysis.txt`
- Raw Linux results:
  - `results/raw/linux/053_msc_huge_capacity_01_20260516_214701`

## 3. Наблюдаемый результат runtime enumeration
- В wrapper-логе: `Runtime USB enumeration status: normal`.
- В `runtime_status.txt`: `runtime_enum_status=normal`.
- Устройство присутствует как `2e8a:4006` (`usb_case_msc_demo MSC baseline`), в `lsusb_tree.txt` интерфейс определён как `Class=Mass Storage, Driver=usb-storage`.

Итог: USB-перечисление и bind к `usb-storage` выполнены.

## 4. Linux evidence (wrapper + quick analysis + raw)
- Wrapper (`...214546...log`):
  - запуск с `--allow-runtime-enum-failure` и `--no-serial-test`;
  - snapshot: `/home/numi/vkr-usb/logs/053_msc_huge_capacity_01_20260516_214701`;
  - `Runtime USB enumeration status: normal`.
- Quick analysis (`...214546...quick_analysis.txt`):
  - подтверждает `Runtime USB enumeration status: normal`;
  - подтверждает тот же snapshot/fallback-каталог.
- Raw (`...214701`):
  - `lsusb.txt`, `system_snapshot.txt`, `lsusb_v_2e8a_4006.txt`: устройство `2e8a:4006` присутствует;
  - `lsusb_tree.txt`: `Class=Mass Storage, Driver=usb-storage`;
  - `journalctl_k_tail.txt` и `kernel_usb_storage_scsi_sd.txt` содержат ошибки на блочном уровне, включая:
    - `Read Capacity(10) failed: Result: hostbyte=DID_ERROR driverbyte=DRIVER_OK`;
    - `I/O error, dev sdb, sector ...`;
    - `Buffer I/O error on dev sdb1 ...`;
    - `FAT-fs (sdb1): unable to read boot sector to mark fs as dirty`.
- По шаблонам `BUG:`, `Oops`, `kernel panic`, `Call Trace`, `segfault` совпадений в raw-логах нет.

## 5. Классификация
`PARTIAL_ENUM`

Обоснование:
- перечисление USB и привязка к `usb-storage` наблюдаются;
- при этом дальнейшая работа блочного устройства деградирует из-за ошибок чтения/доступа к секторам;
- подтверждений `DRIVER_CRASH` и `SYSTEM_CRASH` нет.

## 6. Уровень уверенности
Средний.

Причина: ключевые признаки согласуются между wrapper, quick analysis и raw-артефактами, но kernel-журнал агрегирован и включает события нескольких подключений/кейсов в общей временной ленте.

## 7. Пробелы в доказательной базе
- `dmesg` недоступен непривилегированному пользователю (`Операция не позволена`), анализ ядра основан на `journalctl`.
- В `journalctl_*` и `kernel_usb_storage_scsi_sd.txt` присутствуют события не только текущего запуска, что снижает изоляцию именно `053_msc_huge_capacity_01`.
- Нет отдельного usbmon/pcap-трейса строго для одного цикла подключения этого кейса.

## 8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|Writing snapshot to|Fallback logs directory|allow-runtime-enum-failure|no-serial-test|dmesg: чтение буфера ядра" logs/runs/20260516_214546_053_msc_huge_capacity_01.log logs/runs/20260516_214546_053_msc_huge_capacity_01.quick_analysis.txt`
2. `cat results/raw/linux/053_msc_huge_capacity_01_20260516_214701/runtime_status.txt`
3. `rg -n "2e8a:4006|usb_case_msc_demo MSC baseline|Class=Mass Storage|Driver=usb-storage" results/raw/linux/053_msc_huge_capacity_01_20260516_214701/{lsusb.txt,lsusb_tree.txt,system_snapshot.txt,lsusb_v_2e8a_4006.txt}`
4. `rg -n "Read Capacity\(10\) failed|DID_ERROR|I/O error|Buffer I/O error|FAT-fs|usb-storage|scsi host9|Attached SCSI removable disk" results/raw/linux/053_msc_huge_capacity_01_20260516_214701/{journalctl_k_tail.txt,journalctl_k_tail_300.txt,kernel_usb_storage_scsi_sd.txt}`
5. `rg -n "BUG:|Oops|kernel panic|Call Trace|segfault|general protection fault|soft lockup|hard lockup" results/raw/linux/053_msc_huge_capacity_01_20260516_214701/{journalctl_k_tail.txt,journalctl_k_tail_300.txt,kernel_usb_storage_scsi_sd.txt,dmesg_tail_300.txt,dmesg_full.txt}`
