# Результат эксперимента: 052_msc_zero_capacity_01

## 1. Цель кейса
Проверить реакцию Linux USB/MSC-стека на безопасную мутацию `Mass Storage Class`, в которой устройство перечисляется как MSC, но сообщает нулевую полезную ёмкость носителя (`block_count=0`).

## 2. Входные артефакты
- Конфигурация кейса: `experiments/cases/052_msc_zero_capacity.json`
- Wrapper log:
  - `logs/runs/20260516_205712_052_msc_zero_capacity_01.log`
- Quick analysis:
  - `logs/runs/20260516_205712_052_msc_zero_capacity_01.quick_analysis.txt`
- Raw Linux results:
  - `results/raw/linux/052_msc_zero_capacity_01_20260516_205758`

## 3. Наблюдаемый результат runtime enumeration
- В wrapper-логе зафиксировано: `Runtime USB enumeration status: normal`.
- В raw-артефакте `runtime_status.txt`: `runtime_enum_status=normal`.
- Runtime USB-устройство видимо как `2e8a:4006` (`usb_case_msc_demo MSC baseline`), интерфейс в `lsusb_tree.txt`: `Class=Mass Storage, Driver=usb-storage`.

Итог: на уровне USB-перечисления и привязки к `usb-storage` runtime enumeration успешен.

## 4. Linux evidence (wrapper + quick analysis + raw)
- Wrapper (`...205712...log`):
  - запуск с `--allow-runtime-enum-failure` и `--no-serial-test`;
  - snapshot: `/home/numi/vkr-usb/logs/052_msc_zero_capacity_01_20260516_205758`;
  - `Runtime USB enumeration status: normal`.
- Quick analysis (`...205712...quick_analysis.txt`):
  - подтверждает `Runtime USB enumeration status: normal`;
  - указывает тот же snapshot/fallback-каталог.
- Raw (`...205758`):
  - `lsusb.txt`, `system_snapshot.txt`, `lsusb_v_2e8a_4006.txt`: устройство `2e8a:4006` присутствует;
  - `lsusb_tree.txt`: `Class=Mass Storage, Driver=usb-storage`;
  - `journalctl_k_tail.txt` и `kernel_usb_storage_scsi_sd.txt` содержат признаки проблем на блочном уровне:
    - `sd ... [sdb] 0 512-byte logical blocks: (0 B/0 B)`;
    - `Read Capacity(10) failed: Result: hostbyte=DID_ERROR driverbyte=DRIVER_OK`;
    - `I/O error, dev sdb ...` и `Buffer I/O error on dev sdb1 ...`;
    - `FAT-fs (sdb1): unable to read boot sector to mark fs as dirty`.
- Прямых признаков аварии ядра/системы (`BUG:`, `Oops`, `kernel panic`, `Call Trace`) в артефактах не выявлено.

## 5. Классификация
`PARTIAL_ENUM`

Обоснование:
- USB/MSC-уровень перечисления и bind к `usb-storage` наблюдаются;
- одновременно фиксируются ошибки чтения ёмкости и I/O на блочном устройстве, то есть инициализация storage-функциональности деградирована;
- подтверждений `DRIVER_CRASH` или `SYSTEM_CRASH` нет.

## 6. Уровень уверенности
Средний.

Причина: ключевые индикаторы подтверждаются в нескольких источниках (wrapper, quick analysis, raw), однако `journalctl`-выгрузки агрегируют события нескольких последовательных подключений устройства в общей временной ленте.

## 7. Пробелы в доказательной базе
- `dmesg` в прогоне недоступен непривилегированному пользователю (`Операция не позволена`), поэтому анализ ядра основан на `journalctl`.
- В `journalctl_*` присутствуют события предыдущих подключений MSC-кейсов, что снижает изоляцию именно данного запуска.
- Нет отдельного usbmon/pcap-трейса строго для одного цикла подключения `052_msc_zero_capacity_01`.

## 8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|Writing snapshot to|Fallback logs directory|dmesg: чтение буфера ядра завершилось неудачно" logs/runs/20260516_205712_052_msc_zero_capacity_01.log logs/runs/20260516_205712_052_msc_zero_capacity_01.quick_analysis.txt`
2. `cat results/raw/linux/052_msc_zero_capacity_01_20260516_205758/runtime_status.txt`
3. `rg -n "2e8a:4006|Class=Mass Storage|Driver=usb-storage" results/raw/linux/052_msc_zero_capacity_01_20260516_205758/{lsusb.txt,lsusb_tree.txt,system_snapshot.txt,lsusb_v_2e8a_4006.txt}`
4. `rg -n "0 512-byte logical blocks|Read Capacity\(10\) failed|I/O error|Buffer I/O error|FAT-fs|usb-storage|scsi host9|Attached SCSI removable disk" results/raw/linux/052_msc_zero_capacity_01_20260516_205758/{journalctl_k_tail.txt,journalctl_k_tail_300.txt,kernel_usb_storage_scsi_sd.txt}`
5. `rg -n "BUG:|Oops|kernel panic|Call Trace|segfault" results/raw/linux/052_msc_zero_capacity_01_20260516_205758/{journalctl_k_tail.txt,journalctl_k_tail_300.txt,dmesg_tail_300.txt,dmesg_full.txt}`
