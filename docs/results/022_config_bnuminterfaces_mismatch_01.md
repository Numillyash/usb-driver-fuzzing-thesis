# Результат эксперимента: 022_config_bnuminterfaces_mismatch_01

## 1. Цель кейса
Проверить реакцию Linux USB-стека на рассогласование `bNumInterfaces` в configuration descriptor и определить, приводит ли это к отказу перечисления, частичному перечислению или сохраняется штатная инициализация runtime-устройства.

## 2. Входные артефакты
- Case config:
  - `experiments/cases/022_config_bnuminterfaces_mismatch.json`
- Wrapper log:
  - `logs/runs/20260516_170237_022_config_bnuminterfaces_mismatch_01.log`
- Quick analysis:
  - `logs/runs/20260516_170237_022_config_bnuminterfaces_mismatch_01.quick_analysis.txt`
- Raw Linux results:
  - `results/raw/linux/022_config_bnuminterfaces_mismatch_01_20260516_170308`

## 3. Наблюдаемый результат runtime enumeration
- В wrapper-заезде зафиксировано: `Runtime USB enumeration status: normal` (`...170237...log`).
- В raw-статусе подтверждено: `runtime_enum_status=normal` (`...170308/runtime_status.txt`).
- Runtime-устройство присутствует в USB-списке: `ID 2e8a:4005` (`...170308/lsusb.txt`, `...170308/system_snapshot.txt`).
- Создан последовательный интерфейс: `/dev/ttyACM0` (`...170308/tty_devices.txt`).

Итог: в данном прогоне runtime-перечисление на Linux прошло штатно.

## 4. Linux evidence (wrapper + quick analysis + raw)
- Wrapper (`...170237...log`): полный цикл BOOTSEL -> прошивка -> runtime завершен со статусом `normal`; в runtime видны `2e8a:4005` и `/dev/ttyACM0`.
- Quick analysis (`...170237...quick_analysis.txt`): повторно фиксирует `Runtime USB enumeration status: normal` и наличие runtime USB ID `2e8a:4005` в snapshot-артефактах.
- Raw (`...170308`):
  - `runtime_status.txt`: `runtime_enum_status=normal`;
  - `lsusb.txt` и `system_snapshot.txt`: устройство `2e8a:4005` присутствует;
  - `tty_devices.txt`: присутствует `/dev/ttyACM0`;
  - `journalctl_k_tail.txt`: есть события успешного перечисления runtime-устройства (`idVendor=2e8a`, `idProduct=4005`) и авторизации устройства;
  - явных признаков `kernel panic`, `oops`, `BUG:` или падения хоста в представленных логах нет.

## 5. Классификация
- Основная: `OK`.

Обоснование:
- зафиксирован `runtime_enum_status=normal`;
- runtime-устройство и CDC-интерфейс присутствуют в артефактах запуска;
- отсутствуют подтвержденные признаки `DRIVER_CRASH` или `SYSTEM_CRASH`.

## 6. Уровень уверенности
Средне-высокий.

Причина: наблюдения из `wrapper`, `quick_analysis`, `runtime_status`, `lsusb`, `tty`, `journalctl` согласуются и указывают на штатное перечисление в этом запуске.

## 7. Пробелы в доказательной базе
- В `journalctl_k_tail*` присутствуют события нескольких запусков, поэтому интерпретация требует привязки к временным меткам конкретного прогона.
- В wrapper fallback `dmesg` недоступен по правам (`Операция не позволена`), часть kernel-контекста недоступна.
- В доступных артефактах нет отдельного низкоуровневого control-transfer/pcap-трейса, напрямую подтверждающего, как именно хост обработал именно mismatch `bNumInterfaces`.
- По `lsusb_v_2e8a_4005.txt` в snapshot отображается `bNumInterfaces=2`; без дополнительной трассировки нельзя утверждать, что аномалия поля была наблюдаема хостом именно в этом заезде.

## 8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|runtime_enum_status" logs/runs/*022_config_bnuminterfaces_mismatch_01*.log results/raw/linux/022_config_bnuminterfaces_mismatch_01_20260516_170308/runtime_status.txt`
2. `rg -n "2e8a:4005|ttyACM0" results/raw/linux/022_config_bnuminterfaces_mismatch_01_20260516_170308/{lsusb.txt,system_snapshot.txt,tty_devices.txt,journalctl_k_tail.txt,journalctl_k_tail_300.txt}`
3. `rg -n "panic|oops|BUG:|segfault|crash" results/raw/linux/022_config_bnuminterfaces_mismatch_01_20260516_170308/journalctl_k_tail.txt results/raw/linux/022_config_bnuminterfaces_mismatch_01_20260516_170308/journalctl_k_tail_300.txt`
4. `sed -n '1,140p' results/raw/linux/022_config_bnuminterfaces_mismatch_01_20260516_170308/lsusb_v_2e8a_4005.txt`
