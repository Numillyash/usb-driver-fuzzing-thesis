# Результат эксперимента: 020_config_wtotallength_too_small_01

## 1. Цель кейса
Проверить реакцию Linux USB-стека на заниженное значение `wTotalLength` в configuration descriptor и определить, приводит ли это к срыву runtime-перечисления, деградации привязки CDC или к штатному перечислению.

## 2. Входные артефакты
- Wrapper log:
  - `logs/runs/20260516_163059_020_config_wtotallength_too_small_01.log`
- Quick analysis:
  - `logs/runs/20260516_163059_020_config_wtotallength_too_small_01.quick_analysis.txt`
- Raw Linux results:
  - `results/raw/linux/020_config_wtotallength_too_small_01_20260516_163129`
  - `results/raw/linux/020_config_wtotallength_too_small_01_20260516_163130`

## 3. Наблюдаемый результат runtime enumeration
- В wrapper-заезде зафиксировано: `Runtime USB enumeration status: normal` (`...163059...log`).
- В raw-статусе подтверждено: `runtime_enum_status=normal` (`...163130/runtime_status.txt`).
- Runtime-устройство присутствует в USB-списке: `ID 2e8a:4005` (`...163130/lsusb.txt`, `...163129/system_snapshot.txt`).
- Создан последовательный интерфейс: `/dev/ttyACM0` (`...163130/tty_devices.txt`).

Итог: runtime-перечисление прошло штатно.

## 4. Linux evidence
- Wrapper (`...163059...log`): полный цикл BOOTSEL -> прошивка -> runtime завершен со статусом `normal`.
- Quick analysis (`...163059...quick_analysis.txt`): подтверждает `Runtime USB enumeration status: normal`, наличие `2e8a:4005` и отсутствие признаков аварий.
- Raw (`...163129`, `...163130`):
  - `journalctl_k_tail*.txt` содержит события повторного подключения и успешной привязки `cdc_acm ... ttyACM0` (в том числе на отметке `2026-05-16 16:31:29`);
  - признаков `kernel panic`, `oops`, crash-драйвера или crash-хоста в представленных логах нет.

## 5. Классификация
- Основная: `OK`.

Обоснование:
- runtime status = `normal`;
- устройство `2e8a:4005` видно в runtime;
- `ttyACM0` создан;
- нет доказательств падения драйвера или системы.

## 6. Уровень уверенности
Высокий.

Причина: независимые источники (`wrapper`, `quick_analysis`, `runtime_status`, `lsusb`, `tty`, `journalctl`) согласованно показывают штатное runtime-перечисление.

## 7. Пробелы в доказательной базе
- `journalctl_k_tail*` содержит события нескольких запусков, поэтому интерпретация выполняется с фильтрацией по времени.
- В wrapper fallback `dmesg` недоступен по правам (`Операция не позволена`), поэтому часть kernel-контекста ограничена.

## 8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|normal" logs/runs/*020_config_wtotallength_too_small_01*.log`
2. `cat results/raw/linux/020_config_wtotallength_too_small_01_20260516_163130/runtime_status.txt`
3. `rg -n "2e8a:4005|ttyACM0" results/raw/linux/020_config_wtotallength_too_small_01_20260516_163130/{lsusb.txt,tty_devices.txt,journalctl_k_tail.txt}`
4. `rg -n "panic|oops|BUG:|segfault|crash" results/raw/linux/020_config_wtotallength_too_small_01_20260516_163129/journalctl_k_tail_300.txt results/raw/linux/020_config_wtotallength_too_small_01_20260516_163130/journalctl_k_tail.txt`
