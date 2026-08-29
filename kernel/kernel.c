#include "kernel/memory_manager/mmu.h"
#include "arch/xtensa/interrupts/interrupts.h"
#include "kernel/types.h"

#define UART0_FIFO                ((volatile uint8_t *)0x60000000)

volatile int trigger_div_zero = 0;

void uart_putchar(char c) {
    *UART0_FIFO = c;
}

void uart_print(const char *str) {
    while (*str != '\0') {
        if (*str == '\n') {
            uart_putchar('\r');
        }

        uart_putchar(*str);
        str++;
    }
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
        return;
    }

    uart_print("[PANIC] Interrupt detected ! ");
    uart_print(get_exception_cause_string(ctx->exccause));
    uart_print("\n");

    while (1) {
    }
}

void kernel(void) {
    init_interrupts();
    mm_init();

    const char *msg = "Hello world !\n";

    size_t len = 0;
    while (msg[len] != '\0') {
        len++;
    }

    char *alloc = nmap(len + 1);
    if (alloc == NULL) {
        uart_print("ERREUR : Allocation memoire (nmap) a echoue !\n");
        while(1);
    }

    size_t i = 0;
    for (; i < len; i++) {
        alloc[i] = msg[i];
    }
    alloc[i] = '\0';

    uart_print(alloc);

    uart_print("Kernel is UP ! Input text to echo it.\n\r");

    while (1) {
        while (1) {
            if ((UART0_STATUS & 0x1FF) > 0) {
                char c = *UART0_FIFO;
                uart_putchar(c);
                __asm__ volatile("ill");
            }
        }
    }
}
