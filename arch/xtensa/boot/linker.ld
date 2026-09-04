MEMORY
{
    iram (RWX) : ORIGIN = 0x40378000, LENGTH = 0x40000  /* 256 KB IRAM */
    dram (RW)  : ORIGIN = 0x3FC80000, LENGTH = 0x50000  /* 320 KB DRAM */
}

STACK_SIZE = 0x4000;

ENTRY(_ResetVector)

/* Fonction de la ROM Espressif -- ce n'est pas du code compilé dans ce
 * projet, juste un symbole pointant vers une adresse fixe en ROM.
 * Sans cette ligne, le linker échoue sur "undefined reference to
 * ets_set_appcpu_boot_addr" (appelée par smp_start_core1).
 * Adresse vérifiée dans :
 *   esp-idf/components/esp_rom/esp32s3/ld/esp32s3.rom.ld:51 */
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
