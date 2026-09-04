/******************************************************************************************
  Filename    : smp_core1.S
  Core        : Xtensa LX6
  MCU         : ESP32
  Description : Point d'entrée et pile du cœur 1 (APP CPU) pour ESP32
******************************************************************************************/

    .section .iram0.text, "ax"
    .global _core1_entry
    .align 4
_core1_entry:
    /* 1. Configuration de la pile du cœur 1 et raz du Return Address */
    movi    a1, _core1_stack_top
    movi    a0, 0

    /* 2. Réinitialisation du registre PS pour l'ABI Call0
       0x00000020 = INTLEVEL=0, EXCM=0, Mode non-windowed (Call0) */
    movi    a2, 0x00000020
    wsr     a2, ps
    rsync

    /* 3. Configuration du registre VECBASE pour les interruptions du cœur 1 */
    movi    a2, _vector_base
    wsr.vecbase a2
    rsync

    /* 4. Appel de la fonction principale du cœur 1 en C */
    call0   core1_main

1:
    j       1b

/* =======================================================================================
   PILE DÉDIÉE AU CŒUR 1 (8 KB, alignée sur 16 octets dans la DRAM)
   ======================================================================================= */
    .section .bss, "aw"
    .align 16
_core1_stack_bottom:
    .space 8192

    .global _core1_stack_top
_core1_stack_top:
