#include "arch/xtensa/drivers/cores/smp.h"

#define RTC_CNTL_OPTIONS0_REG          (*(volatile uint32_t *) 0x60008000u)
#define RTC_CNTL_SW_STALL_APPCPU_C0_M  0x3u

#define RTC_CNTL_SW_CPU_STALL_REG      (*(volatile uint32_t *) 0x600080BCu)
#define RTC_CNTL_SW_STALL_APPCPU_C1_M  (0x3Fu << 20)

#define SYSTEM_CORE_1_CONTROL_0_REG      (*(volatile uint32_t *) 0x600C0000u)
#define SYSTEM_CONTROL_CORE_1_RUNSTALL   (1u << 0)
#define SYSTEM_CONTROL_CORE_1_CLKGATE_EN (1u << 1)
#define SYSTEM_CONTROL_CORE_1_RESETING   (1u << 2)

#define SYSTEM_CORE_1_CONTROL_1_REG      (*(volatile uint32_t *) 0x600C0004u)

#define SYSTEM_CPU_INTR_FROM_CPU_0_REG (*(volatile uint32_t *) 0x600C0030u)
#define SYSTEM_CPU_INTR_FROM_CPU_1_REG (*(volatile uint32_t *) 0x600C0034u)
#define SYSTEM_CPU_INTR_FROM_CPU_0     (1u << 0)
#define SYSTEM_CPU_INTR_FROM_CPU_1     (1u << 0)

#define INTERRUPT_CORE0_CPU_INTR_FROM_CPU_1_MAP_REG (*(volatile uint32_t *) 0x600C2140u)
#define INTERRUPT_CORE1_CPU_INTR_FROM_CPU_0_MAP_REG (*(volatile uint32_t *) 0x600C293Cu)

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
    *((volatile uint8_t *) 0x60000000u) = (uint8_t) c;
}

void smp_start_core1(void (*entry)(void)) {
    mark('a');
    core1_user_entry = entry;

    INTERRUPT_CORE0_CPU_INTR_FROM_CPU_1_MAP_REG = IPI_CPU_LINE;
    mark('b');

    RTC_CNTL_OPTIONS0_REG &= ~RTC_CNTL_SW_STALL_APPCPU_C0_M;
    RTC_CNTL_SW_CPU_STALL_REG &= ~RTC_CNTL_SW_STALL_APPCPU_C1_M;
    mark('c');

    SYSTEM_CORE_1_CONTROL_1_REG = (uint32_t) _core1_entry;
    mark('d');

    if (!(SYSTEM_CORE_1_CONTROL_0_REG & SYSTEM_CONTROL_CORE_1_CLKGATE_EN)) {
        SYSTEM_CORE_1_CONTROL_0_REG |= SYSTEM_CONTROL_CORE_1_CLKGATE_EN;
        SYSTEM_CORE_1_CONTROL_0_REG &= ~SYSTEM_CONTROL_CORE_1_RUNSTALL;
        SYSTEM_CORE_1_CONTROL_0_REG |= SYSTEM_CONTROL_CORE_1_RESETING;
        SYSTEM_CORE_1_CONTROL_0_REG &= ~SYSTEM_CONTROL_CORE_1_RESETING;
    }

    mark('e');
}

void core1_main(void) {
    mark('1');

    INTERRUPT_CORE1_CPU_INTR_FROM_CPU_0_MAP_REG = IPI_CPU_LINE;

    uint32_t ps = 0;
    __asm__ volatile ("wsr.ps %0; rsync" :: "r"(ps));

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
        SYSTEM_CPU_INTR_FROM_CPU_0_REG = SYSTEM_CPU_INTR_FROM_CPU_0;
    } else {
        SYSTEM_CPU_INTR_FROM_CPU_1_REG = SYSTEM_CPU_INTR_FROM_CPU_1;
    }
}

static void smp_ipi_ack(int from_core) {
    if (from_core == 0) {
        SYSTEM_CPU_INTR_FROM_CPU_0_REG = 0;
    } else {
        SYSTEM_CPU_INTR_FROM_CPU_1_REG = 0;
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
