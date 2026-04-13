from __future__ import annotations

from dataclasses import dataclass
import struct
from typing import Dict

RFV2_FRAME_MAGIC = 0x5632
RFV2_FRAME_VERSION = 2
RFV2_FRAME_SIZE = 32
RFV2_HEADER_SIZE = 10
RFV2_PAYLOAD_SIZE = RFV2_FRAME_SIZE - RFV2_HEADER_SIZE

RFV2_PKT_HEARTBEAT = 1
RFV2_PKT_PING = 2
RFV2_PKT_PONG = 3
RFV2_PKT_GET_STATUS = 4
RFV2_PKT_STATUS = 5
RFV2_PKT_SET_MODE = 6
RFV2_PKT_ACK = 7
RFV2_PKT_NACK = 8

RFV2_SRC_RP2040 = 1
RFV2_SRC_ESP32C3 = 2
RFV2_SRC_HOST = 3

RFV2_FLAG_NONE = 0x00
RFV2_FLAG_ACK_REQUIRED = 0x01
RFV2_FLAG_ERROR = 0x02

HEADER_STRUCT = struct.Struct("<HBBBBHBB")
HEARTBEAT_STRUCT = struct.Struct("<IBBBB")
PING_STRUCT = struct.Struct("<I")
PONG_STRUCT = struct.Struct("<II")
GET_STATUS_STRUCT = struct.Struct("<B")
STATUS_STRUCT = struct.Struct("<IHBBBBH")
SET_MODE_STRUCT = struct.Struct("<BB")
ACK_STRUCT = struct.Struct("<BBH")
NACK_STRUCT = struct.Struct("<BBHI")

PACKET_TYPE_NAMES: Dict[int, str] = {
    RFV2_PKT_HEARTBEAT: "HEARTBEAT",
    RFV2_PKT_PING: "PING",
    RFV2_PKT_PONG: "PONG",
    RFV2_PKT_GET_STATUS: "GET_STATUS",
    RFV2_PKT_STATUS: "STATUS",
    RFV2_PKT_SET_MODE: "SET_MODE",
    RFV2_PKT_ACK: "ACK",
    RFV2_PKT_NACK: "NACK",
}


def crc8_compute(data: bytes) -> int:
    crc = 0x00

    for value in data:
        crc ^= value
        for _ in range(8):
            if crc & 0x80:
                crc = ((crc << 1) ^ 0x07) & 0xFF
            else:
                crc = (crc << 1) & 0xFF

    return crc


@dataclass
class RFFrameV2:
    pkt_type: int
    src_id: int
    flags: int
    seq: int
    payload: bytes
    magic: int = RFV2_FRAME_MAGIC
    version: int = RFV2_FRAME_VERSION
    payload_len: int = 0
    header_crc8: int = 0

    def __post_init__(self) -> None:
        if self.payload_len == 0:
            self.payload_len = len(self.payload)
        if self.payload_len > RFV2_PAYLOAD_SIZE:
            raise ValueError(f"payload too large: {self.payload_len} > {RFV2_PAYLOAD_SIZE}")
        if len(self.payload) < self.payload_len:
            raise ValueError("payload shorter than payload_len")


def _build_header_bytes(
    magic: int,
    version: int,
    pkt_type: int,
    src_id: int,
    flags: int,
    seq: int,
    payload_len: int,
    header_crc8: int,
) -> bytes:
    return HEADER_STRUCT.pack(
        magic,
        version,
        pkt_type,
        src_id,
        flags,
        seq,
        payload_len,
        header_crc8,
    )


def encode_frame(frame: RFFrameV2) -> bytes:
    payload_bytes = frame.payload[: frame.payload_len]
    payload_bytes = payload_bytes.ljust(RFV2_PAYLOAD_SIZE, b"\x00")

    header_wo_crc = _build_header_bytes(
        frame.magic,
        frame.version,
        frame.pkt_type,
        frame.src_id,
        frame.flags,
        frame.seq,
        frame.payload_len,
        0,
    )
    header_crc8 = crc8_compute(header_wo_crc[:-1])
    header = _build_header_bytes(
        frame.magic,
        frame.version,
        frame.pkt_type,
        frame.src_id,
        frame.flags,
        frame.seq,
        frame.payload_len,
        header_crc8,
    )
    return header + payload_bytes


