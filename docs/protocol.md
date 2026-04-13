# Control-Plane Protocol Draft

## Назначение

Control plane нужен для безопасного управления стендом в режимах `SAFE`, `CONTROLLED` и ограниченно в `AUTORUN`. На этом этапе фиксируется только первый черновик wire format и идентификаторы сообщений. Runtime-логика, ретраи и state machine будут реализованы позже.

## Транспорт

Базовый транспорт для control plane:

- primary: nRF24 radio link между основной машиной и RP2040;
- fallback: USB CDC REPL / line-oriented shell;
- optional debug path: UART bridge на лабораторном стенде.

Общий пакетный формат должен быть одинаковым для radio и для возможного бинарного serial-канала. REPL fallback остаётся текстовым фасадом над теми же командами.

### Ограничения nRF24 transport

- practical radio MTU для одного nRF24 payload: до `32` байт;
- из этих `32` байт часть должна уйти под transport header, sequence и flags;
- поэтому control-plane payload для radio должен проектироваться как `small, bounded, fragmentable`;
- структуры, которые не укладываются в один radio frame, обязаны передаваться chunked или уходить в serial-only path;
- serial transport не имеет этого ограничения и остаётся основным fallback для service/debug операций.

## Пакет верхнего уровня

Каждый бинарный пакет состоит из:

1. `cp_header_t`
2. payload фиксированного или переменного размера
3. `payload_crc32` в хвосте пакета или в payload-структуре команды

Черновой заголовок:

```c
typedef struct __attribute__((packed)) {
    uint16_t magic;
    uint8_t  version;
    uint8_t  packet_type;
    uint8_t  flags;
    uint8_t  header_len;
    uint16_t sequence;
    uint16_t payload_len;
    uint16_t header_crc16;
} cp_header_t;
```

### Поля заголовка

- `magic`: сигнатура control-plane пакета, для первого черновика `0x4350` (`'CP'`);
- `version`: версия протокола, стартовое значение `1`;
- `packet_type`: команда, ответ, событие или ack/nack;
- `flags`: биты `ACK_REQUIRED`, `IS_FRAGMENT`, `IS_RETRY`, `FROM_REPL_GATEWAY`;
- `header_len`: размер заголовка для совместимости с будущими расширениями;
- `sequence`: монотонный номер пакета в пределах сессии;
- `payload_len`: длина payload в байтах без заголовка;
- `header_crc16`: быстрый контроль заголовка.

## Packet Type IDs

Предлагаемый диапазон:

- `0x01`..`0x3f` — host-to-device commands;
- `0x40`..`0x7f` — device-to-host responses;
- `0x80`..`0xbf` — async events;
- `0xc0`..`0xff` — зарезервировано.

## Command IDs

Первый черновик команд:

| ID | Name | Назначение |
| --- | --- | --- |
| `0x01` | `CP_CMD_PING` | Проверка канала и round-trip |
| `0x02` | `CP_CMD_GET_INFO` | Краткая информация о firmware, board и protocol version |
| `0x03` | `CP_CMD_GET_STATUS` | Чтение текущего статуса, mode и error flags |
| `0x04` | `CP_CMD_SET_MODE` | Перевод между `SAFE`, `CONTROLLED`, `AUTORUN` policy |
| `0x05` | `CP_CMD_REBOOT` | Управляемая перезагрузка с указанием причины |
| `0x06` | `CP_CMD_LIST_SCENARIOS` | Перечень слотов сценариев |
| `0x07` | `CP_CMD_GET_SCENARIO_META` | Метаданные конкретного сценария |
| `0x08` | `CP_CMD_UPLOAD_SCENARIO_BEGIN` | Начало записи сценария |
| `0x09` | `CP_CMD_UPLOAD_SCENARIO_CHUNK` | Передача фрагмента сценария |
| `0x0a` | `CP_CMD_UPLOAD_SCENARIO_COMMIT` | Завершение записи и проверка CRC |
| `0x0b` | `CP_CMD_DELETE_SCENARIO` | Удаление сценария из slot store |
| `0x0c` | `CP_CMD_RUN_SCENARIO` | Запуск сценария по `scenario_id` |
| `0x0d` | `CP_CMD_ABORT_SCENARIO` | Аварийная остановка сценария |
| `0x0e` | `CP_CMD_GET_CONFIG` | Чтение конфигурации |
| `0x0f` | `CP_CMD_SET_CONFIG` | Обновление конфигурации |
| `0x10` | `CP_CMD_LOG_INFO` | Информация о лог-буфере |
| `0x11` | `CP_CMD_LOG_READ` | Чтение диапазона логов |
| `0x12` | `CP_CMD_LOG_ERASE` | Очистка log ring |
| `0x13` | `CP_CMD_CLEAR_FAULTS` | Сброс fault/panic markers |

## Command Class Split

Для следующего этапа полезно считать команды двумя классами транспорта.

### Radio-safe commands

Это команды с короткими payload и предсказуемым ответом, которые можно безопасно укладывать в nRF24 MTU:

- `CP_CMD_PING`
- `CP_CMD_GET_INFO`
- `CP_CMD_GET_STATUS`
- `CP_CMD_SET_MODE`
- `CP_CMD_REBOOT`
- `CP_CMD_ABORT_SCENARIO`
- `CP_CMD_LOG_INFO`
- `CP_CMD_CLEAR_FAULTS`

### Serial-only or Serial-preferred commands

Это команды, которые либо уже предполагают крупные payload, либо почти наверняка потребуют fragmentation/reassembly:

