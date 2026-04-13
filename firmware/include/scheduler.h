#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*scheduler_task_fn_t)(void *context);

typedef enum {
    SCHEDULER_TASK_DISABLED = 0,
    SCHEDULER_TASK_READY = 1,
    SCHEDULER_TASK_RUNNING = 2
} scheduler_task_state_t;

typedef enum {
    SCHEDULER_TASK_ID_NONE = 0,
    SCHEDULER_TASK_ID_COMMAND = 1,
    SCHEDULER_TASK_ID_STATUS_HEARTBEAT = 2,
    SCHEDULER_TASK_ID_WATCHDOG = 3,
    SCHEDULER_TASK_ID_LOG_FLUSH = 4
} scheduler_task_id_t;

typedef struct {
    scheduler_task_fn_t fn;
    void *context;
    uint32_t period_ms;
    uint32_t next_run_ms;
    uint16_t task_id;
    uint8_t enabled;
    uint8_t reserved0;
} scheduler_task_t;

typedef struct {
    scheduler_task_t *tasks;
    size_t task_count;
    uint32_t tick_ms;
} scheduler_state_t;

void scheduler_init(scheduler_state_t *state, scheduler_task_t *tasks, size_t task_count);
void scheduler_reset(scheduler_state_t *state);
void scheduler_tick(scheduler_state_t *state, uint32_t now_ms);
bool scheduler_register_task(scheduler_state_t *state, size_t slot, const scheduler_task_t *task);
bool scheduler_enable_task(scheduler_state_t *state, uint16_t task_id, bool enable);

#ifdef __cplusplus
}
#endif

#endif /* SCHEDULER_H */
