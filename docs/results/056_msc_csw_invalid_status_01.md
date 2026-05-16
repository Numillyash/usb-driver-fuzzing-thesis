# Результат эксперимента: 056_msc_csw_invalid_status_01

## 1. Цель кейса
Проверить реакцию Linux USB/MSC-стека на безопасную мутацию BOT/CSW: контролируемый путь «некорректного/failed статуса команды» при валидных USB-дескрипторах и inert/read-only профиле устройства.

## 2. Входные артефакты
- Конфигурация кейса: `experiments/cases/056_msc_csw_invalid_status.json`
- Wrapper log:
  - `logs/runs/20260516_231626_056_msc_csw_invalid_status_01.log`
- Quick analysis:
  - `logs/runs/20260516_231626_056_msc_csw_invalid_status_01.quick_analysis.txt`
- Raw Linux results:
  - `results/raw/linux/056_msc_csw_invalid_status_01_20260516_231654`

## 3. Наблюдаемый результат runtime enumeration
- В wrapper-логе: `Runtime USB enumeration status: normal`.
- В `runtime_status.txt`: `runtime_enum_status=normal`.
- В raw snapshot устройство присутствует как `2e8a:4006`, а в `lsusb_tree.txt` интерфейс определён как `Class=Mass Storage, Driver=usb-storage`.

Итог: USB runtime-enumeration успешен, bind к `usb-storage` подтверждён.

## 4. Linux evidence (wrapper + quick analysis + raw)
- Wrapper (`...231626...log`):
  - запуск с `--no-serial-test --allow-runtime-enum-failure`;
  - статус runtime: `normal`;
  - snapshot: `.../056_msc_csw_invalid_status_01_20260516_231654`;
  - `dmesg` недоступен непривилегированному пользователю (`Операция не позволена`).
- Quick analysis (`...231626...quick_analysis.txt`):
  - подтверждает `Runtime USB enumeration status: normal`;
  - подтверждает путь raw-результатов;
  - фиксирует присутствие `2e8a:4006` и события `usb-storage` в kernel-выгрузках.
- Raw (`...231654`):
  - `runtime_status.txt`: `runtime_enum_status=normal`;
  - `lsusb.txt` / `system_snapshot.txt` / `lsusb_v_2e8a_4006.txt`: устройство `2e8a:4006` наблюдается;
  - `lsusb_tree.txt`: `Class=Mass Storage, Driver=usb-storage`;
  - `journalctl_k_tail.txt` и `kernel_usb_storage_scsi_sd.txt` (в окне запуска около `23:16:37..23:16:53`) содержат:
    - обнаружение USB-устройства и `usb-storage` bind;
    - события блочного уровня (`sd ... [sdb]`, `sdb1`);
    - предупреждения/ошибки I/O уровня ФС и носителя (`FAT-fs ... unable to read boot sector`, `unable to read partition table`).
- По сигнатурам `BUG:`, `Oops`, `kernel panic`, `Call Trace`, `segfault` подтверждений аварии ядра не найдено.

## 5. Классификация
`KERNEL_WARNING`

Обоснование:
- USB enumeration и bind драйвера успешны (`normal`, `usb-storage`);
- при этом в kernel evidence присутствуют предупреждения/ошибки уровня хранения (FAT/partition/I/O) в окне данного прогона;
- подтверждений `DRIVER_CRASH` или `SYSTEM_CRASH` нет.

## 6. Уровень уверенности
Средний.

Причина: ключевые сигналы по запуску `056` согласованы, но kernel-tail файлы агрегируют и соседние подключения, поэтому причинно-следственная изоляция только одного цикла ограничена.

## 7. Пробелы в доказательной базе
- `dmesg` нечитабелен в непривилегированном запуске; анализ основан на `journalctl`-срезах.
- `journalctl_k_tail*.txt` и `kernel_usb_storage_scsi_sd.txt` содержат события нескольких последовательных прогонов, а не строго single-shot лог только для `056`.
- Нет отдельного usbmon/pcap-трейса BOT/CSW обмена для прямого подтверждения конкретной CSW-аномалии на уровне пакетов.

## 8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|Writing snapshot to|Fallback logs directory|allow-runtime-enum-failure|no-serial-test|dmesg: чтение буфера ядра" logs/runs/20260516_231626_056_msc_csw_invalid_status_01.log logs/runs/20260516_231626_056_msc_csw_invalid_status_01.quick_analysis.txt`
2. `cat results/raw/linux/056_msc_csw_invalid_status_01_20260516_231654/runtime_status.txt`
3. `rg -n "2e8a:4006|usb_case_msc_demo MSC baseline|Class=Mass Storage|Driver=usb-storage" results/raw/linux/056_msc_csw_invalid_status_01_20260516_231654/{lsusb.txt,lsusb_tree.txt,system_snapshot.txt,lsusb_v_2e8a_4006.txt}`
4. `rg -n "23:16:3|23:16:4|23:16:5|usb-storage 1-4:1.0|scsi host9|sdb1|unable to read partition table|FAT-fs|I/O error|DID_ERROR" results/raw/linux/056_msc_csw_invalid_status_01_20260516_231654/{journalctl_k_tail.txt,journalctl_k_tail_300.txt,kernel_usb_storage_scsi_sd.txt}`
5. `rg -n "BUG:|Oops|kernel panic|Call Trace|segfault|general protection fault|soft lockup|hard lockup" results/raw/linux/056_msc_csw_invalid_status_01_20260516_231654/{journalctl_k_tail.txt,journalctl_k_tail_300.txt,kernel_usb_storage_scsi_sd.txt,dmesg_tail_300.txt,dmesg_full.txt}`
