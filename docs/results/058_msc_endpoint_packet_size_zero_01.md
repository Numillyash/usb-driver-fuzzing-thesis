# Результат эксперимента: 058_msc_endpoint_packet_size_zero_01

## 1. Цель кейса
Проверить реакцию Linux USB/MSC-стека на descriptor-level мутацию для Mass Storage: у bulk endpoint в конфигурационном дескрипторе задан `wMaxPacketSize=0`, и оценить итог перечисления/привязки драйвера без выхода за безопасный inert/read-only профиль.

## 2. Входные артефакты
- Конфигурация кейса: `experiments/cases/058_msc_endpoint_packet_size_zero.json`
- Wrapper log:
  - `logs/runs/20260517_003125_058_msc_endpoint_packet_size_zero_01.log`
- Quick analysis:
  - `logs/runs/20260517_003125_058_msc_endpoint_packet_size_zero_01.quick_analysis.txt`
- Raw Linux results:
  - `results/raw/linux/058_msc_endpoint_packet_size_zero_01_20260517_003153`

## 3. Наблюдаемый результат runtime enumeration
- В wrapper-логе: `Runtime USB enumeration status: normal`.
- В raw snapshot: `runtime_status.txt` содержит `runtime_enum_status=normal`.
- В `lsusb_tree.txt` устройство присутствует как `Class=Mass Storage, Driver=usb-storage`.

Итог: runtime-enumeration и привязка `usb-storage` произошли успешно.

## 4. Linux evidence (wrapper + quick analysis + raw)
- Wrapper (`...003125...log`):
  - запуск с `--no-serial-test --allow-runtime-enum-failure`;
  - runtime-статус `normal`;
  - snapshot: `058_msc_endpoint_packet_size_zero_01_20260517_003153`.
- Quick analysis (`...003125...quick_analysis.txt`):
  - подтверждает `Runtime USB enumeration status: normal`;
  - подтверждает наличие артефакта `runtime_status.txt` с `runtime_enum_status=normal`;
  - фиксирует в `lsusb_v_2e8a_4006.txt` две строки `wMaxPacketSize     0x0000  1x 0 bytes`.
- Raw (`...003153`):
  - `lsusb_v_2e8a_4006.txt`: `wMaxPacketSize=0x0000` у bulk endpoint-ов;
  - `lsusb_tree.txt`: `Class=Mass Storage, Driver=usb-storage`;
  - `journalctl_k_tail.txt`/`journalctl_k_tail_300.txt` в окне запуска (`00:31:36`–`00:31:42`) показывают детект `usb-storage`, создание `sdb/sdb1` и предупреждение `FAT-fs (sdb1): unable to read boot sector to mark fs as dirty`;
  - явных сигнатур `BUG:`, `Oops`, `kernel panic`, `Call Trace`, `segfault` в предоставленных артефактах нет.

## 5. Классификация
`KERNEL_WARNING`

Обоснование:
- Linux принял устройство и выполнил bind (`runtime_enum_status=normal`, `Driver=usb-storage`), поэтому это не `EXPECTED_REJECT` и не `DRIVER_BIND_ERROR`;
- одновременно в пределах запуска зафиксировано kernel-level предупреждение/ошибка файловой подсистемы для `sdb1` (`FAT-fs ... unable to read boot sector ...`), что соответствует `KERNEL_WARNING`;
- признаков `DRIVER_CRASH`/`SYSTEM_CRASH` нет.

## 6. Уровень уверенности
Средний.

Причина: ключевые признаки (runtime status, bind, `wMaxPacketSize=0`) подтверждаются напрямую, но `journalctl`-хвосты включают события соседних запусков, что ограничивает идеальную изоляцию только одного цикла.

## 7. Пробелы в доказательной базе
- В `journalctl_k_tail*` присутствуют записи от предыдущих прогонов; нужен более узкий временной срез только на интервал конкретного запуска.
- `dmesg` в wrapper-сценарии недоступен непривилегированному пользователю (`Операция не позволена`), поэтому часть kernel-контекста получена только через `journalctl`.
- Нет usbmon/pcap-трейса control/bulk обмена для пакетной верификации реакции хоста именно на `wMaxPacketSize=0`.

## 8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|runtime_enum_status|allow-runtime-enum-failure|no-serial-test" logs/runs/20260517_003125_058_msc_endpoint_packet_size_zero_01.log results/raw/linux/058_msc_endpoint_packet_size_zero_01_20260517_003153/runtime_status.txt`
2. `rg -n "wMaxPacketSize|bEndpointAddress|Endpoint Descriptor" results/raw/linux/058_msc_endpoint_packet_size_zero_01_20260517_003153/lsusb_v_2e8a_4006.txt`
3. `rg -n "Class=Mass Storage|Driver=usb-storage|2e8a:4006" results/raw/linux/058_msc_endpoint_packet_size_zero_01_20260517_003153/{lsusb.txt,lsusb_tree.txt,system_snapshot.txt}`
4. `rg -n "00:31:3|00:31:4|00:31:5|usb-storage 1-4:1.0|scsi host9|FAT-fs|I/O error|unable to read partition table" results/raw/linux/058_msc_endpoint_packet_size_zero_01_20260517_003153/{journalctl_k_tail.txt,journalctl_k_tail_300.txt,kernel_usb_storage_scsi_sd.txt}`
5. `rg -n "BUG:|Oops|kernel panic|Call Trace|segfault|general protection fault|soft lockup|hard lockup" logs/runs/20260517_003125_058_msc_endpoint_packet_size_zero_01.log logs/runs/20260517_003125_058_msc_endpoint_packet_size_zero_01.quick_analysis.txt results/raw/linux/058_msc_endpoint_packet_size_zero_01_20260517_003153/*`
