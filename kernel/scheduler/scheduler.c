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

    int id = task_count++;
    task_t *task = &tasks[id];

    uint32_t *raw = (uint32_t *) &task->ctx;
    for (int r = 0; r < (int) (sizeof(task->ctx) / sizeof(uint32_t)); r++) {
        raw[r] = 0;
    }

    /* Sommet de pile aligné sur 16 octets (masque ~0xF) */
    uint32_t top_of_stack = ((uint32_t)stack + stack_size) & ~0xF;

    task->ctx.a1   = top_of_stack;
    task->ctx.epc1 = (uint32_t) entry;
    task->ctx.ps   = 0x00040000; /* WOE = 1, INTLEVEL = 0 */

    task->stack_base  = stack;
    task->stack_size  = stack_size;
    task->pid         = id;
    task->parent_pid  = -1;
    task->state       = TASK_READY;

    return id;
}

int sched_get_current_pid(void) {
    return current_task;
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

    /* Arme le premier tick */
    set_cpu_private_timer(0, ticks_per_slice);

    /* Active WOE (bit 18) et démasque les interruptions (INTLEVEL = 0) */
    __asm__ volatile (
        "wsr.ps %0\n\t"
        "rsync"
        :
        : "r"(0x00040000)
        : "memory"
    );

    /* Attente passive de la première interruption */
    while (1) {
        __asm__ volatile ("waiti 0");
    }
}

int sched_fork(interrupt_context_t *parent_ctx) {
    if (current_task < 0) {
        return -1;
    }
    if (task_count >= SCHED_MAX_TASKS) {
        return -1;
    }

    task_t *parent = &tasks[current_task];

    void *new_stack = nmap(parent->stack_size);
    if (new_stack == NULL) {
        return -1;
    }

    memcpy(new_stack, parent->stack_base, parent->stack_size);

    long delta = (uint8_t *) new_stack - (uint8_t *) parent->stack_base;

    int id = task_count++;
    task_t *child = &tasks[id];

    child->ctx        = *parent_ctx;
    child->ctx.a1      = parent_ctx->a1 + delta;
    child->ctx.epc1    = parent_ctx->a0;
    child->ctx.a2      = 0;

    child->stack_base  = new_stack;
    child->stack_size  = parent->stack_size;
    child->pid         = id;
    child->parent_pid  = parent->pid;
    child->state       = TASK_READY;

    return id;
}

int fork(void) {
    interrupt_context_t snap;

    __asm__ volatile ("mov %0, a0"  : "=r"(snap.a0));
    __asm__ volatile ("mov %0, a1"  : "=r"(snap.a1));
    __asm__ volatile ("mov %0, a2"  : "=r"(snap.a2));
    __asm__ volatile ("mov %0, a3"  : "=r"(snap.a3));
    __asm__ volatile ("mov %0, a4"  : "=r"(snap.a4));
    __asm__ volatile ("mov %0, a5"  : "=r"(snap.a5));
    __asm__ volatile ("mov %0, a6"  : "=r"(snap.a6));
    __asm__ volatile ("mov %0, a7"  : "=r"(snap.a7));
    __asm__ volatile ("mov %0, a8"  : "=r"(snap.a8));
    __asm__ volatile ("mov %0, a9"  : "=r"(snap.a9));
    __asm__ volatile ("mov %0, a10" : "=r"(snap.a10));
    __asm__ volatile ("mov %0, a11" : "=r"(snap.a11));
    __asm__ volatile ("mov %0, a12" : "=r"(snap.a12));
    __asm__ volatile ("mov %0, a13" : "=r"(snap.a13));
    __asm__ volatile ("mov %0, a14" : "=r"(snap.a14));
    __asm__ volatile ("mov %0, a15" : "=r"(snap.a15));

    __asm__ volatile ("rsr.sar %0"  : "=r"(snap.sar));
    __asm__ volatile ("rsr.ps  %0"  : "=r"(snap.ps));

    snap.exccause = 4;

    return sched_fork(&snap);
}
