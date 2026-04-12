#ifndef MODE_MANAGER_H
#define MODE_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

#include "protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MODE_MANAGER_BOOT_UNKNOWN = 0,
    MODE_MANAGER_BOOT_NORMAL = 1,
    MODE_MANAGER_BOOT_STRAP_AUTORUN = 2,
    MODE_MANAGER_BOOT_WATCHDOG_RECOVERY = 3
} mode_manager_boot_reason_t;

typedef struct {
    uint8_t current_mode;
    uint8_t requested_mode;
    uint8_t boot_reason;
    uint8_t safe_fallback;
} mode_manager_state_t;

void mode_manager_init(mode_manager_state_t *state, cp_device_mode_t initial_mode);
cp_device_mode_t mode_manager_get_mode(const mode_manager_state_t *state);
bool mode_manager_request_mode(mode_manager_state_t *state, cp_device_mode_t next_mode);
void mode_manager_enter_safe(mode_manager_state_t *state, mode_manager_boot_reason_t reason);

#ifdef __cplusplus
}
#endif

#endif /* MODE_MANAGER_H */
