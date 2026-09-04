#include "arch/xtensa/interrupts/interrupts.h"
#include "kernel/scheduler/scheduler.h"
#include "arch/xtensa/drivers/cores/smp.h"

extern uint8_t _vector_base[];
extern void set_cpu_private_timer(int timer, uint32_t delta);

void init_interrupts(void) {
    __asm__ volatile (
        "wsr %0, vecbase\n\t"
        "rsync"
        :: "r"(_vector_base)
        : "memory"
    );
}

interrupt_context_t* c_interrupt_handler(interrupt_context_t *ctx) {
    uint32_t cause;
    __asm__ volatile ("rsr %0, interrupt" : "=r"(cause));

    /* Interruption du Timer CPU (Ligne 6) */
    if (cause & (1u << 6)) {
        set_cpu_private_timer(0, 2000000);
        return sched_tick(ctx);
    }

    /* Interruption inter-cœurs IPI (Ligne 7) */
    if (cause & (1u << 7)) {
        smp_ipi_dispatch_local();
        return ctx;
    }

    return ctx;
}
