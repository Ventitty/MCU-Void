#include "kernel/memory_manager/mmu.h"

#define UART0_FIFO                ((volatile uint8_t *)0x60000000)
#define UART0_STATUS ((volatile uint32_t *)0x6000001C)

void uart_putchar(char c) {
    *UART0_FIFO = c;

    for (volatile int i = 0; i < 1000; i++) {
        __asm__ __volatile__("nop");
    }
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

void uart_print_size(size_t n) {
    if (n == 0) {
        uart_putchar('0');
        return;
    }

    char buffer[21];
    int i = 0;

    while (n > 0) {
        buffer[i++] = '0' + (n % 10);
        n /= 10;
    }

    while (i > 0) {
        uart_putchar(buffer[--i]);
    }

    uart_putchar('\r');
    uart_putchar('\n');
}

void kernel(void) {
    mm_init();

    const char *msg = "Hello world !\n";

    size_t len = 0;
    while (msg[len] != '\0') {
        len++;
    }

    uart_print("Longueur détectée : ");
    uart_print_size(len);
    uart_print("\n---------------------\n");

    char *alloc = nmap(len + 1);

    size_t i = 0;
    for (; i < len; i++) {
        alloc[i] = msg[i];
    }
    alloc[len] = '\0';

    uart_print(alloc);

    while (1) {
    }
}
