# Результат эксперимента: 054_msc_read_capacity_short_response_01

## 1. Цель кейса
Проверить реакцию Linux USB/MSC-стека на безопасную мутацию `Mass Storage Class`, где при валидных дескрипторах и inert/read-only профиле команда `READ CAPACITY` моделируется как контролируемый укороченный/некорректный ответ на уровне class-протокола.

## 2. Входные артефакты
- Конфигурация кейса: `experiments/cases/054_msc_read_capacity_short_response.json`
- Wrapper log:
  - `logs/runs/20260516_221627_054_msc_read_capacity_short_response_01.log`
- Quick analysis:
  - `logs/runs/20260516_221627_054_msc_read_capacity_short_response_01.quick_analysis.txt`
- Raw Linux results:
  - `results/raw/linux/054_msc_read_capacity_short_response_01_20260516_221654`
  - `results/raw/linux/054_msc_read_capacity_short_response_01_20260516_221655`

## 3. Наблюдаемый результат runtime enumeration
- В wrapper-логе: `Runtime USB enumeration status: normal`.
- В `runtime_status.txt` (fallback snapshot): `runtime_enum_status=normal`.
- Устройство наблюдается как `2e8a:4006` (`usb_case_msc_demo MSC baseline`), в дереве USB указано `Class=Mass Storage, Driver=usb-storage`.

Итог: USB-перечисление и bind к `usb-storage` выполнены.

## 4. Linux evidence (wrapper + quick analysis + raw)
- Wrapper (`...221627...log`):
  - запуск с `--no-serial-test --allow-runtime-enum-failure`;
  - `Runtime USB enumeration status: normal`;
  - snapshot: `...221654`, fallback: `...221655`;
  - `dmesg` недоступен непривилегированному пользователю (`Операция не позволена`).
- Quick analysis (`...221627...quick_analysis.txt`):
  - подтверждает `Runtime USB enumeration status: normal`;
  - подтверждает те же каталоги snapshot/fallback.
- Raw (`...221655` и `...221654`):
  - `lsusb.txt` / `system_snapshot.txt` / `lsusb_v_2e8a_4006.txt`: устройство `2e8a:4006` присутствует;
  - `lsusb_tree.txt`: `Class=Mass Storage, Driver=usb-storage`;
  - `journalctl_k_tail.txt`, `journalctl_k_tail_300.txt`, `kernel_usb_storage_scsi_sd.txt` содержат ошибки на уровне SCSI/блочного слоя, включая:
    - `Read Capacity(10) failed: Result: hostbyte=DID_ERROR driverbyte=DRIVER_OK`;
    - `I/O error, dev sdb, sector ...`;
    - `Buffer I/O error on dev sdb/sdb1 ...`;
    - `sdb: unable to read partition table`;
    - `FAT-fs (sdb1): unable to read boot sector to mark fs as dirty`.
- По шаблонам `BUG:`, `Oops`, `kernel panic`, `Call Trace`, `segfault` совпадений в предоставленных логах не найдено.

## 5. Классификация
`PARTIAL_ENUM`

Обоснование:
- устройство успешно перечисляется и привязывается к `usb-storage`;
- дальнейшая работа носителя нестабильна (повторяющиеся I/O/SCSI ошибки при чтении и разметке);
- доказательств `DRIVER_CRASH` и `SYSTEM_CRASH` нет.

## 6. Уровень уверенности
Средний.

Причина: признаки согласованы между wrapper, quick analysis и raw-артефактами, но kernel-журнал содержит агрегированные события нескольких подключений/прогонов.

## 7. Пробелы в доказательной базе
- `dmesg` для пользователя ограничен, анализ ядра опирается на `journalctl`.
- В `journalctl_*` и `kernel_usb_storage_scsi_sd.txt` присутствуют события не только одного момента запуска, а нескольких циклов переподключения.
- Нет отдельного usbmon/pcap-трейса строго для одного цикла именно этого кейса.

## 8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|Writing snapshot to|Fallback logs directory|allow-runtime-enum-failure|no-serial-test|dmesg: чтение буфера ядра" logs/runs/20260516_221627_054_msc_read_capacity_short_response_01.log logs/runs/20260516_221627_054_msc_read_capacity_short_response_01.quick_analysis.txt`
2. `cat results/raw/linux/054_msc_read_capacity_short_response_01_20260516_221655/runtime_status.txt`
3. `rg -n "2e8a:4006|usb_case_msc_demo MSC baseline|Class=Mass Storage|Driver=usb-storage" results/raw/linux/054_msc_read_capacity_short_response_01_20260516_221655/{lsusb.txt,lsusb_tree.txt} results/raw/linux/054_msc_read_capacity_short_response_01_20260516_221654/{system_snapshot.txt,lsusb_v_2e8a_4006.txt}`
4. `rg -n "Read Capacity\(10\) failed|DID_ERROR|DID_OK|Sense Key|Medium not present|I/O error, dev sdb|Buffer I/O error|unable to read partition table|FAT-fs \(sdb1\): unable to read boot sector" results/raw/linux/054_msc_read_capacity_short_response_01_20260516_221655/{journalctl_k_tail.txt,journalctl_k_tail_300.txt,kernel_usb_storage_scsi_sd.txt} results/raw/linux/054_msc_read_capacity_short_response_01_20260516_221654/journalctl_k_tail_300.txt`
5. `rg -n "BUG:|Oops|kernel panic|Call Trace|segfault|general protection fault|soft lockup|hard lockup" results/raw/linux/054_msc_read_capacity_short_response_01_20260516_221655/{journalctl_k_tail.txt,journalctl_k_tail_300.txt,kernel_usb_storage_scsi_sd.txt} results/raw/linux/054_msc_read_capacity_short_response_01_20260516_221654/{journalctl_k_tail_300.txt,dmesg_tail_300.txt,dmesg_full.txt}`
