#include "arch/xtensa/interrupts/interrupts.h"
#include "kernel/scheduler/scheduler.h"

extern uint8_t _vector_base[];
extern void enable_irq(uint32_t mask);

void init_interrupts(void) {
    __asm__ volatile (
        "wsr %0, vecbase\n"
        "isync\n"
        :
        : "r" (_vector_base)
        : "memory"
    );

    enable_irq(1 << 6);
}

static const char* get_exception_cause_string(uint32_t cause) {
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
}

void c_interrupt_handler(interrupt_context_t *ctx) {
    if (ctx->exccause == 4) {
        sched_tick(ctx);
        return;
    }

    uart_print("[PANIC] Interrupt detected ! ");
    uart_print(get_exception_cause_string(ctx->exccause));
    uart_print("\n");

    while (1) {
    }
}
