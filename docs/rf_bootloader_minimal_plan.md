# Минимальный план: OOB RF-команда `bootloader` (ESP32-C3 -> RP2040 usb_case_demo)

## 1) Краткое резюме текущего RF baseline

По состоянию репозитория подтверждён baseline `RFTEST`:

- транспорт: `nRF24`, fixed payload `16` байт;
- формат: `rf_test_packet_t` из `firmware/include/rf_test_packet.h`;
- msg types: `RF_TEST_MSG_DATA=1`, `RF_TEST_MSG_ACK=2`;
- pipe0 используется под `RFTEST` адрес `k_rf_test_addr`;
- pipe1 и `rf_frame_v2` в текущем baseline выключены (`NRF24_DUAL_PIPE_BASELINE=1`, `RFV2_DISABLE_DIAG=1`);
- `portable_demo` (RP2040) отправляет `RFTEST_DATA` и принимает ACK;
- `esp32c3_bridge` принимает `RFTEST_DATA` и отправляет ACK.

Важно: `usb_case_demo` уже имеет локальную CDC-команду `bootloader`, которая вызывает `reset_usb_boot(0, 0)`, но сейчас не слушает RF.

## 2) Минимальная цель изменений (без реализации в этом PR)

Добавить отдельный out-of-band (OOB) стендовый канал управления:

1. Оператор отправляет в ESP32-C3 по UART команду `bootloader`.
2. ESP32-C3 передаёт один минимальный nRF24 пакет-запрос.
3. RP2040 в прошивке `usb_case_demo` принимает пакет.
4. RP2040 вызывает `reset_usb_boot(0, 0)`.

Команда не должна менять поведение `portable_demo` и не должна затрагивать attack-поведение ВКР.

## 3) Наиболее вероятные файлы для точечных изменений

### ESP32-C3 (bridge)

- `firmware/esp32c3_bridge/main/main.c`
  - добавить минимальный UART command parser для строки `bootloader`;
  - при совпадении отправлять один `RFTEST`-совместимый пакет специального типа.
- `firmware/esp32c3_bridge/main/bridge_uart.c`
  - использовать уже существующий `bridge_uart_read(...)` для неблокирующего чтения строки; без изменения API можно обойтись только правкой `main.c`.
- `firmware/include/rf_test_packet.h`
  - добавить новый тип, например `RF_TEST_MSG_BOOTLOADER_REQ = 3`.

### RP2040 (`usb_case_demo`)

- `firmware/src/usb_case_main.c`
  - добавить неблокирующий опрос `nrf24` в основном цикле;
  - при валидном `BOOTLOADER_REQ` вызвать `reset_usb_boot(0, 0)`.
- `firmware/CMakeLists.txt`
  - подключить для `usb_case_demo` RF-слой (`src/nrf24_radio.c`) и нужные `hardware_spi/hardware_gpio` библиотеки,
  - при этом не менять состав и поведение target `portable_demo`.

### Документация

- `docs/protocol.md` (опционально после реализации): зафиксировать, что `BOOTLOADER_REQ` относится к отдельной OOB-команде стенд-менеджмента.
- `docs/test_procedure_rf_roundtrip.md` (опционально): отдельный краткий smoke-test для OOB команды.

## 4) Минимальный формат пакета bootloader request

Чтобы не трогать `RFV2/control-plane`, использовать уже существующий `rf_test_packet_t` (`16` байт):

- `magic = RF_TEST_PACKET_MAGIC` (`0x5246`)
- `version = RF_TEST_PACKET_VERSION` (`1`)
- `msg_type = RF_TEST_MSG_BOOTLOADER_REQ` (`3`, новый enum)
- `seq =` локальный монотонный счётчик ESP32-C3
- `uptime_ms =` текущее uptime ESP32-C3
- `arg0 = 0x424F4F54` (`'BOOT'`) как дополнительный guard
- `flags = 0`

Обработка на RP2040:

- принимать только при совпадении `magic/version/msg_type`;
- дополнительно проверять `arg0 == 0x424F4F54`;
- после проверки сразу выполнять `reset_usb_boot(0, 0)`;
- ACK для этой команды не обязателен (минимальный вариант: fire-and-forget).

## 5) Как должна выглядеть serial-команда на ESP32-C3

Минимальный текстовый интерфейс (line-oriented):

- команда: `bootloader`
- завершение: `\n` или `\r\n`
- ожидаемый ответ в UART лог:
  - при успешной отправке RF-пакета: `bootloader_req tx ok`
  - при ошибке TX: `bootloader_req tx fail`
  - при неизвестной команде: краткая подсказка (`help`, `bootloader`)

Пример:

```text
> bootloader
bootloader_req tx ok
```

## 6) Как слушать пакет в `usb_case_demo`, не ломая USB CDC

Минимальный безопасный подход:

- сохранить текущий цикл обработки CDC (`getchar_timeout_us(0)` + `sleep_ms(10)`);
- добавить короткий неблокирующий poll RF в том же цикле (без длинных блокировок);
- не менять существующие команды `help/info/ping/bootloader` по CDC;
- если RF не инициализировался, `usb_case_demo` продолжает работать как сейчас (CDC-only деградация).

Технический принцип: RF-проверка должна занимать существенно меньше периода heartbeat-цикла и не блокировать чтение CDC.

## 7) Команды сборки и проверки

### RP2040

Сборка (как сейчас):

```bash
./tools/build-container.sh
```

Прошивка:

```bash
./tools/flash-from-wsl.sh
```

### ESP32-C3

Сборка:

```bash
docker compose run --rm -T esp32c3-dev idf.py build
```

Монитор/прошивка (штатный ESP-IDF flow):

```bash
idf.py -p <PORT> flash monitor
```

### Минимальный тест OOB bootloader

1. Запустить `usb_case_demo` на RP2040.
2. Запустить `esp32c3_bridge` и открыть UART monitor.
3. Ввести `bootloader` в UART ESP32-C3.
4. Проверить, что ESP32-C3 сообщает `bootloader_req tx ok`.
5. Проверить, что RP2040 уходит в USB boot mode (переэнумерация как BOOTSEL mass-storage).

## 8) Риски и план отката

Риски:

- ложное срабатывание на шум/чужой пакет (смягчается проверкой `magic/version/msg_type/arg0`);
- деградация интерактивности CDC при неудачной интеграции RF polling;
- случайное влияние на baseline `portable_demo` через общие заголовки.

Минимизация:

- держать изменения изолированными в `usb_case_demo` и `esp32c3_bridge`;
- не менять текущую логику `portable_demo` RF round-trip;
- отдельный smoke-test для `portable_demo` после изменений.

Откат:

1. Удалить обработку `RF_TEST_MSG_BOOTLOADER_REQ` из `usb_case_demo`.
2. Удалить UART-команду `bootloader` из `esp32c3_bridge`.
3. Вернуть `rf_test_packet.h` к enum без `BOOTLOADER_REQ`.
4. Повторно прогнать baseline тест из `docs/test_procedure_rf_roundtrip.md`.

## 9) Явная граница применения

Эта команда `bootloader` является **внеполосной (out-of-band) командой стенд-менеджмента** для обслуживания и перепрошивки лабораторного стенда.

Она **не является payload атакующего сценария ВКР**, не предназначена для эксплуатации уязвимостей и не добавляет вредоносного поведения класса BadUSB.
