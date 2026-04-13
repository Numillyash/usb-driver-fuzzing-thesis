# ADR 0002: Two Wire Formats, One Logical Protocol

## Status

Accepted.

## Context

На текущем этапе уже подтверждён работающий RFTEST round-trip baseline:

- RP2040 отправляет `RFTEST_DATA`;
- ESP32-C3 принимает и валидирует пакет;
- ESP32-C3 отправляет `RFTEST_ACK`;
- RP2040 принимает matching ACK по `seq`.

Этот baseline доказал физическую пригодность радио и логирования, но он не является full control-plane protocol. Одновременно в документации уже существует draft `cp_header_t`, который полезен как модель будущего command protocol, но он не оптимизирован под текущий радиоканал `nRF24`.

## Decision

Принимается следующее архитектурное решение:

1. Текущий RFTEST round-trip baseline остаётся в проекте как bring-up / milestone transport.
2. Future control-plane не должен сразу ломать или заменять RFTEST baseline.
3. Логический уровень команд должен быть единым для radio и serial.
4. Wire format для radio и serial может отличаться.
5. Текущий draft `cp_header_t` считается serial/debug-oriented framing.
6. Для radio будет введён отдельный compact frame v2.
7. Fragmentation сознательно не реализуется на следующем шаге.
8. Следующий шаг: минимальный control primitive поверх уже подтверждённого transport baseline, а не полный protocol stack.

## Consequences

### Positive

- RFTEST baseline остаётся доступным как стабильный hardware bring-up режим;
- serial/debug path можно развивать без давления ограничений `nRF24 MTU`;
- radio framing можно сделать компактным и bounded без искусственной совместимости с serial header;
- логический command model остаётся единым и не раздваивается по смыслу.

### Negative

- в проекте некоторое время будут сосуществовать два wire format;
- документацию нужно явно поддерживать в разделении:
  - implemented now
  - next radio frame
  - future serial/debug framing

## Explicit Non-Decision

Этим ADR не принимаются следующие решения:

- конкретный binary layout compact radio frame v2;
- fragmentation/reassembly;
- session management;
- large payload transport;
- scenario upload protocol;
- log streaming protocol.
