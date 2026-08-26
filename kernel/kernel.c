#include "kernel/memory_manager/mmu.h"

#define SYSTEM_PERIP_CLK_EN0_REG  (*(volatile uint32_t *)0x600C2018)
#define SYSTEM_PERIP_RST_EN0_REG  (*(volatile uint32_t *)0x600C2014)

#define UART0_FIFO                ((volatile uint32_t *)0x60000000)
#define UART0_CLKDIV_REG          (*(volatile uint32_t *)0x60000014)

void uart_putchar(char c) {
    *UART0_FIFO = c;
}

void uart_print(const char *str) {
    while (*str) {
        if (*str == '\n') {
            uart_putchar('\r');
        }

        uart_putchar(*str);
        str++;
    }
}

void kernel(void) {
    mm_init();

    size_t counter = 1;
    const char *msg = "Hello world !\n";
    uart_print(msg);

    while (counter) {
        counter++;
    }

    uart_print(msg);
}
