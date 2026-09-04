#include "kernel/scheduler/scheduler.h"
#include "kernel/utils.h"

static task_t   tasks[SCHED_MAX_TASKS];
static int      current_task = -1;
static uint32_t slice_ticks  = 0;

static interrupt_context_t idle_ctx;
static uint32_t idle_stack[256];

extern void set_cpu_private_timer(int timer, uint32_t delta);

static void idle_loop(void) {
    while (1) {
        __asm__ volatile ("waiti 0");
    }
}

void sched_init(void) {
    for (size_t i = 0; i < SCHED_MAX_TASKS; i++) {
        tasks[i].state = TASK_UNUSED;
        tasks[i].sleep_ticks = 0;
    }

    current_task = -1;

    memset(&idle_ctx, 0, sizeof(idle_ctx));
    idle_ctx.a1   = ((uint32_t) idle_stack + sizeof(idle_stack)) & ~0xFu;
    idle_ctx.epc1 = (uint32_t) idle_loop;
    idle_ctx.ps   = 0x00000020u; /* Mode Call0 ABI */
}

static void task_wrapper(void) {
    if (current_task >= 0 && tasks[current_task].entry != NULL) {
        tasks[current_task].entry();
    }
    task_exit();
}

int sched_create_task(task_entry_t entry, size_t stack_size) {
    int id = -1;
    for (int i = 0; i < SCHED_MAX_TASKS; i++) {
        if (tasks[i].state == TASK_UNUSED) {
            id = i;
            break;
        }
    }
    if (id == -1) return -1;

    void *stack = nmap(stack_size);
    if (stack == NULL) return -1;

    task_t *task = &tasks[id];

    memset(&task->ctx, 0, sizeof(task->ctx));

    uint32_t top_of_stack = ((uint32_t)stack + stack_size) & ~0xFu;

    task->entry        = entry;
    task->ctx.a1       = top_of_stack;
    task->ctx.epc1     = (uint32_t) task_wrapper;
    task->ctx.a0       = (uint32_t) task_exit;
    task->ctx.ps       = 0x00000020u; /* Mode Call0 ABI */
    task->stack_base   = stack;
    task->stack_size   = stack_size;
    task->pid          = id;
    task->parent_pid   = -1;
    task->sleep_ticks  = 0;
    task->state        = TASK_READY;

    return id;
}

int sched_get_current_pid(void) {
    return current_task;
}

interrupt_context_t* sched_tick(interrupt_context_t *ctx) {
    set_cpu_private_timer(0, slice_ticks);

    /* 1. Décrémentation du sommeil et nettoyage des tâches mortes */
    for (int i = 0; i < SCHED_MAX_TASKS; i++) {
        if (tasks[i].state == TASK_BLOCKED) {
            if (tasks[i].sleep_ticks > 0) {
                tasks[i].sleep_ticks--;
            }

            if (tasks[i].sleep_ticks == 0) {
                tasks[i].state = TASK_READY;
            }
        } else if (tasks[i].state == TASK_DEAD) {
            if (tasks[i].stack_base != NULL) {
                unmap(tasks[i].stack_base);
                tasks[i].stack_base = NULL;
            }
            tasks[i].state = TASK_UNUSED;
        }
    }

    /* 2. Sauvegarde du contexte sortant */
    if (current_task >= 0 && current_task < SCHED_MAX_TASKS) {
        if (tasks[current_task].state != TASK_UNUSED) {
            tasks[current_task].ctx = *ctx;
            if (tasks[current_task].state == TASK_RUNNING) {
                tasks[current_task].state = TASK_READY;
            }
        }
    }

    /* 3. Élection du prochain processus (Round-Robin) */
    int start = (current_task >= 0) ? current_task : 0;
    int next_task = start;
    int found = 0;

    for (int i = 0; i < SCHED_MAX_TASKS; i++) {
        next_task = (next_task + 1) % SCHED_MAX_TASKS;
        if (tasks[next_task].state == TASK_READY) {
            found = 1;
            break;
        }
    }

    if (found) {
        current_task = next_task;
        tasks[current_task].state = TASK_RUNNING;
        return &tasks[current_task].ctx;
    }

    if (current_task >= 0 && tasks[current_task].state == TASK_RUNNING) {
        return &tasks[current_task].ctx;
    }

    current_task = -1;
    return &idle_ctx;
}

void sched_start(uint32_t ticks_per_slice) {
    slice_ticks = ticks_per_slice;
    set_cpu_private_timer(0, ticks_per_slice);

    /* Activation du Timer (bit 6) + IPI/UART (bit 7) et écriture de PS = 0x00000020 */
    __asm__ volatile (
        "wsr %0, intenable\n\t"
        "wsr %1, ps\n\t"
        "rsync\n\t"
        :: "r"((1u << 6) | (1u << 7)), "r"(0x00000020u)
        : "memory"
    );

    while (1) {
        __asm__ volatile ("waiti 0");
    }
}

int sched_fork(interrupt_context_t *parent_ctx) {
    if (current_task < 0) {
        return -1;
    }

    int id = -1;
    for (int i = 0; i < SCHED_MAX_TASKS; i++) {
        if (tasks[i].state == TASK_UNUSED) {
            id = i;
            break;
        }
    }
    if (id == -1) return -1;

    task_t *parent = &tasks[current_task];

    void *new_stack = nmap(parent->stack_size);
    if (new_stack == NULL) {
        return -1;
    }

    memcpy(new_stack, parent->stack_base, parent->stack_size);

    long delta = (uint8_t *) new_stack - (uint8_t *) parent->stack_base;

    task_t *child = &tasks[id];

    child->ctx         = *parent_ctx;
    child->ctx.a1      = parent_ctx->a1 + delta;
    child->ctx.epc1    = parent_ctx->a0;
    child->ctx.a2      = 0;
    child->ctx.ps      = 0x00000020u;

    child->stack_base  = new_stack;
    child->stack_size  = parent->stack_size;
    child->pid         = id;
    child->parent_pid  = parent->pid;
    child->sleep_ticks = 0;
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
    snap.ps = 0x00000020u;
    snap.exccause = 4;

    return sched_fork(&snap);
}

void sched_yield(void) {
    uint32_t now;
    __asm__ volatile ("rsr.ccount %0" : "=r"(now));

    __asm__ volatile (
        "wsr %0, ccompare0\n\t"
        "rsync\n\t"
        "waiti 0\n\t"
        :: "r"(now + 1000)
        : "memory"
    );
}

void sleep_ms(uint32_t ms) {
    if (current_task < 0) return;

    tasks[current_task].sleep_ticks = ms;
    tasks[current_task].state = TASK_BLOCKED;

    sched_yield();
}

void task_exit(void) {
    if (current_task >= 0 && current_task < SCHED_MAX_TASKS) {
        tasks[current_task].state = TASK_DEAD;
    }
    sched_yield();

    while (1) {
        __asm__ volatile ("waiti 0");
    }
}

int sched_suspend(int pid) {
    if (pid < 0 || pid >= SCHED_MAX_TASKS) {
        return -1;
    }
    if (tasks[pid].state == TASK_UNUSED) {
        return -1;
    }

    tasks[pid].state = TASK_SUSPENDED;

    if (pid == current_task) {
        sched_yield();
    }

    return 0;
}

int sched_resume(int pid) {
    if (pid < 0 || pid >= SCHED_MAX_TASKS) {
        return -1;
    }
    if (tasks[pid].state != TASK_SUSPENDED) {
        return -1;
    }

    tasks[pid].state = TASK_READY;

    return 0;
}
