# Результат эксперимента: 012_device_unknown_class_01

## 1. Цель кейса
Проверить реакцию Linux USB-стека на нетипичное значение `bDeviceClass` в дескрипторе устройства и определить, приводит ли это к отказу перечисления или деградации привязки драйвера.

## 2. Входные артефакты
- Case JSON: `experiments/cases/012_device_unknown_class.json`
- Wrapper log:
  - `logs/runs/20260516_050224_012_device_unknown_class_01.log`
- Quick analysis:
  - `logs/runs/20260516_050224_012_device_unknown_class_01.quick_analysis.txt`
- Raw Linux results:
  - `results/raw/linux/012_device_unknown_class_01_20260516_050252`
  - `results/raw/linux/012_device_unknown_class_01_20260516_050253`

## 3. Наблюдаемый результат runtime enumeration
- Wrapper зафиксировал: `Runtime USB enumeration status: normal` (`logs/runs/20260516_050224_012_device_unknown_class_01.log:101`).
- В raw-статусе: `runtime_enum_status=normal` (`results/raw/linux/012_device_unknown_class_01_20260516_050253/runtime_status.txt`).
- Runtime-устройство видно в `lsusb` и присутствует `ttyACM0`:
  - `ID 2e8a:4005` (`...050252/system_snapshot.txt`);
  - `/dev/ttyACM0` (`...050253/tty_devices.txt`).

Итог: runtime-перечисление успешно.

## 4. Linux evidence (wrapper, quick analysis, raw)
- Wrapper (`...050224...log`): последовательность BOOTSEL -> прошивка -> runtime завершилась `normal` без признаков аварийного завершения сценария.
- Quick analysis (`...050224...quick_analysis.txt`): подтверждает `Runtime USB enumeration status: normal` и наличие `2e8a:4005` в snapshot.
- Raw (`...050252`, `...050253`):
  - `lsusb_v_2e8a_4005.txt` показывает `bDeviceClass 170 [unknown]`, при этом интерфейсы `CDC Communications`/`CDC Data` распознаны;
  - `journalctl_k_tail*.txt` содержит штатную последовательность переподключения и последующую привязку `cdc_acm ... ttyACM0` (строка с `ttyACM0` на отметке времени `2026-05-16 05:02:52`);
  - признаков kernel panic/oops или падения драйвера в представленных логах нет.

## 5. Классификация
- Основная: `OK`.
- Дополнительная отметка: `KERNEL_WARNING` (в kernel log присутствует `FAT-fs ... unable to read boot sector to mark fs as dirty` в фазе переключения BOOTSEL -> runtime).

Что не подтверждено артефактами:
- `DRIVER_CRASH` — нет явных сообщений о краше драйвера;
- `SYSTEM_CRASH` — нет признаков panic/oops/перезагрузки хоста;
- `DRIVER_BIND_ERROR`/`EXPECTED_REJECT`/`PARTIAL_ENUM` — не подтверждаются, так как перечисление `normal` и `ttyACM0` создан.

## 6. Уровень уверенности
Средний.

Причина: совпадают независимые источники (`wrapper`, `quick analysis`, `runtime_status`, `lsusb`, `tty`), однако kernel tail включает события более широкого временного окна и частично смешивает несколько запусков.

## 7. Пробелы в доказательной базе
- `journalctl_k_tail*` не ограничен строго рамками одного запуска.
- В fallback-снимке `dmesg_tail.txt` пуст, а `dmesg` в wrapper недоступен по правам (`Операция не позволена`).
- Нет отдельной автоматической корреляции событий по точным `start/end` маркерам запуска внутри raw-папок.

## 8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|normal|Done" logs/runs/*012_device_unknown_class_01*.log`
2. `cat results/raw/linux/012_device_unknown_class_01_20260516_050253/runtime_status.txt`
3. `rg -n "bDeviceClass|bInterfaceClass|CDC|idVendor|idProduct" results/raw/linux/012_device_unknown_class_01_20260516_050252/lsusb_v_2e8a_4005.txt`
4. `rg -n "ttyACM0|FAT-fs|USB disconnect|panic|oops" results/raw/linux/012_device_unknown_class_01_20260516_050253/journalctl_k_tail.txt`
5. `bash -n tools/codex-analyze-log.sh`
