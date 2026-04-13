#ifndef SCENARIO_TYPES_H
#define SCENARIO_TYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCENARIO_STORE_MAGIC             UINT32_C(0x53434E52)
#define SCENARIO_FORMAT_VERSION          UINT16_C(1)
#define SCENARIO_NAME_MAX                UINT8_C(32)
#define SCENARIO_CHUNK_DATA_MAX          UINT16_C(192)

typedef enum {
    PERSONA_NONE = 0,
    PERSONA_HID = 1,
    PERSONA_MSC = 2,
    PERSONA_COMPOSITE = 3
} persona_type_t;

typedef enum {
    SCENARIO_STATE_IDLE = 0,
    SCENARIO_STATE_PREPARE = 1,
    SCENARIO_STATE_RUNNING = 2,
    SCENARIO_STATE_WAIT = 3,
    SCENARIO_STATE_FINISHED = 4,
    SCENARIO_STATE_ABORTED = 5,
    SCENARIO_STATE_FAILED = 6
} scenario_state_t;

typedef enum {
    SCENARIO_TRIGGER_MANUAL = 0,
    SCENARIO_TRIGGER_AUTORUN = 1,
    SCENARIO_TRIGGER_BOOT_STRAP = 2
} scenario_trigger_t;

typedef enum {
    SCENARIO_RECONNECT_NEVER = 0,
    SCENARIO_RECONNECT_ONCE = 1,
    SCENARIO_RECONNECT_ALWAYS = 2
} scenario_reconnect_policy_t;

typedef enum {
    SCENARIO_WATCHDOG_DEFAULT = 0,
    SCENARIO_WATCHDOG_RELAXED = 1,
    SCENARIO_WATCHDOG_STRICT = 2
} scenario_watchdog_policy_t;

typedef enum {
    SCENARIO_STEP_NOP = 0,
    SCENARIO_STEP_DELAY_MS = 1,
    SCENARIO_STEP_USB_ATTACH = 2,
    SCENARIO_STEP_USB_DETACH = 3,
    SCENARIO_STEP_SEND_REPORT = 4,
    SCENARIO_STEP_WAIT_FOR_HOST = 5
} scenario_step_opcode_t;

/* Persistent metadata header for a stored scenario object. */
typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t header_size;
    uint16_t format_version;
    uint16_t scenario_id;
    uint16_t descriptor_variant_id;
    uint16_t report_program_id;
    uint16_t timing_profile_id;
    uint16_t payload_size;
    uint32_t payload_crc32;
} scenario_header_t;

/* Operator-visible attributes used by list and inspect operations. */
typedef struct __attribute__((packed)) {
    uint16_t scenario_id;
    uint8_t persona_type;
    uint8_t trigger;
    uint16_t repeat_count;
    uint8_t reconnect_policy;
    uint8_t autorun_enabled;
    uint8_t log_level;
    uint8_t watchdog_policy;
    uint32_t flags;
} scenario_meta_t;

/* Fixed-size directory entry for the scenario store. */
typedef struct __attribute__((packed)) {
    uint16_t scenario_id;
    uint16_t slot_index;
    uint32_t flash_offset;
    uint32_t object_size;
    uint32_t object_crc32;
} scenario_slot_t;

/* One scenario step descriptor. Step-specific data is out of scope for now. */
typedef struct __attribute__((packed)) {
    uint16_t opcode;
    uint16_t flags;
    uint32_t arg0;
    uint32_t arg1;
    uint32_t delay_ms;
} scenario_step_t;

#ifdef __cplusplus
}
#endif

#endif /* SCENARIO_TYPES_H */
