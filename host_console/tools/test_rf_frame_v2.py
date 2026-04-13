#!/usr/bin/env python3

from __future__ import annotations

import os
import sys


SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
HOST_CONSOLE_DIR = os.path.dirname(SCRIPT_DIR)
if HOST_CONSOLE_DIR not in sys.path:
    sys.path.insert(0, HOST_CONSOLE_DIR)

from protocol.rf_frame_v2 import (  # noqa: E402
    RFFrameV2,
    RFV2_FLAG_NONE,
    RFV2_PKT_HEARTBEAT,
    RFV2_SRC_RP2040,
    build_heartbeat_payload,
    decode_frame,
    encode_frame,
    pretty_print_frame,
    validate_frame_crc,
)


def main() -> int:
    heartbeat_payload = build_heartbeat_payload(
        uptime_ms=12345,
        mode=1,
        system_state=2,
        link_state=3,
    )
    frame = RFFrameV2(
        pkt_type=RFV2_PKT_HEARTBEAT,
        src_id=RFV2_SRC_RP2040,
        flags=RFV2_FLAG_NONE,
        seq=7,
        payload=heartbeat_payload,
    )

    encoded = encode_frame(frame)
    decoded = decode_frame(encoded)
    crc_ok = validate_frame_crc(decoded)

    print(f"encoded_len={len(encoded)} crc_ok={crc_ok}")
    print(pretty_print_frame(decoded))

    if not crc_ok:
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
