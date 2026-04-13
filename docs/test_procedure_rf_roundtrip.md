# RF Round-Trip Test Procedure

## Назначение

Этот документ фиксирует короткую процедуру проверки уже реализованного RF round-trip baseline без перехода к full control-plane.

## Preconditions

Нужно подготовить:

- RP2040 прошит текущим `portable_demo`;
- ESP32-C3 прошит текущим `esp32c3_bridge`;
- оба `nRF24L01+PA+LNA` подключены по уже зафиксированной в документации распиновке;
- питание `3.3V` и общая земля проверены;
- модули находятся на короткой дистанции для bring-up.

## Build

RP2040:

```bash
./tools/build-container.sh
```

ESP32-C3:

```bash
docker compose run --rm -T esp32c3-dev idf.py build
```

## Flash

RP2040:

```bash
./tools/flash-from-wsl.sh
```

ESP32-C3:

- flash выполняется стандартным host-side потоком ESP-IDF;
- затем открыть monitor:

```bash
idf.py -p <PORT> monitor
```

## Expected ESP32-C3 Monitor Output

На ESP32-C3 должны появляться:

- `HB bridge=esp32c3 ...`
- `RXRAW ...`
- `RXHEX ...`
- `RFTEST seq=...`
- `ACKTX seq=... ok=1 ...`

## Expected RP2040 Serial Output

На RP2040 должны появляться:

- `portable_demo: rf_test seq=... sent=1 ...`
- `portable_demo: ack seq=... ...`

## Success Criteria

Тест считается успешным, если одновременно выполняется следующее:

- `RFTEST_DATA` идут примерно с периодом `1 Hz`;
- `seq` на ESP32-C3 растёт монотонно;
- `ACKTX ok=1` появляется стабильно;
- RP2040 получает matching ACK по `seq`;
- heartbeat на ESP32-C3 не останавливается.

## Failure Signatures

### RF link problem

- нет `RXRAW/RXHEX`;
- нет `RFTEST`;
- нет `ACKTX`;
- RP2040 видит только `ack timeout`.

### RX/TX state problem

- сначала всё работает, потом появляются мусорные payload;
- `garbage_rx` растёт;
- `ACKTX ok=0` начинает встречаться;
- heartbeat продолжается, но валидный трафик деградирует.

### Validation mismatch

- `RXRAW/RXHEX` есть;
- heartbeat есть;
- но `RFTEST` отсутствует или редок;
- `seq` не совпадает, либо `magic/version` не проходят.
