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
    uint32_t a0;       /* Offset  0 */
    uint32_t a1;       /* Offset  4 (SP) */
    uint32_t a2;       /* Offset  8 */
    uint32_t a3;       /* Offset 12 */
    uint32_t a4;       /* Offset 16 */
    uint32_t a5;       /* Offset 20 */
    uint32_t a6;       /* Offset 24 */
    uint32_t a7;       /* Offset 28 */
    uint32_t a8;       /* Offset 32 */
    uint32_t a9;       /* Offset 36 */
    uint32_t a10;      /* Offset 40 */
    uint32_t a11;      /* Offset 44 */
    uint32_t a12;      /* Offset 48 */
    uint32_t a13;      /* Offset 52 */
    uint32_t a14;      /* Offset 56 */
    uint32_t a15;      /* Offset 60 */
    uint32_t epc1;     /* Offset 64 (16*4) */
    uint32_t ps;       /* Offset 68 (17*4) */
    uint32_t sar;      /* Offset 72 (18*4) */
    uint32_t exccause; /* Offset 76 (19*4) */
} __attribute__((packed)) interrupt_context_t;

interrupt_context_t* c_interrupt_handler(interrupt_context_t *ctx);
void init_interrupts(void);

#endif /* INTERRUPTS_H */
