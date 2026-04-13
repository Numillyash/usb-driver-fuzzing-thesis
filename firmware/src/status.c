#include "status.h"
#include <stddef.h>

void status_init(runtime_status_t *status)
{
    if (status == NULL) {
        return;
    }

    status->mode = CP_DEVICE_MODE_SAFE;
    status->system_state = STATUS_SYSTEM_BOOT;
    status->usb_state = STATUS_USB_DISABLED;
    status->scenario_state = SCENARIO_STATE_IDLE;
    status->uptime_ms = 0;
    status->fault_flags = 0;
    status->active_scenario_id = 0;
    status->reserved0 = 0;
}

void status_set_mode(runtime_status_t *status, cp_device_mode_t mode)
{
    if (status == NULL) {
        return;
    }

    status->mode = (uint8_t)mode;
}

void status_set_system_state(runtime_status_t *status, status_system_state_t state)
{
    if (status == NULL) {
        return;
    }

    status->system_state = (uint8_t)state;
}

void status_set_usb_state(runtime_status_t *status, status_usb_state_t state)
{
    if (status == NULL) {
        return;
    }

    status->usb_state = (uint8_t)state;
}

void status_set_scenario_state(runtime_status_t *status, scenario_state_t state, uint16_t scenario_id)
{
    if (status == NULL) {
        return;
    }

    status->scenario_state = (uint8_t)state;
    status->active_scenario_id = scenario_id;
}

void status_build_protocol_snapshot(const runtime_status_t *status, cp_status_payload_t *snapshot)
{
    if (status == NULL || snapshot == NULL) {
        return;
    }

    snapshot->mode = status->mode;
    snapshot->system_state = status->system_state;
    snapshot->usb_state = status->usb_state;
    snapshot->scenario_state = status->scenario_state;
    snapshot->uptime_ms = status->uptime_ms;
    snapshot->fault_flags = status->fault_flags;
    snapshot->active_scenario_id = status->active_scenario_id;
    snapshot->reserved0 = status->reserved0;
}
