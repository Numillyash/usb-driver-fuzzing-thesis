# Результат эксперимента: 057_msc_stall_on_read10_01

## 1. Цель кейса
Проверить реакцию Linux USB/MSC-стека на безопасную мутацию `READ(10)` в BOT-потоке: устройство остаётся inert/read-only, а чтение моделируется как controlled failure (без записи и без выполнения команд на хосте).

## 2. Входные артефакты
- Конфигурация кейса: `experiments/cases/057_msc_stall_on_read10.json`
- Wrapper log:
  - `logs/runs/20260516_234739_057_msc_stall_on_read10_01.log`
- Quick analysis:
  - `logs/runs/20260516_234739_057_msc_stall_on_read10_01.quick_analysis.txt`
- Raw Linux results:
  - `results/raw/linux/057_msc_stall_on_read10_01_20260516_234807`
  - `results/raw/linux/057_msc_stall_on_read10_01_20260516_234808`

## 3. Наблюдаемый результат runtime enumeration
- В wrapper-логе: `Runtime USB enumeration status: normal`.
- В fallback raw: `runtime_status.txt` содержит `runtime_enum_status=normal`.
- В raw-снимках устройство видно как `2e8a:4006`; в `lsusb_tree.txt` есть `Class=Mass Storage, Driver=usb-storage`.

Итог: runtime-enumeration успешен, bind к `usb-storage` подтверждён.

## 4. Linux evidence (wrapper + quick analysis + raw)
- Wrapper (`...234739...log`):
  - запуск с `--no-serial-test --allow-runtime-enum-failure`;
  - runtime-статус: `normal`;
  - основной snapshot: `...234807`, fallback: `...234808`;
  - чтение `dmesg` недоступно непривилегированному пользователю (`Операция не позволена`).
- Quick analysis (`...234739...quick_analysis.txt`):
  - подтверждает `Runtime USB enumeration status: normal`;
  - подтверждает наличие `2e8a:4006` и bind `usb-storage`;
  - фиксирует ошибки чтения блочного уровня (`DID_ERROR`, `I/O error`, `unable to read partition table`, `FAT-fs ... unable to read boot sector ...`).
- Raw evidence:
  - `...234808/runtime_status.txt`: `runtime_enum_status=normal`;
  - `...234808/lsusb.txt`, `...234807/system_snapshot.txt`, `...234807/lsusb_v_2e8a_4006.txt`: устройство `2e8a:4006` присутствует;
  - `...234808/lsusb_tree.txt`: `Class=Mass Storage, Driver=usb-storage`;
  - `...234808/kernel_usb_storage_scsi_sd.txt`, `journalctl_k_tail*.txt`: repeated `usb-storage` detection + ошибки `READ`/partition/FAT уровня.
- По сигнатурам `BUG:`, `Oops`, `kernel panic`, `Call Trace`, `segfault` совпадений нет.

## 5. Классификация
`KERNEL_WARNING`

Обоснование:
- enumeration и driver bind успешны (`normal`, `usb-storage`);
- одновременно есть kernel-level warning/error сигналы в стеке хранения (`DID_ERROR`, `I/O error`, FAT/partition warnings);
- подтверждений `DRIVER_CRASH` и `SYSTEM_CRASH` нет.

## 6. Уровень уверенности
Средний.

Причина: ключевые индикаторы по запуску `057` согласованы, но `journalctl`/`kernel_usb_storage_scsi_sd` содержат события нескольких близких прогонов, что ограничивает строгую одношаговую изоляцию именно этой итерации.

## 7. Пробелы в доказательной базе
- Нет привилегированного `dmesg` для данного запуска; доступны только `journalctl`-срезы и fallback-артефакты.
- Каталог `...234807` не содержит `runtime_status.txt` (он есть в `...234808`).
- Kernel tail-логи агрегируют соседние подключения/переподключения, а не только один цикл `057`.
- Нет usbmon/pcap-трейса BOT-команд для пакетного подтверждения точного момента `READ(10)` failure.

## 8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|Writing snapshot to|Fallback logs directory|allow-runtime-enum-failure|no-serial-test|dmesg: чтение буфера ядра" logs/runs/20260516_234739_057_msc_stall_on_read10_01.log logs/runs/20260516_234739_057_msc_stall_on_read10_01.quick_analysis.txt`
2. `cat results/raw/linux/057_msc_stall_on_read10_01_20260516_234808/runtime_status.txt`
3. `rg -n "2e8a:4006|usb_case_msc_demo MSC baseline|Class=Mass Storage|Driver=usb-storage" results/raw/linux/057_msc_stall_on_read10_01_20260516_234807/{system_snapshot.txt,lsusb_v_2e8a_4006.txt} results/raw/linux/057_msc_stall_on_read10_01_20260516_234808/{lsusb.txt,lsusb_tree.txt}`
4. `rg -n "23:47:5|23:48:0|usb-storage 1-4:1.0|scsi host9|DID_ERROR|I/O error|unable to read partition table|FAT-fs" results/raw/linux/057_msc_stall_on_read10_01_20260516_234808/{journalctl_k_tail.txt,journalctl_k_tail_300.txt,kernel_usb_storage_scsi_sd.txt}`
5. `rg -n "BUG:|Oops|kernel panic|Call Trace|segfault|general protection fault|soft lockup|hard lockup" results/raw/linux/057_msc_stall_on_read10_01_20260516_234807/* results/raw/linux/057_msc_stall_on_read10_01_20260516_234808/* logs/runs/20260516_234739_057_msc_stall_on_read10_01.log logs/runs/20260516_234739_057_msc_stall_on_read10_01.quick_analysis.txt`
