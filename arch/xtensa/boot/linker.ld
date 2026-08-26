MEMORY
{
    iram (RWX) : ORIGIN = 0x40080000, LENGTH = 0x20000
    dram (RW)  : ORIGIN = 0x3FFB0000, LENGTH = 0x2C000
}

STACK_SIZE = 0x4000;

ENTRY(_ResetVector)

SECTIONS
{
    .ResetVector.text : {
        *(.ResetVector.literal)
        *(.ResetVector.text)
    } > iram

    .iram0.text : {
        *(.iram0.literal)
        *(.literal .literal.*)

        *(.iram0.text)
        *(.text .text.*)
    } > iram

    .dram0.data : {
        *(.data .data.*)
        *(.rodata .rodata.*)
    } > dram

    .bss (NOLOAD) : {
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        _bss_end = .;
    } > dram

    _heap_start = .;
}

_stack_top    = ORIGIN(dram) + LENGTH(dram);
_stack_bottom = _stack_top - STACK_SIZE;
_heap_end     = _stack_bottom;

ASSERT(_heap_end > _heap_start, "Not enough RAM !")
