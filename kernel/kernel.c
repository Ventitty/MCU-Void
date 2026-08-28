#include "kernel/memory_manager/mmu.h"

#define UART0_FIFO                ((volatile uint8_t *)0x60000000)
#define UART0_STATUS_REG   (*(volatile uint32_t *)0x6000001C)

#define UART_TXFIFO_CNT_SHIFT 16
#define UART_TXFIFO_CNT_MASK  0xFF

void uart_putchar(char c) {
    *UART0_FIFO = c;

    for (volatile int i = 0; i < 1000; i++) {
        __asm__ volatile ("nop");
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

void *memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

void *memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

void uart_print_hex(uint32_t n) {
    uart_print("0x");
    char hex_chars[] = "0123456789ABCDEF";
    char buffer[8];
    // On extrait les 8 nibbles (32 bits)
    for (int i = 0; i < 8; i++) {
        buffer[7 - i] = hex_chars[n & 0xF];
        n >>= 4;
    }
    for (int i = 0; i < 8; i++) {
        uart_putchar(buffer[i]);
    }
    uart_print("\n");
}

void kernel(void) {
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

    while (1) {
    }
}
