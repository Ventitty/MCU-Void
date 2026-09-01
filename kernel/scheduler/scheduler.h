#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "kernel/types.h"
#include "kernel/utils.h"
#include "arch/xtensa/interrupts/interrupts.h"
#include "kernel/memory_manager/mmu.h"

#define SCHED_MAX_TASKS 8

typedef enum {
    TASK_UNUSED = 0,
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_SUSPENDED,
} task_state_t;

typedef struct {
    interrupt_context_t ctx;
    task_state_t state;
    void    *stack_base;
    size_t   stack_size;
    int      pid;
    int      parent_pid;
    uint32_t            sleep_ticks;
} task_t;

typedef void (*task_entry_t)(void);

void sched_init(void);
int sched_create_task(task_entry_t entry, size_t stack_size);
interrupt_context_t* sched_tick(interrupt_context_t *ctx);
void sched_start(uint32_t ticks_per_slice);

int sched_fork(interrupt_context_t *parent_ctx);
int sched_get_current_pid(void);
int fork(void);

void task_exit(void);
void sleep_ms(uint32_t ms);
void sched_yield(void);

#endif /* SCHEDULER_H */
