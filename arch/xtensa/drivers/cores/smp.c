#include "arch/xtensa/drivers/cores/smp.h"

/* Registres RTC CNTL (ESP32) */
#define RTC_CNTL_OPTIONS0_REG          (*(volatile uint32_t *) 0x3FF48000u)
#define RTC_CNTL_SW_STALL_APPCPU_C0_M  0x3u

#define RTC_CNTL_SW_CPU_STALL_REG      (*(volatile uint32_t *) 0x3FF480ACu)
#define RTC_CNTL_SW_STALL_APPCPU_C1_M  (0x3Fu << 20)

/* Registres DPORT pour le contrôle du Coeur 1 / APP CPU (ESP32) */
#define DPORT_APPCPU_BOOT_ADDR_REG     (*(volatile uint32_t *) 0x3FF000C8u)
#define DPORT_APPCPU_CTRL_A_REG        (*(volatile uint32_t *) 0x3FF0002Cu)
#define DPORT_APPCPU_CTRL_B_REG        (*(volatile uint32_t *) 0x3FF00030u)

#define DPORT_APPCPU_RESETTING         (1u << 0)
#define DPORT_APPCPU_CLKGATE_EN        (1u << 0)

/* Registres IPI (Software Interrupts DPORT) */
#define DPORT_CPU_INTR_FROM_CPU_0_REG  (*(volatile uint32_t *) 0x3FF000DCu)
#define DPORT_CPU_INTR_FROM_CPU_1_REG  (*(volatile uint32_t *) 0x3FF000E0u)
#define DPORT_CPU_INTR_FROM_CPU_0      (1u << 0)
#define DPORT_CPU_INTR_FROM_CPU_1      (1u << 0)

/* Matrice d'interruptions ESP32 (PRO CPU = Core 0, APP CPU = Core 1) */
#define DPORT_PRO_CPU_INTR_FROM_CPU_1_MAP_REG (*(volatile uint32_t *) 0x3FF101B8u)
#define DPORT_APP_CPU_INTR_FROM_CPU_0_MAP_REG (*(volatile uint32_t *) 0x3FF111B4u)

#define IPI_CPU_LINE 7

extern void _core1_entry(void);

static void (*core1_user_entry)(void) = NULL;
static ipi_handler_t ipi_handlers[2];

int smp_get_core_id(void) {
    uint32_t prid;
    __asm__ volatile ("rsr.prid %0" : "=r"(prid));
    return (int) ((prid >> 13) & 1u);
}

static void mark(char c) {
    /* Adresse FIFO de l'UART0 sur ESP32 classique */
    *((volatile uint8_t *) 0x3FF40000u) = (uint8_t) c;
}

#define RTC_CNTL_STORE5_REG (*(volatile uint32_t *) 0x3FF480B4u)

void smp_start_core1(void (*entry)(void)) {
    core1_user_entry = entry;

    /* Routage IPI pour le cœur 0 */
    DPORT_PRO_CPU_INTR_FROM_CPU_1_MAP_REG = IPI_CPU_LINE;

    /* 1. Adresse de boot lue par la ROM ESP32 / QEMU pour le cœur 1 */
    RTC_CNTL_STORE5_REG = (uint32_t) _core1_entry;
    DPORT_APPCPU_BOOT_ADDR_REG = (uint32_t) _core1_entry;

    /* 2. Déblocage du stall RTC */
    RTC_CNTL_OPTIONS0_REG &= ~RTC_CNTL_SW_STALL_APPCPU_C0_M;
    RTC_CNTL_SW_CPU_STALL_REG &= ~RTC_CNTL_SW_STALL_APPCPU_C1_M;

    /* 3. Déblocage DPORT du cœur 1 */
    DPORT_APPCPU_CTRL_B_REG |= DPORT_APPCPU_CLKGATE_EN;
    DPORT_APPCPU_CTRL_A_REG |= DPORT_APPCPU_RESETTING;
    DPORT_APPCPU_CTRL_A_REG &= ~DPORT_APPCPU_RESETTING;
}

void core1_main(void) {
    mark('1');

    /* Routage IPI pour le cœur 1 (APP CPU) : écoute des interruptions émises par le cœur 0 */
    DPORT_APP_CPU_INTR_FROM_CPU_0_MAP_REG = IPI_CPU_LINE;

    /* Alignement du registre d'état PS pour Call0 ABI (INTLEVEL=0, EXCM=0) */
    uint32_t ps = 0x00000020;
    __asm__ volatile ("wsr %0, ps; rsync" :: "r"(ps));

    if (core1_user_entry != NULL) {
        core1_user_entry();
    }

    for (;;) {
        __asm__ volatile ("waiti 0");
    }
}

void smp_send_ipi(int core_id) {
    if (core_id == smp_get_core_id()) {
        return;
    }

    if (core_id == 0) {
        DPORT_CPU_INTR_FROM_CPU_0_REG = DPORT_CPU_INTR_FROM_CPU_0;
    } else {
        DPORT_CPU_INTR_FROM_CPU_1_REG = DPORT_CPU_INTR_FROM_CPU_1;
    }
}

static void smp_ipi_ack(int from_core) {
    if (from_core == 0) {
        DPORT_CPU_INTR_FROM_CPU_0_REG = 0;
    } else {
        DPORT_CPU_INTR_FROM_CPU_1_REG = 0;
    }
}

void smp_ipi_register_handler(int from_core, ipi_handler_t handler) {
    if (from_core == 0 || from_core == 1) {
        ipi_handlers[from_core] = handler;
    }
}

void smp_ipi_dispatch_local(void) {
    int my_core = smp_get_core_id();
    int from_core = (my_core == 0) ? 1 : 0;

    smp_ipi_ack(from_core);

    if (ipi_handlers[from_core] != NULL) {
        ipi_handlers[from_core]();
    }
}