def decode_frame(raw: bytes) -> RFFrameV2:
    if len(raw) != RFV2_FRAME_SIZE:
        raise ValueError(f"expected {RFV2_FRAME_SIZE} bytes, got {len(raw)}")

    magic, version, pkt_type, src_id, flags, seq, payload_len, header_crc8 = HEADER_STRUCT.unpack(
        raw[:RFV2_HEADER_SIZE]
    )
    payload = raw[RFV2_HEADER_SIZE:]

    return RFFrameV2(
        magic=magic,
        version=version,
        pkt_type=pkt_type,
        src_id=src_id,
        flags=flags,
        seq=seq,
        payload_len=payload_len,
        header_crc8=header_crc8,
        payload=payload,
    )


def validate_frame_crc(frame: RFFrameV2) -> bool:
    header_wo_crc = _build_header_bytes(
        frame.magic,
        frame.version,
        frame.pkt_type,
        frame.src_id,
        frame.flags,
        frame.seq,
        frame.payload_len,
        0,
    )
    return frame.header_crc8 == crc8_compute(header_wo_crc[:-1])


def build_heartbeat_payload(
    uptime_ms: int,
    mode: int,
    system_state: int,
    link_state: int,
    reserved0: int = 0,
) -> bytes:
    return HEARTBEAT_STRUCT.pack(uptime_ms, mode, system_state, link_state, reserved0)


def decode_payload(frame: RFFrameV2) -> dict:
    payload = frame.payload[: frame.payload_len]

    if frame.pkt_type == RFV2_PKT_HEARTBEAT and len(payload) >= HEARTBEAT_STRUCT.size:
        uptime_ms, mode, system_state, link_state, reserved0 = HEARTBEAT_STRUCT.unpack(
            payload[: HEARTBEAT_STRUCT.size]
        )
        return {
            "uptime_ms": uptime_ms,
            "mode": mode,
            "system_state": system_state,
            "link_state": link_state,
            "reserved0": reserved0,
        }
    if frame.pkt_type == RFV2_PKT_PING and len(payload) >= PING_STRUCT.size:
        (nonce,) = PING_STRUCT.unpack(payload[: PING_STRUCT.size])
        return {"nonce": nonce}
    if frame.pkt_type == RFV2_PKT_PONG and len(payload) >= PONG_STRUCT.size:
        nonce, responder_uptime_ms = PONG_STRUCT.unpack(payload[: PONG_STRUCT.size])
        return {"nonce": nonce, "responder_uptime_ms": responder_uptime_ms}
    if frame.pkt_type == RFV2_PKT_GET_STATUS and len(payload) >= GET_STATUS_STRUCT.size:
        (request_flags,) = GET_STATUS_STRUCT.unpack(payload[: GET_STATUS_STRUCT.size])
        return {"request_flags": request_flags}
    if frame.pkt_type == RFV2_PKT_STATUS and len(payload) >= STATUS_STRUCT.size:
        uptime_ms, active_seq, mode, system_state, usb_state, scenario_state, fault_flags = STATUS_STRUCT.unpack(
            payload[: STATUS_STRUCT.size]
        )
        return {
            "uptime_ms": uptime_ms,
            "active_seq": active_seq,
            "mode": mode,
            "system_state": system_state,
            "usb_state": usb_state,
            "scenario_state": scenario_state,
            "fault_flags": fault_flags,
        }
    if frame.pkt_type == RFV2_PKT_SET_MODE and len(payload) >= SET_MODE_STRUCT.size:
        target_mode, persist = SET_MODE_STRUCT.unpack(payload[: SET_MODE_STRUCT.size])
        return {"target_mode": target_mode, "persist": persist}
    if frame.pkt_type == RFV2_PKT_ACK and len(payload) >= ACK_STRUCT.size:
        acked_type, status_code, acked_seq = ACK_STRUCT.unpack(payload[: ACK_STRUCT.size])
        return {"acked_type": acked_type, "status_code": status_code, "acked_seq": acked_seq}
    if frame.pkt_type == RFV2_PKT_NACK and len(payload) >= NACK_STRUCT.size:
        rejected_type, reason_code, rejected_seq, detail = NACK_STRUCT.unpack(payload[: NACK_STRUCT.size])
        return {
            "rejected_type": rejected_type,
            "reason_code": reason_code,
            "rejected_seq": rejected_seq,
            "detail": detail,
        }

    return {"raw_payload_hex": payload.hex()}


def pretty_print_frame(frame: RFFrameV2) -> str:
    type_name = PACKET_TYPE_NAMES.get(frame.pkt_type, f"UNKNOWN({frame.pkt_type})")
    payload_info = decode_payload(frame)
    return (
        f"RFV2 magic=0x{frame.magic:04x} ver={frame.version} type={type_name} "
        f"src={frame.src_id} flags=0x{frame.flags:02x} seq={frame.seq} "
        f"payload_len={frame.payload_len} crc8=0x{frame.header_crc8:02x} "
        f"payload={payload_info}"
    )
