#include "scheduler.h"

#include <stddef.h>
#include <string.h>

void scheduler_init(scheduler_state_t *state, scheduler_task_t *tasks, size_t task_count)
{
    size_t i;

    if (state == NULL) {
        return;
    }

    state->tasks = tasks;
    state->task_count = task_count;
    state->tick_ms = 0;

    if (tasks == NULL) {
        return;
    }

    for (i = 0; i < task_count; ++i) {
        memset(&tasks[i], 0, sizeof(tasks[i]));
        tasks[i].task_id = SCHEDULER_TASK_ID_NONE;
    }
}

void scheduler_reset(scheduler_state_t *state)
{
    size_t i;

    if (state == NULL) {
        return;
    }

    state->tick_ms = 0;

    if (state->tasks == NULL) {
        return;
    }

    for (i = 0; i < state->task_count; ++i) {
        state->tasks[i].next_run_ms = 0;
        state->tasks[i].enabled = 0;
    }
}

void scheduler_tick(scheduler_state_t *state, uint32_t now_ms)
{
    size_t i;

    if (state == NULL) {
        return;
    }

    state->tick_ms = now_ms;

    if (state->tasks == NULL) {
        return;
    }

    for (i = 0; i < state->task_count; ++i) {
        scheduler_task_t *task = &state->tasks[i];

        if (task->fn == NULL || task->enabled == 0) {
            continue;
        }

        if (task->next_run_ms > now_ms) {
            continue;
        }

        task->fn(task->context);
        task->next_run_ms = now_ms + task->period_ms;
    }
}

bool scheduler_register_task(scheduler_state_t *state, size_t slot, const scheduler_task_t *task)
{
    if (state == NULL || task == NULL || state->tasks == NULL || slot >= state->task_count) {
        return false;
    }

    state->tasks[slot] = *task;
    state->tasks[slot].enabled = task->enabled ? 1u : 0u;
    state->tasks[slot].next_run_ms = state->tick_ms + task->period_ms;

    if (state->tasks[slot].fn == NULL) {
        state->tasks[slot].enabled = 0;
        state->tasks[slot].task_id = SCHEDULER_TASK_ID_NONE;
    }

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
