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
    .space 8192

    .global _core1_stack_top
_core1_stack_top:
