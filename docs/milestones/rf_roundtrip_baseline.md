# RF Round-Trip Baseline Milestone

## Статус

Milestone зафиксирован как **достигнутый на реальном железе**.

## Что именно подтверждено

Подтверждён минимальный bidirectional RF baseline между:

- `RP2040 Zero + nRF24L01+PA+LNA`
- `ESP32-C3 SuperMini + nRF24L01+PA+LNA`

Подтверждённый цикл:

1. RP2040 раз в секунду отправляет `RFTEST_DATA`.
2. ESP32-C3 принимает и валидирует пакет.
3. ESP32-C3 отправляет `RFTEST_ACK` с тем же `seq`.
4. RP2040 принимает ACK и проверяет совпадение `seq`.

## Что видно в логах

На ESP32-C3:

- heartbeat через `ESP_LOGI`
- `RXRAW ...`
- `RXHEX ...`
- `RFTEST seq=...`
- `ACKTX seq=... ok=...`

На RP2040:

- `portable_demo: rf_test seq=... sent=...`
- `portable_demo: ack seq=...`
  или
- `portable_demo: ack timeout seq=...`

## Что milestone означает технически

Milestone фиксирует, что уже доказаны:

- физическая работа RF link в обе стороны;
- корректность базовой fixed payload radio configuration;
- пригодность текущей распиновки;
- работоспособность простого request/ack round-trip по `seq`;
- пригодность USB/serial logging на обеих сторонах для дальнейшей отладки.

## Что milestone пока не означает

Этот milestone **не** означает, что уже готов:

- control-plane protocol;
- fragmentation;
- radio session state machine высокого уровня;
- persistent storage;
- scenario transport;
- production-ready recovery policy.

## Зачем milestone важен

После этого этапа дальнейшая работа может идти уже не от гипотезы “работает ли вообще radio”, а от конкретных инженерных задач:

- стабилизация RX/TX state machine;
- переход от RF test packet к минимальному control primitive;
- аккуратное наращивание control-plane поверх уже подтверждённого transport baseline.
