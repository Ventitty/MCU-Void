/******************************************************************************************
  Filename    : IntVectTable.s / vector.S
  Core        : Xtensa LX7
  MCU         : ESP32-S3
  Description : Interrupt vector tables and context switching for ESP32-S3 Bare-Metal
******************************************************************************************/

/* =======================================================================================
   MACROS DE SAUVEGARDE ET RESTAURATION DU CONTEXTE CPU (Compatible Call0 ABI)
   ======================================================================================= */

.macro SaveCpuContext
    /* 1. Allocation de la trame de pile (96 octets alignés sur 16) */
    addi sp, sp, -96

    /* 2. Sauvegarde des registres généraux */
    s32i.n a0,  sp, 0*4
    s32i.n a2,  sp, 2*4
    s32i.n a3,  sp, 3*4
    s32i.n a4,  sp, 4*4
    s32i.n a5,  sp, 5*4
    s32i.n a6,  sp, 6*4
    s32i.n a7,  sp, 7*4
    s32i.n a8,  sp, 8*4
    s32i.n a9,  sp, 9*4
    s32i.n a10, sp, 10*4
    s32i.n a11, sp, 11*4
    s32i.n a12, sp, 12*4
    s32i.n a13, sp, 13*4
    s32i.n a14, sp, 14*4
    s32i.n a15, sp, 15*4

    /* Sauvegarde du SP original (a1) à l'offset 4 */
    addi a2, sp, 96
    s32i.n a2, sp, 1*4

    /* 3. Sauvegarde des registres spéciaux */
    rsr a2, epc1
    s32i a2, sp, 16*4

    rsr a2, ps
    s32i a2, sp, 17*4

    rsr a2, sar
    s32i a2, sp, 18*4

    rsr a2, exccause
    s32i a2, sp, 19*4

    /* 4. Désactivation de PS.EXCM (Bit 4) pour autoriser l'exécution du C */
    rsr a2, ps
    movi a3, ~0x10
    and a2, a2, a3
    wsr a2, ps
    rsync
.endm

.macro RestoreCpuContext
    /* 1. Réactivation de PS.EXCM (Bit 4) avant de quitter l'exception */
    rsr a2, ps
    movi a3, 0x10
    or a2, a2, a3
    wsr a2, ps
    rsync

    /* 2. Restauration des registres spéciaux */
    l32i a2, sp, 16*4
    wsr a2, epc1

    l32i a2, sp, 18*4
    wsr a2, sar
    rsync

    /* 3. Restauration des registres généraux */
    l32i.n a0,  sp, 0*4
    l32i.n a2,  sp, 2*4
    l32i.n a3,  sp, 3*4
    l32i.n a4,  sp, 4*4
    l32i.n a5,  sp, 5*4
    l32i.n a6,  sp, 6*4
    l32i.n a7,  sp, 7*4
    l32i.n a8,  sp, 8*4
    l32i.n a9,  sp, 9*4
    l32i.n a10, sp, 10*4
    l32i.n a11, sp, 11*4
    l32i.n a12, sp, 12*4
    l32i.n a13, sp, 13*4
    l32i.n a14, sp, 14*4
    l32i.n a15, sp, 15*4

    /* 4. Restauration du pointeur de pile */
    addi sp, sp, 96
.endm

.macro call_isr isr_name
    SaveCpuContext
    mov a2, sp                  /* Passe le pointeur de contexte au handler C (ABI Call0) */
    call0 \isr_name
    RestoreCpuContext
.endm

/* =======================================================================================
   TABLE DES VECTEURS D'INTERRUPTION (IRAM, Alignée sur 1024 octets)
   ======================================================================================= */

.section .iram0.vectors, "ax"
.global _vector_base
.type _vector_base, @function
.balign 1024

_vector_base:

    .org 0x000
    WindowVectors:
        rfwo

    .org 0x180
    Level2InterruptVector:
        j Level2InterruptVectorHandler

    .org 0x1C0
    Level3InterruptVector:
        j Level3InterruptVectorHandler

    .org 0x200
    Level4InterruptVector:
        j Level4InterruptVectorHandler

    .org 0x240
    Level5InterruptVector:
        j Level5InterruptVectorHandler

    .org 0x280
    DebugExceptionVector:
        1: j 1b

    .org 0x2C0
    NMIExceptionVector:
        1: j 1b

    .org 0x300
    Level1KernelInterruptVector:
        j Level1KernelInterruptVectorHandler

    .org 0x340
    Level1UserInterruptVector:
        j Level1UserInterruptVectorHandler

    .org 0x3C0
    DoubleExceptionVector:
        1: j 1b

.size _vector_base, .-_vector_base

/* =======================================================================================
   GESTIONNAIRES DE VECTEURS (HANDLERS)
   ======================================================================================= */

.section .iram0.text, "ax"
.global _vector_handlers
.type _vector_handlers, @function
.align 4

.extern c_interrupt_handler

_vector_handlers:

    Level2InterruptVectorHandler:
        call_isr c_interrupt_handler
        rfi 2

    Level3InterruptVectorHandler:
        call_isr c_interrupt_handler
        rfi 3

    Level4InterruptVectorHandler:
        call_isr c_interrupt_handler
        rfi 4

    Level5InterruptVectorHandler:
        call_isr c_interrupt_handler
        rfi 5

    Level1KernelInterruptVectorHandler:
        call_isr c_interrupt_handler
        rfe

    Level1UserInterruptVectorHandler:
        call_isr c_interrupt_handler
        rfe

.size _vector_handlers, .-_vector_handlers

/* =======================================================================================
   FONCTIONS UTILITAIRES LOW-LEVEL
   ======================================================================================= */

.section .iram0.text, "ax"
.align 4
.global enable_irq
.type enable_irq, @function
enable_irq:
    wsr a2, INTENABLE
    rsync
    ret
.size enable_irq, .-enable_irq

.section .iram0.text, "ax"
.align 4
.global disable_irq
.type disable_irq, @function
disable_irq:
    wsr a2, INTCLEAR
    rsync
    ret
.size disable_irq, .-disable_irq

.section .iram0.text, "ax"
.align 4
.global set_cpu_private_timer
.type set_cpu_private_timer, @function
set_cpu_private_timer:
    beqi a2, 0, .L_timer0
    beqi a2, 1, .L_timer1
    beqi a2, 2, .L_timer2
    ret
.L_timer0:
    rsr  a11, ccount
    add  a11, a11, a3
    wsr  a11, ccompare0
    rsync
    ret
.L_timer1:
    rsr  a11, ccount
    add  a11, a11, a3
    wsr  a11, ccompare1
    rsync
    ret
.L_timer2:
    rsr  a11, ccount
    add  a11, a11, a3
    wsr  a11, ccompare2
    rsync
    ret
.size set_cpu_private_timer, .-set_cpu_private_timer
