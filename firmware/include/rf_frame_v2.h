#ifndef RF_FRAME_V2_H
#define RF_FRAME_V2_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RFV2_FRAME_MAGIC        UINT16_C(0x5632)
#define RFV2_FRAME_VERSION      UINT8_C(2)
#define RFV2_FRAME_SIZE         UINT8_C(32)
#define RFV2_HEADER_SIZE        UINT8_C(10)
#define RFV2_PAYLOAD_SIZE       UINT8_C(RFV2_FRAME_SIZE - RFV2_HEADER_SIZE)

typedef enum {
    RFV2_SRC_RP2040 = 1,
    RFV2_SRC_ESP32C3 = 2,
    RFV2_SRC_HOST = 3
} rfv2_src_id_t;

typedef enum {
    RFV2_FLAG_NONE = 0x00,
    RFV2_FLAG_ACK_REQUIRED = 0x01,
    RFV2_FLAG_ERROR = 0x02
} rfv2_flags_t;

typedef enum {
    RFV2_PKT_HEARTBEAT = 1,
    RFV2_PKT_PING = 2,
    RFV2_PKT_PONG = 3,
    RFV2_PKT_GET_STATUS = 4,
    RFV2_PKT_STATUS = 5,
    RFV2_PKT_SET_MODE = 6,
    RFV2_PKT_ACK = 7,
    RFV2_PKT_NACK = 8
} rfv2_pkt_type_t;

typedef struct __attribute__((packed)) {
    uint16_t magic;
    uint8_t version;
    uint8_t pkt_type;
    uint8_t src_id;
    uint8_t flags;
    uint16_t seq;
    uint8_t payload_len;
    uint8_t header_crc8;
} rfv2_header_t;

typedef struct __attribute__((packed)) {
    uint32_t uptime_ms;
    uint8_t mode;
    uint8_t system_state;
    uint8_t link_state;
    uint8_t reserved0;
} rfv2_heartbeat_payload_t;

typedef struct __attribute__((packed)) {
    uint32_t nonce;
} rfv2_ping_payload_t;

typedef struct __attribute__((packed)) {
    uint32_t nonce;
    uint32_t responder_uptime_ms;
} rfv2_pong_payload_t;

typedef struct __attribute__((packed)) {
    uint8_t request_flags;
} rfv2_get_status_payload_t;

typedef struct __attribute__((packed)) {
    uint32_t uptime_ms;
    uint16_t active_seq;
    uint8_t mode;
    uint8_t system_state;
    uint8_t usb_state;
    uint8_t scenario_state;
    uint16_t fault_flags;
} rfv2_status_payload_t;

typedef struct __attribute__((packed)) {
    uint8_t target_mode;
    uint8_t persist;
} rfv2_set_mode_payload_t;

typedef struct __attribute__((packed)) {
    uint8_t acked_type;
    uint8_t status_code;
    uint16_t acked_seq;
} rfv2_ack_payload_t;

typedef struct __attribute__((packed)) {
    uint8_t rejected_type;
    uint8_t reason_code;
    uint16_t rejected_seq;
    uint32_t detail;
} rfv2_nack_payload_t;

typedef struct __attribute__((packed)) {
    rfv2_header_t header;
    uint8_t payload[RFV2_PAYLOAD_SIZE];
} rfv2_frame_t;

#if defined(__cplusplus)
static_assert(sizeof(rfv2_header_t) == RFV2_HEADER_SIZE, "rfv2_header_t size must be 10 bytes");
static_assert(sizeof(rfv2_frame_t) == RFV2_FRAME_SIZE, "rfv2_frame_t size must be 32 bytes");
static_assert(sizeof(rfv2_heartbeat_payload_t) <= RFV2_PAYLOAD_SIZE, "heartbeat payload too large");
static_assert(sizeof(rfv2_ping_payload_t) <= RFV2_PAYLOAD_SIZE, "ping payload too large");
static_assert(sizeof(rfv2_pong_payload_t) <= RFV2_PAYLOAD_SIZE, "pong payload too large");
static_assert(sizeof(rfv2_get_status_payload_t) <= RFV2_PAYLOAD_SIZE, "get_status payload too large");
static_assert(sizeof(rfv2_status_payload_t) <= RFV2_PAYLOAD_SIZE, "status payload too large");
static_assert(sizeof(rfv2_set_mode_payload_t) <= RFV2_PAYLOAD_SIZE, "set_mode payload too large");
static_assert(sizeof(rfv2_ack_payload_t) <= RFV2_PAYLOAD_SIZE, "ack payload too large");
static_assert(sizeof(rfv2_nack_payload_t) <= RFV2_PAYLOAD_SIZE, "nack payload too large");
#else
_Static_assert(sizeof(rfv2_header_t) == RFV2_HEADER_SIZE, "rfv2_header_t size must be 10 bytes");
_Static_assert(sizeof(rfv2_frame_t) == RFV2_FRAME_SIZE, "rfv2_frame_t size must be 32 bytes");
_Static_assert(sizeof(rfv2_heartbeat_payload_t) <= RFV2_PAYLOAD_SIZE, "heartbeat payload too large");
_Static_assert(sizeof(rfv2_ping_payload_t) <= RFV2_PAYLOAD_SIZE, "ping payload too large");
_Static_assert(sizeof(rfv2_pong_payload_t) <= RFV2_PAYLOAD_SIZE, "pong payload too large");
_Static_assert(sizeof(rfv2_get_status_payload_t) <= RFV2_PAYLOAD_SIZE, "get_status payload too large");
_Static_assert(sizeof(rfv2_status_payload_t) <= RFV2_PAYLOAD_SIZE, "status payload too large");
_Static_assert(sizeof(rfv2_set_mode_payload_t) <= RFV2_PAYLOAD_SIZE, "set_mode payload too large");
_Static_assert(sizeof(rfv2_ack_payload_t) <= RFV2_PAYLOAD_SIZE, "ack payload too large");
_Static_assert(sizeof(rfv2_nack_payload_t) <= RFV2_PAYLOAD_SIZE, "nack payload too large");
#endif

#ifdef __cplusplus
}
#endif

#endif /* RF_FRAME_V2_H */
