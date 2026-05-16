# Результат эксперимента: 021_config_wtotallength_too_large_01

## 1. Цель кейса
Проверить реакцию Linux USB-стека на завышенное значение `wTotalLength` в configuration descriptor и определить, приводит ли это к отказу перечисления, частичному перечислению или к штатной инициализации runtime-устройства.

## 2. Входные артефакты
- Case config:
  - `experiments/cases/021_config_wtotallength_too_large.json`
- Wrapper log:
  - `logs/runs/20260516_164141_021_config_wtotallength_too_large_01.log`
- Quick analysis:
  - `logs/runs/20260516_164141_021_config_wtotallength_too_large_01.quick_analysis.txt`
- Raw Linux results:
  - `results/raw/linux/021_config_wtotallength_too_large_01_20260516_164212`

## 3. Наблюдаемый результат runtime enumeration
- В wrapper-заезде зафиксировано: `Runtime USB enumeration status: normal` (`...164141...log`).
- В raw-статусе подтверждено: `runtime_enum_status=normal` (`...164212/runtime_status.txt`).
- Runtime-устройство присутствует в USB-списке: `ID 2e8a:4005` (`...164212/lsusb.txt`, `...164212/system_snapshot.txt`).
- Последовательный интерфейс создан: `/dev/ttyACM0` (`...164212/tty_devices.txt`).

Итог: в данном прогоне Linux выполнил штатное runtime-перечисление.

## 4. Linux evidence (wrapper + quick analysis + raw)
- Wrapper (`...164141...log`): полный цикл BOOTSEL -> прошивка -> runtime завершился со статусом `normal`; устройство `2e8a:4005` и `ttyACM0` присутствуют.
- Quick analysis (`...164141...quick_analysis.txt`): подтверждает `Runtime USB enumeration status: normal`; фиксирует наличие артефактов `lsusb`/`journalctl`/`tty` для запуска `021`.
- Raw (`...164212`):
  - `runtime_status.txt`: `runtime_enum_status=normal`;
  - `lsusb.txt`: присутствует `Bus 001 Device 046: ID 2e8a:4005`;
  - `tty_devices.txt`: присутствует `/dev/ttyACM0`;
  - `journalctl_k_tail.txt` и `journalctl_k_tail_300.txt`: есть события привязки `cdc_acm ... ttyACM0` в окне запуска; явных признаков `kernel panic`/`oops`/краха драйвера в представленных фрагментах нет.

## 5. Классификация
- Основная: `OK`.

Обоснование:
- runtime status = `normal`;
- устройство и CDC-интерфейс видимы в артефактах запуска;
- отсутствуют подтвержденные признаки `DRIVER_CRASH` или `SYSTEM_CRASH`.

## 6. Уровень уверенности
Высокий.

Причина: согласованность независимых источников (`wrapper`, `quick_analysis`, `runtime_status`, `lsusb`, `tty`, `journalctl`) по факту штатного перечисления в рассматриваемом прогоне.

## 7. Пробелы в доказательной базе
- `journalctl_k_tail*` содержит историю нескольких прогонов, поэтому интерпретация выполняется с привязкой к времени запуска.
- Fallback `dmesg` в wrapper недоступен по правам (`Операция не позволена`), поэтому часть kernel-контекста ограничена.
- В предоставленных артефактах нет отдельного декодированного трасса control-transfer по полю `wTotalLength`; вывод сделан по наблюдаемому результату перечисления и bind-событиям.

## 8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|runtime_enum_status" logs/runs/*021_config_wtotallength_too_large_01*.log results/raw/linux/021_config_wtotallength_too_large_01_20260516_164212/runtime_status.txt`
2. `rg -n "2e8a:4005|ttyACM0" results/raw/linux/021_config_wtotallength_too_large_01_20260516_164212/{lsusb.txt,system_snapshot.txt,tty_devices.txt,journalctl_k_tail.txt,journalctl_k_tail_300.txt}`
3. `rg -n "panic|oops|BUG:|segfault|crash" results/raw/linux/021_config_wtotallength_too_large_01_20260516_164212/journalctl_k_tail.txt results/raw/linux/021_config_wtotallength_too_large_01_20260516_164212/journalctl_k_tail_300.txt`
4. `sed -n '1,120p' results/raw/linux/021_config_wtotallength_too_large_01_20260516_164212/lsusb_v_2e8a_4005.txt`
