#include "kernel/memory_manager/mmu.h"
#include "arch/xtensa/interrupts/interrupts.h"
#include "kernel/types.h"
#include "kernel/utils.h"
#include "kernel/scheduler/scheduler.h"
#include "arch/xtensa/drivers/cores/smp.h"
#include "kernel/scheduler/spinlock.h"

/* Registres UART0 ESP32 */
#define UART0_BASE        0x3FF40000u
#define UART0_FIFO_REG    (*(volatile uint32_t *)(UART0_BASE + 0x00))
#define UART0_STATUS_REG  (*(volatile uint32_t *)(UART0_BASE + 0x1C))

/* Compteur d'octets dans la FIFO RX */
#define UART0_RX_FIFO_CNT (UART0_STATUS_REG & 0xFFu)

volatile int trigger_div_zero = 0;
static spinlock_t uart_lock = SPINLOCK_INIT;

/* Corrigé : Accès 32-bit obligatoire sur le bus APB ESP32 */
void uart_putchar(char c) {
    UART0_FIFO_REG = (uint32_t)(uint8_t)c;
}

void uart_print(const char *str) {
    uint32_t saved = spinlock_acquire_irqsave(&uart_lock);

    while (*str != '\0') {
        if (*str == '\n') {
            uart_putchar('\r');
        }
        uart_putchar(*str);
        str++;
    }

    spinlock_release_irqrestore(&uart_lock, saved);
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

void echo(void) {
    uart_print("Kernel is UP ! Input text to echo it.\n\r");
    uart_print("Input > ");

    while (1) {
        if (UART0_RX_FIFO_CNT > 0) {
            char c = (char)(UART0_FIFO_REG & 0xFFu);

            if (c == '\r' || c == '\n') {
                uart_putchar('\r');
                uart_putchar('\n');
                uart_print("Input > ");
            } else if (c == '\b' || c == 0x7F) {
                uart_print("\b \b");
            } else {
                uart_putchar(c);
            }
        }

        sched_yield();
    }
}

static volatile int core1_ipi_received = 0;

static void core1_ipi_handler(void) {
    core1_ipi_received = 1;
}

void core1_task(void) {
    /* Activer la ligne d'interruption IPI (bit 7) dans INTENABLE pour le Coeur 1 */
    uint32_t intenable;
    __asm__ volatile ("rsr.intenable %0" : "=r"(intenable));
    intenable |= (1u << 7);
    __asm__ volatile ("wsr.intenable %0; rsync" :: "r"(intenable));

    smp_ipi_register_handler(0, core1_ipi_handler);

    uart_print("[core1] demarre, en attente d'une IPI du coeur 0...\n\r");

    while (!core1_ipi_received) {
        __asm__ volatile ("waiti 0");
    }

    uart_print("[core1] IPI recue ! Le coeur 0 est bien arrive jusqu'ici.\n\r");

    int counter = 0;
    while (1) {
        uart_print("[core1] vivant, compteur = ");
        uart_print_int(counter++);
        uart_print("\n\r");
        delay(20000000);
    }
}

void kernel(void) {
    init_interrupts();
    mm_init();
    sched_init();

    /* Activer les lignes d'interruption Timer (bit 6) et IPI (bit 7) dans INTENABLE */
    uint32_t intenable = (1u << 6) | (1u << 7);
    __asm__ volatile ("wsr.intenable %0; rsync" :: "r"(intenable));

    uart_print("Kernel is starting...\n\r");

    const char *msg = "Hello world !\n";
    size_t len = 0;
    while (msg[len] != '\0') len++;

    char *alloc = nmap(len + 1);
    if (alloc == NULL) {
        uart_print("ERREUR : Allocation memoire (nmap) a echoue !\n");
        while(1);
    }

    size_t i = 0;
    for (; i < len; i++) alloc[i] = msg[i];
    alloc[i] = '\0';

    uart_print(alloc);

    /* Enregistrement de la tâche echo */
    sched_create_task(echo, 4096);

    uart_print("[1] avant smp_start_core1\n\r");
    smp_start_core1(core1_task);
    uart_print("[2] apres smp_start_core1\n\r");

    delay(5000000);
    smp_send_ipi(1);
    uart_print("[3] apres smp_send_ipi\n\r");

    uart_print("[core0] id = ");
    uart_print_int(smp_get_core_id());
    uart_print("\n\r");

    /* Démarrage de l'ordonnanceur */
    sched_start(2000000);
}
