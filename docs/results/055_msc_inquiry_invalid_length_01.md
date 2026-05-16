# Результат эксперимента: 055_msc_inquiry_invalid_length_01

## 1. Цель кейса
Проверить реакцию Linux USB/MSC-стека на безопасную мутацию `Mass Storage Class`: некорректная длина ответа на SCSI `INQUIRY` при валидных USB-дескрипторах и контролируемом inert/read-only профиле.

## 2. Входные артефакты
- Конфигурация кейса: `experiments/cases/055_msc_inquiry_invalid_length.json`
- Wrapper log:
  - `logs/runs/20260516_223923_055_msc_inquiry_invalid_length_01.log`
- Quick analysis:
  - `logs/runs/20260516_223923_055_msc_inquiry_invalid_length_01.quick_analysis.txt`
- Raw Linux results:
  - `results/raw/linux/055_msc_inquiry_invalid_length_01_20260516_223951`

## 3. Наблюдаемый результат runtime enumeration
- В wrapper-логе: `Runtime USB enumeration status: normal`.
- В `runtime_status.txt`: `runtime_enum_status=normal`.
- В snapshot устройство видно как `2e8a:4006` (`usb_case_msc_demo MSC baseline`), в `lsusb_tree.txt` интерфейс: `Class=Mass Storage, Driver=usb-storage`.

Итог: USB-перечисление успешно, bind к `usb-storage` наблюдается.

## 4. Linux evidence (wrapper + quick analysis + raw)
- Wrapper (`...223923...log`):
  - запуск с `--no-serial-test --allow-runtime-enum-failure`;
  - `Runtime USB enumeration status: normal`;
  - snapshot/fallback: `.../055_msc_inquiry_invalid_length_01_20260516_223951`;
  - `dmesg` недоступен непривилегированному пользователю (`Операция не позволена`).
- Quick analysis (`...223923...quick_analysis.txt`):
  - подтверждает `Runtime USB enumeration status: normal`;
  - подтверждает тот же каталог raw-результатов;
  - фиксирует в kernel-логе события для текущего временного окна `22:39:42..22:39:50`: обнаружение `2e8a:4006` и `usb-storage ... detected`.
- Raw (`...223951`):
  - `lsusb.txt` / `system_snapshot.txt` / `lsusb_v_2e8a_4006.txt`: устройство `2e8a:4006` присутствует;
  - `lsusb_tree.txt`: `Class=Mass Storage, Driver=usb-storage`;
  - `journalctl_k_tail*.txt`: в конце окна есть повторное обнаружение runtime-устройства и `usb-storage` bind;
  - в этом же наборе есть ошибки `FAT-fs ... unable to read boot sector`, `unable to read partition table`, `I/O error`, но журнал агрегированный и содержит события нескольких запусков/кейсов, а не изолированный single-shot только для `055`.
- По шаблонам `BUG:`, `Oops`, `kernel panic`, `Call Trace`, `segfault` подтверждений аварии ядра в артефактах нет.

## 5. Классификация
`PARTIAL_ENUM`

Обоснование:
- подтверждены успешные USB enumeration и bind к `usb-storage`;
- для текущего запуска нет чистого изолированного доказательства устойчивого завершения SCSI/блочной инициализации (в `lsblk_f.txt` виден только `sda`, а kernel-журнал смешан с соседними прогонами);
- признаков `DRIVER_CRASH`/`SYSTEM_CRASH` нет.

## 6. Уровень уверенности
Средний.

Причина: ключевые сигналы согласованы между wrapper/quick-analysis/raw, но kernel-свидетельства не изолированы строго одним циклом именно кейса `055`.

## 7. Пробелы в доказательной базе
- `dmesg` недоступен в прогоне непривилегированному пользователю; анализ опирается на `journalctl`.
- `journalctl_k_tail*.txt` и `kernel_usb_storage_scsi_sd.txt` агрегируют несколько последовательных подключений и разные кейсы.
- Нет отдельного usbmon/pcap-трейса или выделенного журнала только для одного цикла `055_msc_inquiry_invalid_length_01`.

## 8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|Writing snapshot to|Fallback logs directory|allow-runtime-enum-failure|no-serial-test|dmesg: чтение буфера ядра" logs/runs/20260516_223923_055_msc_inquiry_invalid_length_01.log logs/runs/20260516_223923_055_msc_inquiry_invalid_length_01.quick_analysis.txt`
2. `cat results/raw/linux/055_msc_inquiry_invalid_length_01_20260516_223951/runtime_status.txt`
3. `rg -n "2e8a:4006|usb_case_msc_demo MSC baseline|Class=Mass Storage|Driver=usb-storage" results/raw/linux/055_msc_inquiry_invalid_length_01_20260516_223951/{lsusb.txt,lsusb_tree.txt,system_snapshot.txt,lsusb_v_2e8a_4006.txt}`
4. `rg -n "22:39:42|22:39:50|usb-storage 1-4:1.0|scsi host9|Direct-Access|Attached SCSI removable disk|unable to read partition table|FAT-fs|I/O error" results/raw/linux/055_msc_inquiry_invalid_length_01_20260516_223951/{journalctl_k_tail.txt,journalctl_k_tail_300.txt,kernel_usb_storage_scsi_sd.txt}`
5. `rg -n "BUG:|Oops|kernel panic|Call Trace|segfault|general protection fault|soft lockup|hard lockup" results/raw/linux/055_msc_inquiry_invalid_length_01_20260516_223951/{journalctl_k_tail.txt,journalctl_k_tail_300.txt,kernel_usb_storage_scsi_sd.txt,dmesg_tail_300.txt,dmesg_full.txt}`
