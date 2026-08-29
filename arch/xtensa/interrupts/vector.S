.section .iram0.text, "ax"
.align 1024       /* VECBASE DOIT être aligné sur 1024 octets (0x400) */
.global _vector_base
_vector_base:
    .space 0x340  /* Décalage exact pour le UserExceptionVector en Niveau 1 */

.global _UserExceptionVector
_UserExceptionVector:
    /* 1. Allouer la trame de pile (96 octets) */
    /* 16 registres généraux + 4 registres spéciaux = 20 * 4 = 80 octets. On arrondit à 96. */
    addi sp, sp, -96

    /* 2. Sauvegarder a0 et a2 à a15 */
    s32i a0, sp, 0
    s32i a2, sp, 8
    s32i a3, sp, 12
    s32i a4, sp, 16
    s32i a5, sp, 20
    s32i a6, sp, 24
    s32i a7, sp, 28
    s32i a8, sp, 32
    s32i a9, sp, 36
    s32i a10, sp, 40
    s32i a11, sp, 44
    s32i a12, sp, 48
    s32i a13, sp, 52
    s32i a14, sp, 56
    s32i a15, sp, 60

    /* Calculer et sauvegarder le a1 original (le SP d'avant l'interruption) */
    addi a2, sp, 96
    s32i a2, sp, 4

    /* 3. Sauvegarder les registres spéciaux */
    /* EPC1 : Exception Program Counter (Adresse où le CPU doit retourner) */
    rsr.epc1 a2
    s32i a2, sp, 64

    /* PS : Processor Status (Niveau de privilège et statut des interruptions) */
    rsr.ps a2
    s32i a2, sp, 68

    /* SAR : Shift Amount Register (Vital ! Le code C l'utilise en arrière-plan) */
    rsr.sar a2
    s32i a2, sp, 72

    /* EXCCAUSE : Cause de l'exception (utile pour le diagnostic) */
    rsr.exccause a2
    s32i a2, sp, 76

    /* 4. Appel du gestionnaire en C */
    /* On place 'sp' dans 'a2' pour le passer comme premier argument à la fonction C */
    mov a2, sp
    call0 c_interrupt_handler

    /* 5. Restaurer les registres spéciaux */
    l32i a2, sp, 64
    wsr.epc1 a2

    l32i a2, sp, 68
    wsr.ps a2

    l32i a2, sp, 72
    wsr.sar a2

    /* Synchroniser le pipeline CPU après avoir modifié des registres de contrôle (PS) */
    rsync

    /* 6. Restaurer les registres généraux */
    l32i a0, sp, 0
    l32i a2, sp, 8
    l32i a3, sp, 12
    l32i a4, sp, 16
    l32i a5, sp, 20
    l32i a6, sp, 24
    l32i a7, sp, 28
    l32i a8, sp, 32
    l32i a9, sp, 36
    l32i a10, sp, 40
    l32i a11, sp, 44
    l32i a12, sp, 48
    l32i a13, sp, 52
    l32i a14, sp, 56
    l32i a15, sp, 60

    /* 7. Libérer la pile et quitter l'interruption */
    /* a1 (sp) est restauré ici de facto */
    addi sp, sp, 96

    /* Return From Exception (restaure automatiquement le PC depuis EPC1) */
    rfe
