#include "kernel/memory_manager/mmu.h"
#include "arch/xtensa/interrupts/interrupts.h"
#include "kernel/types.h"
#include "kernel/utils.h"
#include "kernel/scheduler/scheduler.h"
#include "arch/xtensa/drivers/cores/smp.h"
#include "kernel/scheduler/spinlock.h"

#define UART0_FIFO                ((volatile uint8_t *)0x60000000)

volatile int trigger_div_zero = 0;

static spinlock_t uart_lock = SPINLOCK_INIT;

void uart_putchar(char c) {
    *UART0_FIFO = c;
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

void test_task(void) {
    int pid = fork();

    if (pid == 0) {
        uart_print("[enfant] toujours vivant\n");

        while (1) {
            delay(10000000);
        }
    } else if (pid > 0) {
        uart_print("[parent] a cree un enfant\n");

        while (1) {
            uart_print_int(pid);
            delay(10000000);
        }
    } else {
        uart_print("fork() a echoue\n");
        while (1);
    }
}

void short_lived_task(void) {
    uart_print("[CHILD] PID ");
    uart_print_int(sched_get_current_pid());
    uart_print(" va s'éteindre...\n\r");
}

void test_task_recycling(void) {
    uart_print("[TEST] Démarrage du test de recyclage de slots...\n");

    for (int i = 0; i < 50; i++) {
        int pid = sched_create_task(short_lived_task, 1024);

        if (pid < 0) {
            uart_print("[TEST ECHEC] Plus de slots disponibles à l'itération ");
            uart_print_int(i);
            uart_print("\n");
            return;
        }

        sleep_ms(1);
    }

    uart_print("[TEST SUCCES] 50 tâches exécutées et nettoyées sans pénurie de slots !\n");
}

void test_fork_isolation(void) {
    uart_print("[TEST FORK] Démarrage du test d'isolation de la pile...\n\r");

    volatile int stack_var = 42;

    int pid = fork();

    if (pid < 0) {
        uart_print("[TEST ECHEC] fork() a échoué (mémoire ou slots insuffisants)\n\r");
    } else if (pid == 0) {
        uart_print("[ENFANT] PID ");
        uart_print_int(sched_get_current_pid());
        uart_print(" : Modification de stack_var (42 -> 99)...\n\r");

        stack_var = 99;
        sleep_ms(1);

        if (stack_var == 99) {
            uart_print("[ENFANT] Variable locale vérifiée avec succès dans l'enfant.\n\r");
        } else {
            uart_print("[TEST ECHEC] Pile corrompue dans l'enfant !\n\r");
        }

    } else {
        uart_print("[PARENT] Enfant créé avec PID ");
        uart_print_int(pid);
        uart_print(". Attente de la fin de l'enfant...\n\r");

        sleep_ms(5);

        if (stack_var == 42) {
            uart_print("[PARENT] stack_var est toujours égale à 42 !\n\r");
            uart_print("[TEST SUCCES] Isolation de la pile validée pour fork() !\n\r");
        } else {
            uart_print("[TEST ECHEC] La variable du parent a été altérée par l'enfant !\n\r");
        }
    }
}

void echo(void) {
    uart_print("Kernel is UP ! Input text to echo it.\n\r");
    uart_print("Input > ");

    while (1) {

        if ((UART0_STATUS & 0x1FF) > 0) {
            char c = *UART0_FIFO;

            if (c == '\r' || c == '\n') {
                uart_putchar('\r');
                uart_putchar('\n');
                uart_print("Input > ");
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
    smp_ipi_register_handler(0, core1_ipi_handler); /* IPI venant du cœur 0 */

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

    uart_print("Kernel is starting...\n\r");

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

    /*int t1 = sched_create_task(test_task, 4096);
     *    int t2 = sched_create_task(test_task, 4096);
     *    int t3 = sched_create_task(test_task, 4096);
     *
     *
     *    uart_print("[PID]: ");
     *    uart_print_int(t1);
     *    uart_print(".\n\r");
     *
     *    uart_print("[PID]: ");
     *    uart_print_int(t2);
     *    uart_print(".\n\r");
     *
     *    uart_print("[PID]: ");
     *    uart_print_int(t3);
     *    uart_print(".\n\r");*/

    //sched_create_task(test_task_recycling, 4096);
    //sched_create_task(test_fork_isolation, 4096);

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

    sched_start(2000000);
}
