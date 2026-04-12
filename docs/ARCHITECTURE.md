# Архитектура стенда USB-экспериментов с RP2040 Zero, радиомостом и трёхконтурным логированием

## 1. Назначение системы

Стенд предназначен для исследования реакции тестовой Windows-машины на USB-устройство на базе RP2040 Zero, способное воспроизводить корректные и некорректные сценарии USB/HID, а в дальнейшем и USB Mass Storage / composite-сценарии.

Ключевая особенность стенда заключается в том, что тестовая машина может зависнуть, потерять логирование, уйти в BSOD или перезагрузиться. Поэтому логирование строится по трём независимым контурам:

1. локальный журнал на самой RP2040 Zero;
2. живой радиолог на основную машину;
3. аудит на самой тестовой Windows-машине.

## 2. Общая схема системы

### 2.1. Основной ПК

Роль основного ПК:

* запуск операторской консоли;
* хранение сценариев и конфигураций;
* отправка команд в стенд;
* приём и сохранение логов;
* расшифровка бинарных событий;
* последующий анализ результатов.

### 2.2. Радиомост на стороне основной машины

Устройство: **ESP32-C3 SuperMini + nRF24L01+PA+LNA**.

Роль:

* приём телеметрии и ответов от RP2040 Zero;
* передача команд от основного ПК в радиоканал;
* мост между USB/UART основного ПК и RF-каналом;
* отсутствие собственной логики сценариев.

### 2.3. Исполнитель на тестовой стороне

Устройство: **RP2040 Zero + nRF24L01+PA+LNA**.

Роль:

* воспроизведение USB-сценариев;
* локальное журналирование;
* исполнение конфигураций и сценариев;
* передача событий и статусов по радиоканалу.

### 2.4. Тестовая Windows-машина

Роль:

* объект эксперимента;
* источник системных артефактов: USB/PnP события, логи драйверов, BSOD dump и др.

## 3. Режимы работы RP2040 Zero

Режимы прошивки разделяются на три уровня.

### 3.1. SAFE

В этом режиме:

* USB persona выключена;
* радио работает;
* логирование работает;
* разрешены сервисные команды;
* можно читать и стирать логи;
* можно загружать конфигурации и сценарии.

Назначение: безопасная отладка и восстановление после неудачных экспериментов.

### 3.2. CONTROLLED

В этом режиме:

* устройство ждёт команд с основного ПК;
* сценарии запускаются по команде;
* USB persona включается и выключается управляемо;
* основной режим экспериментов и отладки.

### 3.3. AUTORUN

В этом режиме:

* при старте выполняется ранее сохранённый сценарий;
* управление с основного ПК может быть ограничено или отсутствовать;
* нужен для автономных прогонов и экспериментов, при которых тестовая машина может упасть слишком рано.

### 3.4. Boot policy

Политика загрузки:

* если strap pin активен при старте — переход в `AUTORUN`;
* если strap pin не активен — переход в `SAFE` или `CONTROLLED`;
* после нескольких подряд watchdog/panic restart устройство принудительно стартует в `SAFE`.

## 4. Состояния системы

### 4.1. Глобальные состояния рантайма

```c
typedef enum {
    SYS_BOOT = 0,
    SYS_SAFE_IDLE,
    SYS_CONTROLLED_IDLE,
    SYS_CONTROLLED_RUNNING,
    SYS_AUTORUN_RUNNING,
    SYS_ERROR,
    SYS_PANIC
} system_state_t;
```

### 4.2. Состояния USB persona

```c
typedef enum {
    USB_DISABLED = 0,
    USB_ARMED,
    USB_ENUMERATING,
    USB_ACTIVE,
    USB_DETACHED,
    USB_FAULT
} usb_state_t;
```

### 4.3. Состояния сценария

```c
typedef enum {
    SCN_IDLE = 0,
    SCN_PREPARE,
    SCN_RUNNING,
    SCN_WAIT,
    SCN_FINISHED,
    SCN_ABORTED,
    SCN_FAILED
} scenario_state_t;
```

## 5. Модульная структура прошивки RP2040

Предлагаемая структура:

```text
firmware/
  src/
    main.c
    scheduler.c
    mode_manager.c
    radio_link.c
    command_dispatch.c
    logger.c
    storage.c
    status.c
    scenario_engine.c
    usb_persona.c
    usb_persona_hid.c
    usb_persona_msc.c        // резерв под следующий этап
    watchdog_guard.c
  include/
    scheduler.h
    mode_manager.h
    radio_link.h
    command_dispatch.h
    logger.h
    storage.h
    status.h
    scenario_engine.h
    usb_persona.h
    protocol.h
    scenario_types.h
    log_types.h
```

