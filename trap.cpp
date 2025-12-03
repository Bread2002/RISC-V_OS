/* Copyright (c) 2025, Rye Stahle-Smith
December 2nd, 2025 - trap.cpp
Description: Trap handler with syscall support for exit, yield, and semaphore operations. */
#include <stdint.h>
#include "scheduler.h"
#include "shell.h"

volatile uint64_t* const UART0 = (uint64_t*)0x10000000;

extern "C" void trap_handler() {
    uint64_t cause;
    asm volatile("csrr %0, mcause" : "=r"(cause));

    if (cause == 11) { // Environment call from U-mode
        uint64_t syscall_id;
        asm volatile("mv %0, a7" : "=r"(syscall_id));

        // Read syscall arguments from registers
        uint64_t arg0, arg1, arg2, arg3;
        asm volatile("mv %0, a0" : "=r"(arg0));
        asm volatile("mv %0, a1" : "=r"(arg1));
        asm volatile("mv %0, a2" : "=r"(arg2));
        asm volatile("mv %0, a3" : "=r"(arg3));

        int64_t result = -1;

        if (syscall_id == SYSCALL_EXIT) {
            // Exit current process
            if (current > 0) {
                terminate_process(current);
            }
            asm volatile("csrw mepc, %0" :: "r"(kernel_resume_pc));
            return;
        }
        
        else if (syscall_id == SYSCALL_YIELD) {
            // Yield to scheduler (mark as ready, jump back)
            Process* p = scheduler_get_proc_by_pid(current);
            if (p && p->state == PROC_RUNNING) {
                p->state = PROC_READY;
            }
            asm volatile("csrw mepc, %0" :: "r"(kernel_resume_pc));
            return;
        }
        
        else if (syscall_id == SYSCALL_SEM_CREATE) {
            // arg0 = initial value
            result = sem_create((int)arg0);
            // Return value in a0
            asm volatile("mv a0, %0" :: "r"(result));
        }
        
        else if (syscall_id == SYSCALL_SEM_WAIT) {
            // arg0 = semaphore id
            sem_wait((int)arg0);
            result = 0;
            asm volatile("mv a0, %0" :: "r"(result));
        }
        
        else if (syscall_id == SYSCALL_SEM_SIGNAL) {
            // arg0 = semaphore id
            sem_signal((int)arg0);
            result = 0;
            asm volatile("mv a0, %0" :: "r"(result));
        }
        
        else if (syscall_id == SYSCALL_SEM_DESTROY) {
            // arg0 = semaphore id
            result = sem_destroy((int)arg0) ? 0 : -1;
            asm volatile("mv a0, %0" :: "r"(result));
        }
        
        else {
            print_str("Error: Unknown syscall ");
            print_hex(syscall_id);
            print_str("\n");
            asm volatile("csrw mepc, %0" :: "r"(kernel_resume_pc));
            return;
        }

        // Advance mepc past ecall instruction (add 4 bytes)
        uint64_t mepc;
        asm volatile("csrr %0, mepc" : "=r"(mepc));
        mepc += 4;
        asm volatile("csrw mepc, %0" :: "r"(mepc));
        
        return;
    }

    // Unhandled trap
    print_str("Error: Unhandled trap! mcause = ");
    print_hex(cause);
    print_str("\n");

    while (1) asm volatile("wfi");
}
