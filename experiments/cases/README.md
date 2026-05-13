# Каталог кейсов

Каталог содержит начальный набор безопасных кейсов для исследования реакций USB-стека Windows и Linux.

## Формат

Кейсы хранятся в формате JSON (без YAML) для обеспечения совместимости со стандартной библиотекой Python и снижения внешних зависимостей.

## Группы

- `baseline` — контрольные корректные профили.
- `device_descriptor` — мутации device descriptor.
- `config_descriptor` — мутации configuration descriptor.
- `interface_endpoint` — мутации interface/endpoint descriptor.
- `string_descriptor` — мутации string descriptor.

## Правила безопасности

- Запрещены полезные нагрузки выполнения команд на хосте.
- Запрещены сценарии эскалации привилегий и закрепления.
- Разрешены только контролируемые аномалии descriptor/enumeration.