### 5.1. Назначение модулей

* `scheduler` — кооперативный диспетчер задач;
* `mode_manager` — выбор режима старта и переходы в safe mode;
* `radio_link` — обмен пакетами с nRF24;
* `command_dispatch` — обработка команд control plane;
* `logger` — событийный журнал в RAM и flash;
* `storage` — конфигурации, сценарии, counters, panic record;
* `status` — компактный статус устройства;
* `scenario_engine` — исполнение сценариев как state machine;
* `usb_persona` — общий интерфейс USB-поведения;
* `usb_persona_hid` — текущая USB persona;
* `usb_persona_msc` — резерв под следующий этап;
* `watchdog_guard` — watchdog и recovery policy.

## 6. Планировщик “по мотивам GyverOS”

Используется лёгкий кооперативный диспетчер задач.

### 6.1. Правила

* все задачи неблокирующие;
* задачи выполняют короткий шаг работы;
* длительные операции разбиваются на фазы;
* `tud_task()` вызывается отдельно и очень часто;
* callbacks не должны делать тяжёлых действий.

### 6.2. Базовая структура задачи

```c
typedef void (*task_fn_t)(void);

typedef struct {
    task_fn_t fn;
    uint32_t period_ms;
    uint32_t next_run_ms;
    uint8_t enabled;
} task_t;
```

### 6.3. Набор базовых задач

* `task_radio_rx`
* `task_radio_tx`
* `task_command`
* `task_logger_flush`
* `task_status_heartbeat`
* `task_scenario_step`
* `task_watchdog`
* `task_storage_commit`

### 6.4. Главный цикл

```c
for (;;) {
    tud_task();
    scheduler_tick();
}
```

## 7. Формат сценария

Сценарий задаётся декларативно, а не набором сырых байтов в реальном времени.

```c
typedef enum {
    PERSONA_NONE = 0,
    PERSONA_HID,
    PERSONA_MSC,
    PERSONA_COMPOSITE
} persona_type_t;

typedef struct {
    uint16_t scenario_id;
    uint8_t  version;
    uint8_t  persona_type;
    uint16_t descriptor_variant_id;
    uint16_t report_program_id;
    uint16_t timing_profile_id;
    uint16_t repeat_count;
    uint8_t  reconnect_policy;
    uint8_t  autorun_enabled;
    uint8_t  log_level;
    uint8_t  watchdog_policy;
    uint32_t flags;
    uint32_t crc32;
} scenario_config_t;
```

### 7.1. Смысл сценария

Сценарий задаёт:

* какой тип USB persona использовать;
* какой дескриптор или его вариант применять;
* какой набор фаз/отчётов/операций исполнять;
* с какими таймингами, повторениями и политикой reconnect.

## 8. Командный протокол control plane

Минимальный набор команд:

```c
typedef enum {
    CMD_PING = 1,
    CMD_GET_STATUS,
    CMD_SET_MODE,
    CMD_UPLOAD_CONFIG,
    CMD_SELECT_SCENARIO,
    CMD_START,
    CMD_STOP,
    CMD_USB_ATTACH,
    CMD_USB_DETACH,
    CMD_GET_LOG_INFO,
    CMD_GET_LOG_CHUNK,
    CMD_ERASE_LOG,
    CMD_SAVE_CONFIG,
    CMD_REBOOT
} command_id_t;
```

Ответы:

```c
typedef enum {
    RESP_ACK = 1,
    RESP_NACK,
    RESP_STATUS,
    RESP_LOG_CHUNK,
    RESP_EVENT,
    RESP_PANIC
} response_id_t;
```

### 8.1. Базовые поля пакета

Каждый пакет должен содержать:

* `seq`
* `request_id`
* `command_id` или `response_id`
* `payload_len`
* `payload`
* `crc16`

## 9. Формат журнала событий

Используется фиксированный бинарный формат записи.

```c
typedef struct {
    uint32_t t_ms;
    uint16_t event_id;
    uint16_t flags;
    uint32_t arg0;
    uint32_t arg1;
} log_record_t;
```

### 9.1. Список ключевых событий

