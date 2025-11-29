/* Copyright (c) 2025, Rye Stahle-Smith
December 2nd, 2025 - scheduler.h
Description: Scheduler API definitions including process structures, states, trapframe references, and public scheduling functions. */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#pragma once
#include <stdint.h>
#include <stdbool.h>

#define SYSCALL_EXIT 1
#define SYSCALL_YIELD 2
#define MAX_PROCS 8
#define DEFAULT_STACK_SIZE  (16 * 1024)  // 16 KB

enum ProcessState {
    PROC_FREE = 0,
    PROC_READY,
    PROC_RUNNING,
    PROC_SLEEP,
    PROC_ZOMBIE,
};

struct Process {
    int       pid;
    const char* name;
    void      (*entry)();      // entry function for the process
    uint8_t*    stack;         // pointer to bottom of stack (allocated)
    uint8_t*    stack_top;     // pointer to top-of-stack (stack + size)
    uint32_t    stack_size;
    ProcessState state;
};

extern Process proc_table[MAX_PROCS];
extern uintptr_t kernel_saved_sp;
extern uintptr_t kernel_resume_pc;

// Current running PID (index in table or -1)
extern int current;

// Scheduler public API
bool scheduler_init();
void scheduler_main();

// create a process that will run 'entry'; returns pid or -1 on error
int create_process(void (*entry)(), const char* name, uint32_t stack_size);

// Create process from binary code (for user programs)
int create_process_from_binary(const uint8_t* binary, uint32_t binary_size, 
                                const char* name, uint32_t stack_size);

int scheduler_run_pid(int pid);     // run process immediately (blocking)
Process* scheduler_get_proc_by_pid(int pid);

// Simple cooperative yield (placeholder for future context switch)
void schedule_yield();

// Query active process count (helper)
int scheduler_proc_count();

static void run_process(Process* p);
void terminate_process(int pid);

extern "C" void scheduler_process_return();

Process* scheduler_get_process_table();
int scheduler_get_max_procs();

#endif
