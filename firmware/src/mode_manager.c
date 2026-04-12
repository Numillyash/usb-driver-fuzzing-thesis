#include "mode_manager.h"

void mode_manager_init(mode_manager_state_t *state, cp_device_mode_t initial_mode)
{
    if (state == NULL) {
        return;
    }

    state->current_mode = (uint8_t)initial_mode;
    state->requested_mode = (uint8_t)initial_mode;
    state->boot_reason = MODE_MANAGER_BOOT_NORMAL;
    state->safe_fallback = 0;
}

cp_device_mode_t mode_manager_get_mode(const mode_manager_state_t *state)
{
    if (state == NULL) {
        return CP_DEVICE_MODE_SAFE;
    }

    return (cp_device_mode_t)state->current_mode;
}

bool mode_manager_request_mode(mode_manager_state_t *state, cp_device_mode_t next_mode)
{
    if (state == NULL) {
        return false;
    }

    /* Stub only: policy checks and persistence hooks are added later. */
    state->requested_mode = (uint8_t)next_mode;
    state->current_mode = (uint8_t)next_mode;
    return true;
}

void mode_manager_enter_safe(mode_manager_state_t *state, mode_manager_boot_reason_t reason)
{
    if (state == NULL) {
        return;
    }

    state->boot_reason = (uint8_t)reason;
    state->safe_fallback = 1;
    state->requested_mode = CP_DEVICE_MODE_SAFE;
    state->current_mode = CP_DEVICE_MODE_SAFE;
}
