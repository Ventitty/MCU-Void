#include "arch/xtensa/interrupts/interrupts.h"
#include "kernel/scheduler/scheduler.h"

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

/* static const char* get_exception_cause_string(uint32_t cause) {
    switch (cause) {
        case 0:  return "Illegal Instruction";
        case 1:  return "System Call (Syscall)";
        case 2:  return "Instruction Fetch Error";
        case 3:  return "Load/Store Error";
        case 4:  return "Level-1 Interrupt";
        case 5:  return "Alloca Exception";
        case 6:  return "Integer Divide By Zero";
        case 9:  return "Load/Store Alignment Error";
        case 12: return "PIF Data Error";
        case 28: return "Load Prohibit (Null pointer read / Invalid memory access)";
        case 29: return "Store Prohibit (Null pointer write / Protected memory write)";
        default: return "Unknown / Reserved Exception";
    }
} */

interrupt_context_t* c_interrupt_handler(interrupt_context_t *ctx) {
    uint32_t cause;
    __asm__ volatile ("rsr %0, interrupt" : "=r"(cause));

    if (cause & (1u << 6)) {
        set_cpu_private_timer(0, 2000000);
        return sched_tick(ctx);
    }

    return ctx;
}
