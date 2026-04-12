#ifndef LOG_TYPES_H
#define LOG_TYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_REGION_MAGIC                UINT32_C(0x4C4F4752)
#define LOG_FORMAT_VERSION              UINT16_C(1)
#define LOG_RECORD_ALIGNMENT            UINT8_C(4)

typedef enum {
    LOG_SEVERITY_TRACE = 0,
    LOG_SEVERITY_DEBUG = 1,
    LOG_SEVERITY_INFO = 2,
    LOG_SEVERITY_WARN = 3,
    LOG_SEVERITY_ERROR = 4,
    LOG_SEVERITY_FATAL = 5
} log_severity_t;

typedef enum {
    LOG_SOURCE_SYSTEM = 0,
    LOG_SOURCE_PROTOCOL = 1,
    LOG_SOURCE_RADIO = 2,
    LOG_SOURCE_STORAGE = 3,
    LOG_SOURCE_SCENARIO = 4,
    LOG_SOURCE_USB = 5,
    LOG_SOURCE_WATCHDOG = 6
} log_source_t;

typedef enum {
    LOG_EVENT_BOOT = 0x0001,
    LOG_EVENT_MODE_CHANGE = 0x0002,
    LOG_EVENT_PROTOCOL_RX = 0x0003,
    LOG_EVENT_PROTOCOL_TX = 0x0004,
    LOG_EVENT_SCENARIO_START = 0x0005,
    LOG_EVENT_SCENARIO_STOP = 0x0006,
    LOG_EVENT_USB_STATE = 0x0007,
    LOG_EVENT_FAULT = 0x0008,
    LOG_EVENT_WATCHDOG = 0x0009
} log_event_id_t;

/* Region header for the persistent flash-backed log ring. */
typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t version;
    uint16_t header_size;
    uint32_t region_size;
    uint32_t write_offset;
    uint32_t wrapped_count;
    uint32_t dropped_records;
    uint32_t crc32;
} log_region_header_t;

/* Common prefix for all binary log records. Payload bytes may follow. */
typedef struct __attribute__((packed)) {
    uint16_t record_size;
    uint8_t severity;
    uint8_t source;
    uint16_t event_id;
    uint16_t flags;
    uint32_t uptime_ms;
    uint32_t arg0;
    uint32_t arg1;
    uint32_t crc32;
} log_record_header_t;

/* Short status snapshot that can be embedded as a structured log payload. */
typedef struct __attribute__((packed)) {
    uint8_t mode;
    uint8_t system_state;
    uint8_t usb_state;
    uint8_t scenario_state;
    uint32_t fault_flags;
} log_status_snapshot_t;

#ifdef __cplusplus
}
#endif

#endif /* LOG_TYPES_H */
