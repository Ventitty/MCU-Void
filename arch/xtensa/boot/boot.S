.section .ResetVector.text, "ax"
    .global _ResetVector
    .align 4
    .literal_position
_ResetVector:
    movi    a1, _stack_top
    movi    a0, 0
    j       _start_c

    .section .iram0.text, "ax"
    .global _start_c
    .align 4
    .literal_position
_start_c:
    movi    a2, _bss_start
    movi    a3, _bss_end
1:
    bge     a2, a3, 2f
    s32i    a0, a2, 0
    addi    a2, a2, 4
    j       1b
2:
    call0   kernel
3:
    j       3b

    .section .iram0.text, "ax"
    .global _placeholder_exception_handler
    .align 4
_placeholder_exception_handler:
4:
    j       4b
