#include "arch/xtensa/interrupts/interrupts.h"

extern uint8_t _vector_base[];

void xtensa_set_vecbase(void *base) {
    __asm__ __volatile__("wsr.vecbase %0" :: "r"(base));
}

void xtensa_enable_interrupts(uint32_t mask) {
    __asm__ __volatile__("wsr.intenable %0" :: "r"(mask));
}

void xtensa_clear_interrupts(uint32_t mask) {
    __asm__ __volatile__("wsr.intclear %0" :: "r"(mask));
}

void xtensa_set_ps(uint32_t ps) {
    __asm__ __volatile__("wsr.ps %0 ; rsync" :: "r"(ps));
}

void esp32s3_route_interrupt(uint32_t source_id, uint32_t cpu_inum) {
    if (cpu_inum > 31) return;
    volatile uint32_t *matrix_reg = (volatile uint32_t *)(INTERRUPT_MATRIX_BASE + (cpu_inum * 4));
    *matrix_reg = source_id;
}

void init_interrupts(void) {
    xtensa_set_vecbase(_vector_base);

    esp32s3_route_interrupt(34, 1);

    UART0_CONF1 = (UART0_CONF1 & ~0x3FF) | 1;
    UART0_INT_ENA |= (1 << 0);

    xtensa_enable_interrupts(1 << 1);

    xtensa_set_ps(0);
}
