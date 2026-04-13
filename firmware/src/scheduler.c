#include "scheduler.h"
#include <stddef.h>

void scheduler_init(scheduler_state_t *state, scheduler_task_t *tasks, size_t task_count)
{
    if (state == NULL) {
        return;
    }

    state->tasks = tasks;
    state->task_count = task_count;
    state->tick_ms = 0;
}

void scheduler_reset(scheduler_state_t *state)
{
    if (state == NULL) {
        return;
    }

    state->tick_ms = 0;
}

void scheduler_tick(scheduler_state_t *state, uint32_t now_ms)
{
    if (state == NULL) {
        return;
    }

    /* Stub only: task execution order and callback dispatch are added later. */
    state->tick_ms = now_ms;
}

bool scheduler_register_task(scheduler_state_t *state, size_t slot, const scheduler_task_t *task)
{
    if (state == NULL || task == NULL || state->tasks == NULL || slot >= state->task_count) {
        return false;
    }

    state->tasks[slot] = *task;
    return true;
}

bool scheduler_enable_task(scheduler_state_t *state, uint16_t task_id, bool enable)
{
    size_t i;

    if (state == NULL || state->tasks == NULL) {
        return false;
    }

    for (i = 0; i < state->task_count; ++i) {
        if (state->tasks[i].task_id == task_id) {
            state->tasks[i].enabled = enable ? 1u : 0u;
            return true;
        }
    }

    return false;
}
