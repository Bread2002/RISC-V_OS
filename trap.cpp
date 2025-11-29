/* Copyright (c) 2025, Rye Stahle-Smith
December 2nd, 2025 - trap.cpp
Description: High-level trap handler responsible for decoding mcause, handling syscalls (exit, yield), adjusting mepc, and interacting with the scheduler. */
#include <stdint.h>
#include "scheduler.h"
#include "shell.h"

volatile uint64_t* const UART0 = (uint64_t*)0x10000000;

extern "C" void trap_handler() {
    uint64_t cause;
    asm volatile("csrr %0, mcause" : "=r"(cause));

    if (cause == 11) { // Environment call from U-mode
        uint64_t id;
        asm volatile("mv %0, a7" : "=r"(id));

        if (id == SYSCALL_EXIT) {
            if (current > 0) {
                terminate_process(current);
            }
                
            asm volatile("csrw mepc, %0" :: "r"(kernel_resume_pc));

            return;
        }
    }

    // Unhandled trap
    print_str("Error: Unhandled trap! mcause = ");
    print_hex(cause);
    print_str("\n");

    while (1) asm volatile("wfi");
}
