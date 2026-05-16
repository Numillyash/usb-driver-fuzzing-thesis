# Результат эксперимента: 030_interface_class_invalid_01

## 1. Цель кейса
Проверить реакцию Linux USB-стека на невалидное значение `bInterfaceClass` у одного из интерфейсов композитного устройства и определить, возникает ли отказ привязки драйвера, частичное перечисление или штатная инициализация.

## 2. Входные артефакты
- Case config: `experiments/cases/030_interface_class_invalid.json`
- Wrapper log: `logs/runs/20260516_171427_030_interface_class_invalid_01.log`
- Quick analysis: `logs/runs/20260516_171427_030_interface_class_invalid_01.quick_analysis.txt`
- Raw Linux results: `results/raw/linux/030_interface_class_invalid_01_20260516_171458`

## 3. Наблюдаемый результат runtime enumeration
- В wrapper-логе зафиксировано: `Runtime USB enumeration status: normal`.
- В raw-статусе подтверждено: `runtime_enum_status=normal` (`runtime_status.txt`).
- Runtime-устройство присутствует в `lsusb`: `ID 2e8a:4005` (`lsusb.txt`, `system_snapshot.txt`).
- Есть устройство `/dev/ttyACM0` (`tty_devices.txt`), в `journalctl_k_tail.txt` присутствует `cdc_acm 1-4:1.0: ttyACM0: USB ACM device`.

Итог: в данном прогоне runtime-перечисление прошло штатно.

## 4. Linux evidence (wrapper + quick analysis + raw)
- Wrapper (`...171427...log`): цикл BOOTSEL -> прошивка -> runtime завершен со статусом `normal`; в runtime видны `2e8a:4005` и `/dev/ttyACM0`.
- Quick analysis (`...171427...quick_analysis.txt`): подтверждает `Runtime USB enumeration status: normal`; фиксирует наличие `2e8a:4005` в snapshot-артефактах.
- Raw (`...171458`):
  - `runtime_status.txt`: `runtime_enum_status=normal`.
  - `lsusb_tree.txt`: `If 0` и `If 1` устройства `2e8a:4005` с драйвером `cdc_acm`.
  - `lsusb_v_2e8a_4005.txt`: отображены валидные классы CDC (`bInterfaceClass=2` и `bInterfaceClass=10`).
  - `journalctl_k_tail.txt`: есть события перечисления `2e8a:4005` и создание `ttyACM0`; явных `panic/oops/BUG` для этого прогона не обнаружено.

## 5. Классификация
- Основная: `OK`.

Обоснование:
- перечисление runtime-устройства успешно (`normal`);
- драйвер `cdc_acm` привязан и интерфейс `ttyACM0` создан;
- признаки `DRIVER_BIND_ERROR`, `DRIVER_CRASH` и `SYSTEM_CRASH` в предоставленных артефактах отсутствуют.

## 6. Уровень уверенности
Средний.

Причина: все доступные артефакты этого запуска указывают на штатный результат, но в `lsusb_v_2e8a_4005.txt` не наблюдается невалидный `bInterfaceClass`, поэтому сам факт предъявления именно аномального интерфейсного класса в данном прогоне из этих данных не подтверждается.

## 7. Пробелы в доказательной базе
- `journalctl_k_tail*` содержит события нескольких прогонов; нужна строгая привязка по времени к текущему запуску.
- В wrapper fallback `dmesg` недоступен по правам (`Операция не позволена`), часть kernel-контекста отсутствует.
- Нет отдельного USB pcap/control-transfer трейса, подтверждающего фактические байты интерфейсного дескриптора в этом запуске.

## 8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|runtime_enum_status" logs/runs/*030_interface_class_invalid_01*.log results/raw/linux/030_interface_class_invalid_01_20260516_171458/runtime_status.txt`
2. `rg -n "2e8a:4005|ttyACM0|cdc_acm" results/raw/linux/030_interface_class_invalid_01_20260516_171458/{lsusb.txt,system_snapshot.txt,lsusb_tree.txt,tty_devices.txt,journalctl_k_tail.txt,journalctl_k_tail_300.txt}`
3. `sed -n '1,220p' results/raw/linux/030_interface_class_invalid_01_20260516_171458/lsusb_v_2e8a_4005.txt`
4. `rg -n "panic|oops|BUG:|segfault|crash" results/raw/linux/030_interface_class_invalid_01_20260516_171458/journalctl_k_tail.txt results/raw/linux/030_interface_class_invalid_01_20260516_171458/journalctl_k_tail_300.txt`
