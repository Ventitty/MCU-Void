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
            }
        }
    }
}
