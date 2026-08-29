.section .ResetVector.text, "ax"
    .global _ResetVector
    .align 4
    .literal_position
_ResetVector:
    /* 1. Effacer le bit EXCM et baisser INTLEVEL (PS = 0) */
    movi    a2, 0
    wsr.ps  a2
    rsync

    /* 2. Charger VECBASE immediatement sur notre table */
    movi    a2, _vector_base
    wsr.vecbase a2
    rsync

    /* 3. Initialiser le pointeur de pile dans la DRAM valide */
    movi    a1, _stack_top
    movi    a0, 0
    j       _start_c

.section .iram0.text, "ax"
    .global _start_c
    .align 4
    .literal_position
_start_c:
    /* 4. Effacer la section .bss */
    movi    a2, _bss_start
    movi    a3, _bss_end
1:
    bge     a2, a3, 2f
    s32i    a0, a2, 0
    addi    a2, a2, 4
    j       1b

2:
    /* 5. Saut vers le noyau C */
    call0   kernel

3:
    j       3b
