#ifndef STATUS_H
#define STATUS_H

#include <stdint.h>

#include "protocol.h"
#include "scenario_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    STATUS_SYSTEM_BOOT = 0,
    STATUS_SYSTEM_SAFE_IDLE = 1,
    STATUS_SYSTEM_CONTROLLED_IDLE = 2,
    STATUS_SYSTEM_CONTROLLED_RUNNING = 3,
    STATUS_SYSTEM_AUTORUN_RUNNING = 4,
    STATUS_SYSTEM_ERROR = 5,
    STATUS_SYSTEM_PANIC = 6
} status_system_state_t;

typedef enum {
    STATUS_USB_DISABLED = 0,
    STATUS_USB_ARMED = 1,
    STATUS_USB_ENUMERATING = 2,
    STATUS_USB_ACTIVE = 3,
    STATUS_USB_DETACHED = 4,
    STATUS_USB_FAULT = 5
} status_usb_state_t;

typedef struct {
    uint8_t mode;
    uint8_t system_state;
    uint8_t usb_state;
    uint8_t scenario_state;
    uint32_t uptime_ms;
    uint32_t fault_flags;
    uint16_t active_scenario_id;
    uint16_t reserved0;
} runtime_status_t;

void status_init(runtime_status_t *status);
void status_set_mode(runtime_status_t *status, cp_device_mode_t mode);
void status_set_system_state(runtime_status_t *status, status_system_state_t state);
void status_set_usb_state(runtime_status_t *status, status_usb_state_t state);
void status_set_scenario_state(runtime_status_t *status, scenario_state_t state, uint16_t scenario_id);
void status_build_protocol_snapshot(const runtime_status_t *status, cp_status_payload_t *snapshot);

#ifdef __cplusplus
}
#endif

#endif /* STATUS_H */
