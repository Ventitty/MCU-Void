/******************************************************************************************
  Filename    : boot.S
  Core        : Xtensa LX6
  MCU         : ESP32
  Description : Vecteur de Reset et initialisation BSS/Stack pour le Core 0 (PRO CPU)
******************************************************************************************/

.section .ResetVector.text, "ax"
    .global _ResetVector
    .align 4
    .literal_position
_ResetVector:
    /* 1. Configuration du registre PS pour l'ABI Call0
       0x00000020 = INTLEVEL=0, EXCM=0, WOE=0 (Mode Call0 non-fenêtré) */
    movi    a2, 0x00000020
    wsr     a2, ps
    rsync

    /* 2. Charger VECBASE immédiatement sur notre table d'interruptions */
    movi    a2, _vector_base
    wsr     a2, vecbase
    rsync

    /* 3. Initialiser le pointeur de pile dans la DRAM (définie dans linker.ld) */
    movi    a1, _stack_top
    movi    a0, 0
    j       _start_c

.section .iram0.text, "ax"
    .global _start_c
    .align 4
    .literal_position
_start_c:
    /* 4. Effacer la section .bss (mise à zéro par mots de 32 bits) */
    movi    a2, _bss_start
    movi    a3, _bss_end
1:
    bge     a2, a3, 2f
    s32i    a0, a2, 0
    addi    a2, a2, 4
    j       1b

2:
    /* 5. Saut vers le point d'entrée du noyau C */
    call0   kernel

3:
    /* Boucle infinie si le noyau se termine */
    j       3b
