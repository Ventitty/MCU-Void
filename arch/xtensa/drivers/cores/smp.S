# arch/xtensa/boot/core1_entry.S
#
# *** NON VÉRIFIÉ *** -- comme tout l'assembleur de ce projet, je n'ai
# aucun moyen de l'assembler ni de le tester ici.
#
# Point d'entrée du cœur 1 (APP_CPU), configuré via
# ets_set_appcpu_boot_addr() dans kernel/smp/smp.c. Contrairement à
# _ResetVector (boot.S), CE code ne doit PAS re-zérober .bss -- le cœur
# 0 tourne déjà et .bss peut contenir un état légitime à ce moment-là.
# Pose sa PROPRE pile (distincte de celle du cœur 0 -- sinon collision
# immédiate et corruption) et son PROPRE VECBASE (chaque cœur a son
# propre jeu de registres, VECBASE y compris), puis saute en C.

    .section .iram0.text, "ax"
    .global _core1_entry
    .align 4
_core1_entry:
    movi    a1, _core1_stack_top
    movi    a0, 0

    movi    a2, _vector_base
    wsr.vecbase a2
    rsync

    call0   core1_main
1:
    j       1b

    .section .bss, "aw"
    .align 16
_core1_stack_bottom:
    .space 8192  # 8 Ko -- ajuster si core1_main()/les tâches du cœur 1
                 # ont besoin de plus de pile
    .global _core1_stack_top
_core1_stack_top:
