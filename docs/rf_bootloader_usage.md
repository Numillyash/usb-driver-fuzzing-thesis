# Минимальный OOB RF путь команды `bootloader`

## Назначение

Реализован минимальный внеполосный (out-of-band) стендовый путь:

1. Оператор отправляет в `esp32c3_bridge` команду `bootloader` через USB Serial/JTAG COM-порт ESP32-C3.
2. ESP32-C3 отправляет серию фиксированных `rf_test_packet_t` с `msg_type=RF_TEST_MSG_BOOTLOADER_REQ` (несколько попыток подряд).
3. `usb_case_demo` на RP2040 неблокирующе опрашивает nRF24.
4. При валидном пакете RP2040 печатает диагностику и вызывает `reset_usb_boot(0, 0)`.

## Формат пакета

Используется существующий fixed-payload формат `rf_test_packet_t` (16 байт):

- `magic = 0x5246` (`RF_TEST_PACKET_MAGIC`)
- `version = 1` (`RF_TEST_PACKET_VERSION`)
- `msg_type = 3` (`RF_TEST_MSG_BOOTLOADER_REQ`)
- `seq = локальный счётчик ESP32-C3`
- `uptime_ms = uptime ESP32-C3`
- `arg0 = 0x424F4F54` (`'BOOT'`)
- `flags = 0`

В `usb_case_demo` пакет принимается только при совпадении `magic/version/msg_type/arg0`.

## Команды сборки

RP2040:

```bash
./tools/build-container.sh
```

ESP32-C3:

```bash
docker compose run --rm -T esp32c3-dev idf.py build
```

## Ожидаемые serial-команды (ESP32-C3)

- `help` — показать доступные команды.
- `bootloader` — отправить RF bootloader request.

Ожидаемые ответы ESP32-C3:

- `bootloader_req tx ok`
- `bootloader_req tx fail`
- Лог по попыткам: `bootloader_req attempt=N ... status=...`

## Troubleshooting

- Для ESP32-C3 команды `help`/`bootloader` принимаются через USB Serial/JTAG COM-порт (например, `VID:PID=303A:1001`), а не через внешний UART0.
- На RP2040 выполните команду `info` и проверьте поля `rf_ready`, `rf_listener_mode`, `rf_raw_rx_count`, `rf_wrong_pipe_count`, `rf_wrong_len_count`, `rf_bad_magic_count`, `rf_bootloader_rx_count`, `rf_bootloader_bad_count`, `rf_last_msg_type`, `rf_last_seq`, `rf_last_rx_len`, `rf_last_pipe`, `rf_last_payload_len`, `rf_last_status`.
- Если на ESP32-C3 видно `bootloader_req tx ok`, но на RP2040 `rf_raw_rx_count=0`, вероятна рассинхронизация RF-направления/адреса/канала/pipe.
- Если на RP2040 `rfdiag_STATUS=0x40` и одновременно `rfdiag_FIFO_STATUS` показывает непустой RX FIFO, это означает, что пакет дошёл до радио, но прошивка не вычитывает RX FIFO (проверьте актуальность firmware с фиксами RX draining).
- Если растёт `rf_wrong_len_count` или `rf_bad_magic_count`, вероятно есть несовпадение формата RF-пакета между передатчиком и приёмником.

## Локальная процедура теста на Windows

1. Собрать RP2040 и ESP32-C3 прошивки.
2. Прошить RP2040 в `usb_case_demo.uf2`.
3. Прошить ESP32-C3 в `esp32c3_bridge`.
4. Открыть COM-порт ESP32-C3 USB Serial/JTAG в терминале.
5. Убедиться, что видна подсказка `commands: help bootloader`.
6. Ввести `bootloader`.
7. Проверить ответ ESP32-C3 `bootloader_req tx ok`.
8. Проверить логи RP2040:
   - `usb_case_demo: RF bootloader request received`
   - `usb_case_demo: entering USB bootloader`
9. Проверить переэнумерацию RP2040 как BOOTSEL mass-storage.

## План отката

1. Удалить `RF_TEST_MSG_BOOTLOADER_REQ` из `rf_test_packet.h`.
2. Удалить обработку RF bootloader request из `usb_case_demo`.
3. Удалить команду `bootloader` из `esp32c3_bridge`.
4. Повторно прогнать baseline проверку `portable_demo` по `docs/test_procedure_rf_roundtrip.md`.

## Ограничение

Команда работает только пока RP2040-прошивка жива и nRF24 на RP2040 успешно инициализирован.
