# Protocol Future Notes

## Intentionally Not Implemented Yet

Следующее на текущем milestone сознательно не реализовано:

- packet fragmentation;
- retransmission policy на уровне protocol logic;
- полноценный command/response control-plane;
- scenario upload/download;
- flash persistence;
- config storage;
- host-side binary framing;
- multi-packet log transport;
- session establishment и channel ownership.

## What Belongs To Future Control-Plane Work

Следующий этап должен относиться уже к control-plane, а не к raw RF bring-up:

- выделение минимального control primitive поверх подтверждённого round-trip transport;
- переход от `RFTEST_*` сообщений к control-plane packet types;
- определение ACK/NACK semantics уровня команд;
- bounded fragmentation policy для radio;
- timeout/retry matrix;
- совместимость radio и serial paths на уровне одного command model.
