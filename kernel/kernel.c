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

void c_interrupt_handler(interrupt_context_t *ctx) {
    uart_print("\n\r[PANIC] Exception !\n\r");

    if (ctx->exccause != 0) {
        if (ctx->exccause == 6) {
            uart_print("\n\r========================================\n\r");
            uart_print("[PANIC] EXCEPTION : Division par ZERO !\n\r");
            uart_print("========================================\n\r");

            while(1);
        } else if (UART0_INT_ST & (1 << 0)) {
            while ((UART0_STATUS & 0x3FF) > 0) {
                char c = *UART0_FIFO;
                *UART0_FIFO = c;

                if (c == '0') {
                    trigger_div_zero = 1;
                }
            }
            UART0_INT_CLR = 0xFFFF;

            xtensa_clear_interrupts(1 << 1);
        }
    } else {
        uart_print("\n\r[PANIC] Exception inconnue !\n\r");

        while(1);
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

    uart_print("Kernel demarre. Tapez du texte pour l'echo.\n\r");
    uart_print("Tapez '0' pour provoquer une division par zero.\n\r> ");

    volatile int a = 42;
    volatile int b = 0;

    while (1) {
        if (trigger_div_zero) {
            trigger_div_zero = 0;

            a = a / b;
        }
    }
}
