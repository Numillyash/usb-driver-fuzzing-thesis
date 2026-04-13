#include "watchdog_guard.h"

#include <stddef.h>

void watchdog_guard_init(watchdog_guard_t *guard, uint32_t timeout_ms)
{
    if (guard == NULL) {
        return;
    }

    guard->timeout_ms = timeout_ms;
    guard->last_kick_ms = 0;
    guard->last_check_ms = 0;
    guard->trip_count = 0;
    guard->state = WATCHDOG_GUARD_OK;
    guard->reason = WATCHDOG_GUARD_REASON_NONE;
}

void watchdog_guard_kick(watchdog_guard_t *guard, uint32_t now_ms)
{
    if (guard == NULL) {
        return;
    }

    guard->last_kick_ms = now_ms;
    guard->last_check_ms = now_ms;
    guard->state = WATCHDOG_GUARD_OK;
    guard->reason = WATCHDOG_GUARD_REASON_NONE;
}

watchdog_guard_state_t watchdog_guard_poll(watchdog_guard_t *guard, uint32_t now_ms)
{
    if (guard == NULL) {
        return WATCHDOG_GUARD_EXPIRED;
    }

    guard->last_check_ms = now_ms;

    if (guard->timeout_ms == 0) {
        guard->state = WATCHDOG_GUARD_OK;
        return WATCHDOG_GUARD_OK;
    }

    if ((now_ms - guard->last_kick_ms) >= guard->timeout_ms) {
        guard->state = WATCHDOG_GUARD_EXPIRED;
        guard->trip_count++;
    }

    return (watchdog_guard_state_t)guard->state;
}

void watchdog_guard_force_reason(watchdog_guard_t *guard, watchdog_guard_reason_t reason)
{
    if (guard == NULL) {
        return;
    }

    guard->reason = (uint8_t)reason;
}
