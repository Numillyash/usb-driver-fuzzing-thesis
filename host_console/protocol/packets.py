from __future__ import annotations


def build_text_command(command: str) -> str:
    """
    Temporary REPL-oriented command builder.

    This stays intentionally text-based until the binary framing from
    docs/protocol.md is implemented on both host and device sides.
    """
    normalized = command.strip()
    if not normalized:
        normalized = "ping"
    return normalized + "\n"
