/* Copyright (c) 2025, Rye Stahle-Smith
December 2nd, 2025 - boot.S
Description: Bare-metal RISC-V bootloader that initializes machine mode, sets up trap vectors, prepares stack state, prints boot messages, and jumps into the kernel. */
    .section .text
    .globl _start
    .align 4

_start:
    # Set stack pointer to top of RAM
    la sp, _stack_start

    li   t1, 0
    csrw medeleg, t1
    csrw mideleg, t1

    # Initialize trap vectors
    la   t0, trap_vector
    csrw mtvec, t0
    csrw stvec, t0

    la a0, boot_msg
    call print_str

    # Jump to kernel main
    call kernel_main

hang:
    wfi
    j hang

    .section .rodata
boot_msg:
    .ascii "(bootloader) Booting...\n\0"

/* Trap trampoline (M-mode entry) - forward to trap_vector label */
    .section .text
    .globl trap
trap:
    j trap_vector

/* Stack section */
    .section .bss
    .align 16
stack:
    .space 16384   # 16 KiB stack
_stack_start:
