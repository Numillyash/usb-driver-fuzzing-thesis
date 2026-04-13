# Control-Plane Minimal Plan

## Purpose

Этот документ фиксирует минимальный следующий шаг после подтверждённого RFTEST round-trip baseline.

## Next Minimal Packet Types

Следующий минимальный набор logical packet types:

- `HEARTBEAT`
- `PING`
- `PONG`
- `GET_STATUS`
- `STATUS`
- `SET_MODE`
- `ACK`
- `NACK`

## Scope Of The Next Step

Следующий шаг должен сделать только одно:

- подтвердить минимальный control primitive поверх уже работающего RF transport и serial/debug path.

Это означает:

- единый logical command set;
- очень маленький bounded command/response набор;
- без попытки сразу решить все transport-задачи.

## Explicitly Not Included Yet

На следующий шаг сознательно не входят:

- fragmentation
- scenario upload
- log streaming
- session management
- config transfer
- large payloads

## Sequencing

Порядок работ должен оставаться таким:

1. сохранить RFTEST baseline как независимый bring-up reference;
2. добавить минимальный control primitive;
3. только потом расширять command set и transport behavior.
