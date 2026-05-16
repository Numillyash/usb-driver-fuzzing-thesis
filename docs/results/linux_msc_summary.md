# Сводка Linux MSC-кейсов (050-059)

## Назначение MSC Linux robustness-тестирования
Цель блока `050-059` — проверить устойчивость Linux USB/MSC-стека при безопасных аномалиях поведения MSC-устройства на RP2040, разделяя успешность USB enumeration/bind и деградацию на уровне SCSI, блочного и FS-слоя.

## Методика
- Использовано безопасное исследовательское устройство на RP2040 (`2e8a:4006`) в режиме MSC.
- Поведение носителя: read-only/RAM-backed (контролируемая эмуляция).
- Доступ к реальным дискам хоста не выполнялся.
- Оператор не выполнял запись, форматирование и монтирование тестового MSC-носителя.
- Источники доказательств: wrapper-логи (`logs/runs/*quick_analysis.txt`), `lsusb`, `lsusb -t`, `journalctl`, `dmesg` (где доступен), `lsblk`, а также per-case отчёты в `docs/results/05*_msc_*.md`.

## Таксономия результатов
Использовалась единая шкала:
`OK`, `EXPECTED_REJECT`, `PARTIAL_ENUM`, `DRIVER_BIND_ERROR`, `USERSPACE_FAILURE`, `KERNEL_WARNING`, `DRIVER_CRASH`, `SYSTEM_CRASH`.

## Таблица результатов 050-059
| Кейс | Краткое описание | Класс |
|---|---|---|
| 050 baseline initial | baseline MSC initial run | `PARTIAL_ENUM` |
| 050 baseline repeat | baseline MSC repeat run | `OK` |
| 051 zero block size | логический размер блока = 0 | `PARTIAL_ENUM` |
| 052 zero capacity | ёмкость = 0 блоков | `PARTIAL_ENUM` |
| 053 huge capacity | аномально большая ёмкость | `PARTIAL_ENUM` |
| 054 READ CAPACITY short response | укороченный ответ READ CAPACITY | `PARTIAL_ENUM` |
| 055 INQUIRY invalid length | некорректная длина INQUIRY-данных | `PARTIAL_ENUM` |
| 056 CSW invalid status | невалидный/аномальный BOT CSW status (эквивалент) | `KERNEL_WARNING` |
| 057 READ10 stall/failure | деградация на READ10 (stall/failure-поведение) | `KERNEL_WARNING` |
| 058 endpoint packet size zero | `wMaxPacketSize=0` у MSC endpoint | `KERNEL_WARNING` |
| 059 BOT residue mismatch | BOT residue mismatch (контролируемый эквивалент) | `KERNEL_WARNING` |

## Счётчики по классификации
- `OK`: 1
- `PARTIAL_ENUM`: 6
- `KERNEL_WARNING`: 4
- `EXPECTED_REJECT`: 0
- `DRIVER_BIND_ERROR`: 0
- `USERSPACE_FAILURE`: 0
- `DRIVER_CRASH`: 0
- `SYSTEM_CRASH`: 0

## Основные наблюдения
- MSC baseline после MSC-aware runner даёт ожидаемую успешность в повторном прогоне (`050_msc_baseline_repeat_01 -> OK`).
- Кейсы `051-055` в целом доходят до USB enumeration и bind к `usb-storage`, но деградируют на storage/SCSI/block-уровне.
- Кейсы `056-059` в целом также доходят до USB enumeration и bind к `usb-storage`, при этом фиксируются kernel/storage warnings.
- Кейс `058` особенно показателен: `wMaxPacketSize=0` подтверждён напрямую в `lsusb`-артефактах.
- Подтверждённых `DRIVER_CRASH` не выявлено.
- Подтверждённых `SYSTEM_CRASH` не выявлено.

## Ограничения доказательной базы
- Часть `journalctl` tail-срезов содержит шум от предыдущих запусков.
- Для строгой атрибуции событий к одному запуску нужны более узкие временные окна `journalctl` и желательно `usbmon`/pcap.
- Из-за ограничений TinyUSB API часть мутаций BOT/CSW представлена как контролируемые эквиваленты, а не буквальная побайтная порча «сырого» CSW.

## Итог по MSC-блоку
В текущих Linux-прогонах USB/`usb-storage` стек сохранил устойчивость: malformed MSC-поведение приводило к деградации и предупреждениям на уровне хранения, но без подтверждённых падений драйвера или kernel panic/system crash.

## Дальнейшие Linux-only шаги
- Захват `usbmon` для каждого прогона.
- Более строгие timestamp-окна kernel-логов на один запуск.
- Повтор наиболее сильных кейсов (`058`, `059`, а также деградационных `051-055`).
- Композитные кейсы MSC+CDC/HID.
- Кейсы с временными/состояниями мутациями (timing/state).
