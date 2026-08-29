#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "kernel/types.h"

#define UART0_BASE       0x60000000
#define UART0_INT_ST     (*(volatile uint32_t *)(UART0_BASE + 0x08))
#define UART0_INT_ENA    (*(volatile uint32_t *)(UART0_BASE + 0x0C))
#define UART0_INT_CLR    (*(volatile uint32_t *)(UART0_BASE + 0x10))
#define UART0_STATUS     (*(volatile uint32_t *)(UART0_BASE + 0x1C))
#define UART0_CONF1      (*(volatile uint32_t *)(UART0_BASE + 0x24))

/* --- Matrice d'interruptions ESP32-S3 --- */
#define INTERRUPT_MATRIX_BASE 0x600C0000
#define CORE0_UART0_MAP_REG   (*(volatile uint32_t *)(INTERRUPT_MATRIX_BASE + (34 * 4)))

typedef struct {
    uint32_t a0, a1, a2, a3, a4, a5, a6, a7;
    uint32_t a8, a9, a10, a11, a12, a13, a14, a15;
    uint32_t epc1;
    uint32_t ps;
    uint32_t sar;
    uint32_t exccause;
} interrupt_context_t;

void c_interrupt_handler(interrupt_context_t *ctx);
void init_interrupts(void);

#endif /* INTERRUPTS_H */
