#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CP_PROTOCOL_MAGIC            UINT16_C(0x4350)
#define CP_PROTOCOL_VERSION          UINT8_C(1)
#define CP_PROTOCOL_HEADER_SIZE      UINT8_C(12)

#define CP_FLAG_ACK_REQUIRED         UINT8_C(1u << 0)
#define CP_FLAG_IS_FRAGMENT          UINT8_C(1u << 1)
#define CP_FLAG_IS_RETRY             UINT8_C(1u << 2)
#define CP_FLAG_FROM_REPL_GATEWAY    UINT8_C(1u << 3)

#define CP_PACKET_PAYLOAD_MAX        UINT16_C(224)
#define CP_SEQUENCE_INVALID          UINT16_C(0xffff)

typedef enum {
    CP_PACKET_TYPE_NONE = 0x00,

    CP_CMD_PING = 0x01,
    CP_CMD_GET_INFO = 0x02,
    CP_CMD_GET_STATUS = 0x03,
    CP_CMD_SET_MODE = 0x04,
    CP_CMD_REBOOT = 0x05,
    CP_CMD_LIST_SCENARIOS = 0x06,
    CP_CMD_GET_SCENARIO_META = 0x07,
    CP_CMD_UPLOAD_SCENARIO_BEGIN = 0x08,
    CP_CMD_UPLOAD_SCENARIO_CHUNK = 0x09,
    CP_CMD_UPLOAD_SCENARIO_COMMIT = 0x0a,
    CP_CMD_DELETE_SCENARIO = 0x0b,
    CP_CMD_RUN_SCENARIO = 0x0c,
    CP_CMD_ABORT_SCENARIO = 0x0d,
    CP_CMD_GET_CONFIG = 0x0e,
    CP_CMD_SET_CONFIG = 0x0f,
    CP_CMD_LOG_INFO = 0x10,
    CP_CMD_LOG_READ = 0x11,
    CP_CMD_LOG_ERASE = 0x12,
    CP_CMD_CLEAR_FAULTS = 0x13,

    CP_RSP_ACK = 0x40,
    CP_RSP_NACK = 0x41,
    CP_RSP_PONG = 0x42,
    CP_RSP_INFO = 0x43,
    CP_RSP_STATUS = 0x44,
    CP_RSP_SCENARIO_LIST = 0x45,
    CP_RSP_SCENARIO_META = 0x46,
    CP_RSP_CONFIG = 0x47,
    CP_RSP_LOG_INFO = 0x48,
    CP_RSP_LOG_DATA = 0x49,
    CP_RSP_BUSY = 0x4a,

    CP_EVT_BOOT = 0x80,
    CP_EVT_MODE_CHANGED = 0x81,
    CP_EVT_SCENARIO_STATE = 0x82,
    CP_EVT_USB_STATE = 0x83,
    CP_EVT_FAULT = 0x84,
    CP_EVT_LOG_READY = 0x85
} cp_packet_type_t;

typedef enum {
    CP_ERR_NONE = 0x00,
    CP_ERR_BAD_MAGIC = 0x01,
    CP_ERR_BAD_VERSION = 0x02,
    CP_ERR_BAD_LENGTH = 0x03,
    CP_ERR_BAD_CRC = 0x04,
    CP_ERR_UNKNOWN_CMD = 0x05,
    CP_ERR_INVALID_STATE = 0x06,
    CP_ERR_BUSY = 0x07,
    CP_ERR_NOT_FOUND = 0x08,
    CP_ERR_NO_SPACE = 0x09,
    CP_ERR_UNSUPPORTED = 0x0a
} cp_error_code_t;

typedef enum {
    CP_DEVICE_MODE_SAFE = 0,
    CP_DEVICE_MODE_CONTROLLED = 1,
    CP_DEVICE_MODE_AUTORUN = 2
} cp_device_mode_t;

typedef enum {
    CP_REBOOT_REASON_NONE = 0,
    CP_REBOOT_REASON_OPERATOR = 1,
    CP_REBOOT_REASON_APPLY_CONFIG = 2,
    CP_REBOOT_REASON_RECOVERY = 3
} cp_reboot_reason_t;

/* Common fixed header for all binary control-plane packets. */
typedef struct __attribute__((packed)) {
    uint16_t magic;
    uint8_t version;
    uint8_t packet_type;
    uint8_t flags;
    uint8_t header_len;
    uint16_t sequence;
    uint16_t payload_len;
    uint16_t header_crc16;
} cp_header_t;

/* Used by PING/PONG to validate transport and basic timing. */
typedef struct __attribute__((packed)) {
    uint32_t nonce;
    uint32_t host_time_ms;
} cp_ping_payload_t;

/* Generic ACK/NACK tail for commands with no typed response payload. */
typedef struct __attribute__((packed)) {
    uint8_t command_id;
    uint8_t status_code;
    uint16_t reserved0;
    uint32_t detail;
} cp_ack_payload_t;

/* Compact error response, reused by NACK and BUSY replies. */
typedef struct __attribute__((packed)) {
    uint8_t command_id;
    uint8_t reason_code;
    uint16_t retry_after_ms;
    uint32_t detail;
} cp_nack_payload_t;

/* Basic firmware identity. Kept small enough for radio transport. */
typedef struct __attribute__((packed)) {
    uint16_t protocol_version;
    uint16_t board_id;
    uint32_t firmware_version;
    uint32_t feature_flags;
} cp_info_payload_t;

/* Runtime snapshot for GET_STATUS and periodic status events. */
typedef struct __attribute__((packed)) {
    uint8_t mode;
    uint8_t system_state;
    uint8_t usb_state;
    uint8_t scenario_state;
    uint32_t uptime_ms;
    uint32_t fault_flags;
    uint16_t active_scenario_id;
    uint16_t reserved0;
} cp_status_payload_t;

/* Request to change the operating mode. */
typedef struct __attribute__((packed)) {
    uint8_t target_mode;
    uint8_t persist;
    uint16_t reserved0;
} cp_set_mode_payload_t;

/* Request a clean reboot with an optional boot-policy hint. */
typedef struct __attribute__((packed)) {
    uint8_t reboot_reason;
    uint8_t next_mode;
    uint16_t reserved0;
} cp_reboot_payload_t;

/* Header used before chunked scenario upload starts. */
typedef struct __attribute__((packed)) {
    uint16_t scenario_id;
    uint16_t total_size;
    uint32_t total_crc32;
} cp_upload_begin_payload_t;

/* Scenario chunk transfer. Data bytes follow this fixed header. */
typedef struct __attribute__((packed)) {
    uint16_t scenario_id;
    uint16_t chunk_offset;
    uint16_t chunk_size;
    uint16_t chunk_crc16;
} cp_upload_chunk_payload_t;

/* Finalize scenario upload after all chunks are written. */
typedef struct __attribute__((packed)) {
    uint16_t scenario_id;
    uint16_t reserved0;
    uint32_t total_crc32;
} cp_upload_commit_payload_t;

/* Read a bounded slice of the persistent log ring. */
typedef struct __attribute__((packed)) {
    uint32_t read_offset;
    uint16_t max_bytes;
    uint16_t reserved0;
} cp_log_read_payload_t;

/* Metadata for the persistent log ring. */
typedef struct __attribute__((packed)) {
    uint32_t region_size;
    uint32_t write_offset;
    uint32_t wrapped_count;
    uint32_t dropped_records;
} cp_log_info_payload_t;

#ifdef __cplusplus
}
#endif

#endif /* PROTOCOL_H */
