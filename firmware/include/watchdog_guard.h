#ifndef WATCHDOG_GUARD_H
#define WATCHDOG_GUARD_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WATCHDOG_GUARD_OK = 0,
    WATCHDOG_GUARD_WARNING = 1,
    WATCHDOG_GUARD_EXPIRED = 2
} watchdog_guard_state_t;

typedef enum {
    WATCHDOG_GUARD_REASON_NONE = 0,
    WATCHDOG_GUARD_REASON_LOOP_STALL = 1,
    WATCHDOG_GUARD_REASON_TASK_TIMEOUT = 2,
    WATCHDOG_GUARD_REASON_MANUAL_RECOVERY = 3
} watchdog_guard_reason_t;

typedef struct {
    uint32_t timeout_ms;
    uint32_t last_kick_ms;
    uint32_t last_check_ms;
    uint16_t trip_count;
    uint8_t state;
    uint8_t reason;
} watchdog_guard_t;

void watchdog_guard_init(watchdog_guard_t *guard, uint32_t timeout_ms);
void watchdog_guard_kick(watchdog_guard_t *guard, uint32_t now_ms);
watchdog_guard_state_t watchdog_guard_poll(watchdog_guard_t *guard, uint32_t now_ms);
void watchdog_guard_force_reason(watchdog_guard_t *guard, watchdog_guard_reason_t reason);

#ifdef __cplusplus
}
#endif

#endif /* WATCHDOG_GUARD_H */
