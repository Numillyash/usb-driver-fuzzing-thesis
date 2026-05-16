# Результат эксперимента: 051_msc_zero_block_size_01

## 1. Цель кейса
Проверить реакцию Linux USB/MSC-стека на безопасную мутацию `Mass Storage Class`, в которой устройство остаётся корректно дескрипторно перечисляемым, но в логике ёмкости возвращает некорректный параметр `logical block size = 0`.

## 2. Входные артефакты
- Конфигурация кейса: `experiments/cases/051_msc_zero_block_size.json`
- Wrapper log:
  - `logs/runs/20260516_203944_051_msc_zero_block_size_01.log`
- Quick analysis:
  - `logs/runs/20260516_203944_051_msc_zero_block_size_01.quick_analysis.txt`
- Raw Linux results:
  - `results/raw/linux/051_msc_zero_block_size_01_20260516_204015`
  - `results/raw/linux/051_msc_zero_block_size_01_20260516_204016`

## 3. Наблюдаемый результат runtime enumeration
- В wrapper-логе зафиксировано: `Runtime USB enumeration status: normal`.
- В `results/raw/linux/051_msc_zero_block_size_01_20260516_204016/runtime_status.txt`: `runtime_enum_status=normal`.
- Runtime USB-устройство наблюдается как `2e8a:4006` (`usb_case_msc_demo MSC baseline`).

Итог: USB runtime enumeration прошёл успешно на уровне обнаружения устройства и привязки USB-интерфейса.

## 4. Linux evidence (wrapper + quick analysis + raw)
- Wrapper (`...203944...log`):
  - запуск выполнен с `--allow-runtime-enum-failure` и `--no-serial-test`;
  - snapshot записан в `...204015`, fallback-логи в `...204016`;
  - статус runtime enumeration: `normal`.
- Quick analysis (`...203944...quick_analysis.txt`):
  - подтверждает `Runtime USB enumeration status: normal`;
  - подтверждает видимость runtime-ID `2e8a:4006`;
  - фиксирует сообщения уровня SCSI/partition (`unable to read partition table`, `partition table beyond EOD`).
- Raw (`...204015`, `...204016`):
  - `lsusb_tree.txt` (`...204016`): `Class=Mass Storage, Driver=usb-storage`;
  - `journalctl_k_tail.txt`/`journalctl_k_tail_300.txt` и `kernel_usb_storage_scsi_sd.txt` (`...204016`):
    - `usb-storage ... USB Mass Storage device detected`;
    - `scsi host9: usb-storage ...`;
    - `Attached SCSI removable disk`;
    - при этом наблюдаются `unable to read partition table` и `partition table beyond EOD`.
- Признаков `BUG:`, `Oops`, `kernel panic`, `Call Trace`, `segfault` в предоставленных raw-логах не выявлено.

## 5. Классификация
`PARTIAL_ENUM`

Обоснование:
- USB-устройство перечисляется и привязывается к `usb-storage`;
- одновременно на блочном этапе фиксируются ошибки чтения таблицы разделов, что соответствует неполной/деградированной инициализации storage-пути;
- подтверждений падения драйвера, ядра или системы нет.

## 6. Уровень уверенности
Средний.

Причина: ключевые признаки (`runtime_enum_status=normal`, bind к `usb-storage`, ошибки partition-level) подтверждаются несколькими артефактами, но `journalctl_*` и агрегированные kernel-выгрузки включают события нескольких последовательных подключений в общей временной ленте.

## 7. Пробелы в доказательной базе
- В `...204015` отсутствуют `runtime_status.txt`, `lsusb_tree.txt`, `lsusb.txt` (основные runtime-метаданные присутствуют в fallback `...204016`).
- `dmesg` недоступен для пользователя в этом прогоне (`Operation not permitted`), поэтому анализ ядра опирается на `journalctl`-артефакты.
- Нет изолированного usbmon/pcap-трейса строго для одного подключения мутированного кейса.

## 8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|Writing snapshot to|Fallback logs directory" logs/runs/20260516_203944_051_msc_zero_block_size_01.log`
2. `cat results/raw/linux/051_msc_zero_block_size_01_20260516_204016/runtime_status.txt`
3. `rg -n "2e8a:4006|Class=Mass Storage|Driver=usb-storage" results/raw/linux/051_msc_zero_block_size_01_20260516_2040*/{lsusb.txt,lsusb_tree.txt,system_snapshot.txt}`
4. `rg -n "usb-storage|scsi host|Attached SCSI removable disk|unable to read partition table|partition table beyond EOD|FAT-fs" results/raw/linux/051_msc_zero_block_size_01_20260516_2040*/{journalctl_k_tail.txt,journalctl_k_tail_300.txt,kernel_usb_storage_scsi_sd.txt}`
5. `rg -n "BUG:|Oops|kernel panic|Call Trace|segfault" results/raw/linux/051_msc_zero_block_size_01_20260516_2040*/{journalctl_k_tail.txt,journalctl_k_tail_300.txt,dmesg_tail_300.txt,dmesg_full.txt}`
