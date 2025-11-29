/* Copyright (c) 2025, Rye Stahle-Smith
December 2nd, 2025 - user_program.ld
Description: Linker script for RISC-V assembly user programs, defining flat binary layout starting at address 0x0. */
ENTRY(_start)

SECTIONS
{
    . = 0x0;  /* Programs load at offset 0 in their own memory space */

    .text : {
        *(.text*)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

    .bss : {
        *(.bss*)
        *(COMMON)
    }
}