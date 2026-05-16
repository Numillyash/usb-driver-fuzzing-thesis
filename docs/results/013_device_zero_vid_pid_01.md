# Результат эксперимента: 013_device_zero_vid_pid_01

## 1. Цель кейса
Проверить реакцию Linux USB-стека на нулевые идентификаторы `idVendor=0x0000` и `idProduct=0x0000` в device descriptor и определить, приводит ли это к полному отказу, частичному перечислению или штатной привязке CDC-драйвера.

## 2. Входные артефакты
- Case JSON: `experiments/cases/013_device_zero_vid_pid.json`
- Wrapper log:
  - `logs/runs/20260516_050744_013_device_zero_vid_pid_01.log`
- Quick analysis:
  - `logs/runs/20260516_050744_013_device_zero_vid_pid_01.quick_analysis.txt`
- Raw Linux results:
  - `results/raw/linux/013_device_zero_vid_pid_01_20260516_050813`

## 3. Наблюдаемый результат runtime enumeration
- Wrapper зафиксировал: `Runtime USB enumeration status: partial` (`logs/runs/20260516_050744_013_device_zero_vid_pid_01.log`).
- Raw-статус подтверждает: `runtime_enum_status=partial` (`results/raw/linux/013_device_zero_vid_pid_01_20260516_050813/runtime_status.txt`).
- Устройство с нулевыми VID/PID видно в USB-списке:
  - `Bus 001 Device 042: ID 0000:0000 ...` (`.../lsusb.txt`, `.../system_snapshot.txt`).

Итог: Linux принимает устройство на уровне базового перечисления (оно появляется в `lsusb`), но результат запуска отмечен как частичный.

## 4. Linux evidence (wrapper, quick analysis, raw)
- Wrapper (`...050744...log`): полный цикл BOOTSEL -> прошивка -> runtime завершился с `partial`, без признаков аварийного завершения сценария.
- Quick analysis (`...050744...quick_analysis.txt`): фиксирует `Runtime USB enumeration status: partial` и наличие `ID 0000:0000` в snapshot.
- Raw (`...050813`):
  - `lsusb_v_0000_0000.txt` показывает корректно прочитанный descriptor с `idVendor 0x0000`, `idProduct 0x0000`, `bNumConfigurations 1`, интерфейсами CDC;
  - `journalctl_k_tail.txt` содержит событие обнаружения устройства с `idVendor=0000, idProduct=0000` (время `2026-05-16 05:08:05`);
  - `tty_devices.txt` не содержит `ttyACM0` (ошибка `Нет такого файла или каталога`).

## 5. Классификация
- Основная: `PARTIAL_ENUM`.

Обоснование:
- устройство с `0000:0000` обнаружено и дескрипторы читаются;
- при этом итоговый runtime-статус явно `partial`, и в snapshot нет подтверждения доступного `ttyACM*`.

Что не подтверждено артефактами:
- `DRIVER_CRASH` — нет явных сообщений о краше драйвера;
- `SYSTEM_CRASH` — нет признаков kernel panic/oops/перезагрузки хоста;
- `DRIVER_BIND_ERROR` — есть неполная привязка, но нет явного сообщения о падении конкретного драйвера;
- `EXPECTED_REJECT` — полного отклонения устройства нет, так как `0000:0000` присутствует в `lsusb`.

## 6. Уровень уверенности
Средний.

Причина: `wrapper`, `quick_analysis`, `runtime_status`, `lsusb` и `journalctl` согласованно указывают на частичное перечисление; при этом отсутствует отдельный узкий лог только для конкретного момента bind/не-bind CDC в рамках одного короткого окна.

## 7. Пробелы в доказательной базе
- В snapshot отсутствует прямое подтверждение успешной привязки `cdc_acm` именно для устройства `0000:0000`.
- `journalctl_k_tail*` включает события нескольких запусков и требует ручной фильтрации по времени.
- Fallback `dmesg` в wrapper недоступен по правам (`Операция не позволена`), поэтому часть kernel-контекста ограничена.

## 8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|partial" logs/runs/*013_device_zero_vid_pid_01*.log`
2. `cat results/raw/linux/013_device_zero_vid_pid_01_20260516_050813/runtime_status.txt`
3. `rg -n "0000:0000|idVendor=0000|idProduct=0000" results/raw/linux/013_device_zero_vid_pid_01_20260516_050813/{lsusb.txt,system_snapshot.txt,journalctl_k_tail.txt}`
4. `rg -n "ttyACM|cdc_acm|Device is not authorized|USB disconnect" results/raw/linux/013_device_zero_vid_pid_01_20260516_050813/journalctl_k_tail.txt results/raw/linux/013_device_zero_vid_pid_01_20260516_050813/tty_devices.txt`
5. `bash -n tools/codex-analyze-log.sh`
