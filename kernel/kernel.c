#include "kernel/memory_manager/mmu.h"
#include "arch/xtensa/interrupts/interrupts.h"
#include "kernel/types.h"
#include "kernel/utils.h"
#include "kernel/scheduler/scheduler.h"

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

void uart_print_int(int num) {
    char buf[12];
    int i = 0;
    unsigned int u;

    if (num == 0) {
        uart_putchar('0');
        return;
    }

    if (num < 0) {
        uart_putchar('-');
        u = (unsigned int)(-(num + 1)) + 1;
    } else {
        u = (unsigned int)num;
    }

    while (u > 0) {
        buf[i++] = '0' + (u % 10);
        u /= 10;
    }

    while (i > 0) {
        uart_putchar(buf[--i]);
    }
}

static void delay(volatile uint32_t count) {
    while (count--) {
        __asm__ volatile ("nop");
    }
}

void test_task(void) {
    uart_print("[test_task]\n");
    int pid = fork();

    if (pid == 0) {
        while (1) {
            uart_print("[enfant] toujours vivant\n");
            delay(1000000);
        }
    } else if (pid > 0) {
        while (1) {
            uart_print("[parent] a cree un enfant\n");
            delay(1000000);
        }
    } else {
        uart_print("fork() a echoue\n");
        while (1);
    }
}

void kernel(void) {
    init_interrupts();
    mm_init();
    sched_init();

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

    int t1 = sched_create_task(test_task, 4096);

    uart_print("[PID]: ");
    uart_print_int(t1);
    uart_print(".\n\r");

    sched_start(1000000);

    while (1) {
        while (1) {
            if ((UART0_STATUS & 0x1FF) > 0) {
                char c = *UART0_FIFO;
                uart_putchar(c);
            }
        }
    }

}
