MEMORY
{
    iram (RWX) : ORIGIN = 0x40378000, LENGTH = 0x40000  /* 256 KB IRAM */
    dram (RW)  : ORIGIN = 0x3FC80000, LENGTH = 0x50000  /* 320 KB DRAM */
}

STACK_SIZE = 0x4000;

ENTRY(_ResetVector)

PROVIDE ( ets_set_appcpu_boot_addr = 0x40000720 );

SECTIONS
{
    .ResetVector.text : {
        . = ALIGN(4);
        *(.ResetVector.literal)
        *(.ResetVector.text)
    } > iram

    .iram0.vectors : ALIGN(1024) {
        _vector_base = .;
        KEEP(*(.iram0.vectors))
    } > iram

    .text : ALIGN(4) {
        *(.iram0.literal)
        *(.iram0.text .iram0.text.*)
        *(.literal .literal.*)
        *(.text .text.*)
    } > iram

    .dram0.data : ALIGN(16) {
        *(.data .data.*)
        *(.rodata .rodata.*)
    } > dram

    .bss (NOLOAD) : ALIGN(16) {
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        _bss_end = .;
        . = ALIGN(16);
    } > dram

    _heap_start = .;
}

_stack_top    = ORIGIN(dram) + LENGTH(dram);
_stack_bottom = _stack_top - STACK_SIZE;
_heap_end     = _stack_bottom;

ASSERT(_heap_end > _heap_start, "Not enough RAM !")