- `CP_CMD_LIST_SCENARIOS`
- `CP_CMD_GET_SCENARIO_META`
- `CP_CMD_UPLOAD_SCENARIO_BEGIN`
- `CP_CMD_UPLOAD_SCENARIO_CHUNK`
- `CP_CMD_UPLOAD_SCENARIO_COMMIT`
- `CP_CMD_DELETE_SCENARIO`
- `CP_CMD_RUN_SCENARIO`
- `CP_CMD_GET_CONFIG`
- `CP_CMD_SET_CONFIG`
- `CP_CMD_LOG_READ`
- `CP_CMD_LOG_ERASE`

Интерпретация на текущем этапе:

- `radio-safe` не значит “только radio”, эти команды также должны работать по serial;
- `serial-only` не запрещает будущую поддержку по radio, но требует явной fragment policy и более строгих timeout/retry правил.

## Response IDs

Первый черновик ответов:

| ID | Name | Назначение |
| --- | --- | --- |
| `0x40` | `CP_RSP_ACK` | Успешное подтверждение команды |
| `0x41` | `CP_RSP_NACK` | Ошибка валидации или отказ политики |
| `0x42` | `CP_RSP_PONG` | Ответ на `PING` |
| `0x43` | `CP_RSP_INFO` | Ответ на `GET_INFO` |
| `0x44` | `CP_RSP_STATUS` | Ответ на `GET_STATUS` |
| `0x45` | `CP_RSP_SCENARIO_LIST` | Порция списка сценариев |
| `0x46` | `CP_RSP_SCENARIO_META` | Метаданные сценария |
| `0x47` | `CP_RSP_CONFIG` | Снимок конфигурации |
| `0x48` | `CP_RSP_LOG_INFO` | Метаданные log ring |
| `0x49` | `CP_RSP_LOG_DATA` | Фрагмент лог-записей |
| `0x4a` | `CP_RSP_BUSY` | Устройство занято и просит retry/backoff |

## Async Event IDs

Асинхронные события нужны для статусов и логирования вне модели request/response:

| ID | Name | Назначение |
| --- | --- | --- |
| `0x80` | `CP_EVT_BOOT` | Завершение старта и boot reason |
| `0x81` | `CP_EVT_MODE_CHANGED` | Смена режима |
| `0x82` | `CP_EVT_SCENARIO_STATE` | Изменение состояния сценария |
| `0x83` | `CP_EVT_USB_STATE` | Изменение USB persona state |
| `0x84` | `CP_EVT_FAULT` | Fault, panic или watchdog escalation |
| `0x85` | `CP_EVT_LOG_READY` | Появились новые записи в log ring |

## Общие правила payload

- multi-byte поля передаются в little-endian;
- все структуры wire format должны быть `packed`;
- для расширяемых структур добавляются `size` и `version`;
- переменные блоки передаются как `count + bytes[]`;
- команды записи обязаны нести CRC payload или полного объекта.

## Error / NACK codes

Для `CP_RSP_NACK` предлагается отдельный `reason_code`:

| ID | Name | Значение |
| --- | --- | --- |
| `0x01` | `CP_ERR_BAD_MAGIC` | Неверная сигнатура пакета |
| `0x02` | `CP_ERR_BAD_VERSION` | Неподдерживаемая версия протокола |
| `0x03` | `CP_ERR_BAD_LENGTH` | Несогласованная длина payload |
| `0x04` | `CP_ERR_BAD_CRC` | Ошибка CRC |
| `0x05` | `CP_ERR_UNKNOWN_CMD` | Неизвестная команда |
| `0x06` | `CP_ERR_INVALID_STATE` | Команда запрещена в текущем режиме |
| `0x07` | `CP_ERR_BUSY` | Устройство занято |
| `0x08` | `CP_ERR_NOT_FOUND` | Сущность не найдена |
| `0x09` | `CP_ERR_NO_SPACE` | Недостаточно места во flash |
| `0x0a` | `CP_ERR_UNSUPPORTED` | Команда или опция пока не реализована |

## REPL Fallback

REPL fallback нужен как сервисный канал даже при отсутствии бинарного host-side клиента.

### Базовые правила

- REPL работает поверх USB CDC;
- каждая команда — одна строка ASCII;
- аргументы разделяются пробелами;
- ответ либо короткая строка `OK` / `ERR`, либо многострочный вывод с завершающей строкой статуса;
- при наличии бинарного control-plane runtime REPL выступает thin wrapper над теми же dispatch IDs.

### Минимальный набор команд

| REPL | Бинарный эквивалент |
| --- | --- |
| `help` | локальная сервисная команда |
| `ping` | `CP_CMD_PING` |
| `info` | `CP_CMD_GET_INFO` |
| `status` | `CP_CMD_GET_STATUS` |
| `mode safe` | `CP_CMD_SET_MODE` |
| `mode controlled` | `CP_CMD_SET_MODE` |
| `reboot` | `CP_CMD_REBOOT` |
| `log info` | `CP_CMD_LOG_INFO` |
| `log erase` | `CP_CMD_LOG_ERASE` |

### Ограничения fallback-режима

- REPL не должен быть единственным каналом для больших бинарных объектов;
- загрузка сценариев через REPL допустима только как debug-only режим с chunked hex/base64 представлением;
- команды, опасные для автономного прогона, в `AUTORUN` могут быть отключены.

## Что ещё не зафиксировано

Следующие решения остаются открытыми и будут уточняться до реализации:

- максимальный MTU radio-пакета и правила фрагментации;
- тип CRC для полного payload;
- механизм session/open-channel handshake;
- retry policy и timeout matrix;
- бинарный формат `GET_INFO`, `GET_STATUS`, `LOG_DATA` и scenario upload records.
