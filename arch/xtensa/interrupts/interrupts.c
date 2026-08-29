#include "arch/xtensa/interrupts/interrupts.h"

extern uint8_t _vector_base[];

void init_interrupts(void) {
    __asm__ volatile (
        "wsr %0, vecbase\n"
        "isync\n"
        :
        : "r" (_vector_base)
        : "memory"
    );
}

