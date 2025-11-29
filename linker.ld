/* Copyright (c) 2025, Rye Stahle-Smith
December 2nd, 2025 - linker.ld
Description: Kernel linker script defining memory layout, virtual addresses, sections, and entry point for the RISC-V OS. */
ENTRY(_start)

MEMORY
{
    RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 50M
}

SECTIONS
{
    . = ORIGIN(RAM);

    .text : {
        *(.text*)
        *(.rodata*)
    } > RAM

    .data : {
        *(.data*)
    } > RAM

    .bss : {
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        __bss_end__ = .;
    } > RAM

    /* ----------------------------------------------- */
    /* Kernel heap region: grows upward                */
    /* ----------------------------------------------- */
    . = ALIGN(16);
    _kernel_heap_start = .;

    /* Reserve 10 MB for kernel heap (tweak as needed) */
    . = . + 10M;
    _kernel_heap_end = .;

    /* ----------------------------------------------- */
    /* Stack at top of RAM (grows downward)            */
    /* ----------------------------------------------- */
    _stack_start = ORIGIN(RAM) + LENGTH(RAM);
}
