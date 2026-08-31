#include "kernel/scheduler/scheduler.h"

static task_t   tasks[SCHED_MAX_TASKS];
static int      current_task = -1;
static size_t      task_count   = 0;
static uint32_t slice_ticks  = 0;

extern void set_cpu_private_timer(int timer, uint32_t delta);

void sched_init(void) {
    for (size_t i = 0; i < SCHED_MAX_TASKS; i++) {
        tasks[i].state = TASK_UNUSED;
    }

    current_task = -1;
    task_count = 0;
}

int sched_create_task(task_entry_t entry, size_t stack_size) {
    if (task_count >= SCHED_MAX_TASKS) {
        return -1;
    }

    void *stack = nmap(stack_size);
    if (stack == NULL) {
        return -1;
    }

    int id = task_count;
    task_count = task_count + 1;
    task_t *task = &tasks[id];

    uint32_t *raw = (uint32_t *) &task->ctx;
    for (int r = 0; r < (int) (sizeof(task->ctx) / sizeof(uint32_t)); r++) {
        raw[r] = 0;
    }

    task->ctx.a1   = (uint32_t) ((uint8_t *) stack + stack_size);
    task->ctx.epc1 = (uint32_t) entry;
    task->ctx.ps   = 0;

    task->state = TASK_READY;

    return id;
}

void sched_tick(interrupt_context_t *ctx) {
    set_cpu_private_timer(0, slice_ticks);

    if (task_count == 0) {
        return;
    }

    if (current_task >= 0) {
        tasks[current_task].ctx = *ctx;
    }

    int current = current_task;
    for (size_t i = 0; i < task_count; i++) {
        current = (current + 1) % task_count;
        if (tasks[current].state == TASK_READY) {
            break;
        }
    }

    current_task = current;
    *ctx = tasks[current].ctx;
}

void sched_start(uint32_t ticks_per_slice) {
    slice_ticks = ticks_per_slice;
    set_cpu_private_timer(0, ticks_per_slice);

    while (1) {
    }
}