```c
typedef enum {
    EVT_BOOT = 1,
    EVT_MODE_CHANGED,
    EVT_CONFIG_LOADED,
    EVT_SCENARIO_SELECTED,
    EVT_SCENARIO_STARTED,
    EVT_SCENARIO_STEP,
    EVT_SCENARIO_FINISHED,
    EVT_USB_ATTACH,
    EVT_USB_DETACH,
    EVT_USB_ENUM_START,
    EVT_USB_ACTIVE,
    EVT_USB_FAULT,
    EVT_DESCRIPTOR_SELECTED,
    EVT_REPORT_SENT,
    EVT_RADIO_RX,
    EVT_RADIO_TX_OK,
    EVT_RADIO_TX_FAIL,
    EVT_FLASH_COMMIT,
    EVT_WATCHDOG_KICK,
    EVT_ASSERT,
    EVT_PANIC
} event_id_t;
```

Текстовую интерпретацию выполняет консоль на основном ПК.

## 10. Flash layout

Flash делится на несколько функциональных областей:

```text
[ settings block ]
[ active scenario config ]
[ scenario bank ]
[ persistent counters / boot reason / panic record ]
[ event log ring ]
```

### 10.1. Что хранить обязательно

* текущий режим;
* последний выбранный сценарий;
* счётчики перезагрузок;
* причина panic / watchdog reset;
* CRC активной конфигурации;
* write pointer кольцевого журнала.

### 10.2. Что писать во flash выборочно

Во flash записываются:

* важные события;
* checkpoints;
* panic/error;
* периодические snapshots.

Нельзя писать туда полный поток телеметрии без фильтрации.

## 11. USB persona manager

Общий интерфейс для разных типов USB-поведения:

```c
typedef struct {
    void (*init)(void);
    void (*apply_config)(const scenario_config_t *cfg);
    void (*attach)(void);
    void (*detach)(void);
    void (*step)(void);
    usb_state_t (*get_state)(void);
} usb_persona_vtable_t;
```

Сначала реализуется `HID`, позже — `MSC` и `COMPOSITE`.

## 12. Scenario engine

`scenario_engine` не занимается низкоуровневым USB и не знает про радио-детали. Его роль:

* загрузить сценарий;
* управлять фазами;
* вызывать attach/detach persona;
* соблюдать тайминги;
* считать повторы;
* завершать или аварийно прерывать сценарий;
* логировать ключевые переходы.

## 13. Консоль на основном ПК

Рекомендуемый формат — Python CLI.

Структура:

```text
host_console/
  console.py
  transport/
    serial_bridge.py
  protocol/
    packets.py
    codec.py
  commands/
    ping.py
    status.py
    mode.py
    start.py
    stop.py
    logs.py
  decode/
    log_decoder.py
```

### 13.1. Базовые команды пользователя

* `ping`
* `status`
* `mode safe|controlled|autorun`
* `load <config>`
* `start`
* `stop`
* `attach`
* `detach`
* `logs`
* `erase-log`
* `reboot`

## 14. Политика безопасного восстановления

Обязательные механизмы recovery:

* strap pin → вход в `SAFE`;
* timeout без связи → fallback в safe mode;
* несколько подряд panic/watchdog reboot → безопасный запуск в `SAFE`;
* фиксирование причины загрузки: cold boot, software reboot, watchdog, panic.

## 15. Что кодить первым

### Этап 1

* `ARCHITECTURE.md`
* `protocol.h`
* `scenario_types.h`
* `log_types.h`

### Этап 2

* `scheduler`
* `logger`
* `mode_manager`

### Этап 3

* `radio_link`
* `command_dispatch`
* `status`

### Этап 4

* `usb_persona_hid`
* `scenario_engine`

### Этап 5

* консоль на основном ПК

## 16. Что сознательно не усложняется на MVP-этапе

На текущем этапе не включаются:

* RTOS;
* второе ядро RP2040;
* динамическое выделение памяти;
* файловая система во flash;
* сложный shell на самой плате;
* шифрование радиоканала.

## 17. Критерий успешного MVP

MVP считается достигнутым, если:

* RP2040 запускается в одном из режимов `SAFE / CONTROLLED / AUTORUN`;
* основной ПК может получить статус устройства по радиоканалу;
* можно загрузить конфигурацию сценария;
* можно запустить и остановить HID-сценарий;
* ключевые события пишутся в RAM/flash журнал;
* события передаются на основной ПК через ESP32-C3 радиомост.
