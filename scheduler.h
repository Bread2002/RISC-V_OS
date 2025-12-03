/* Copyright (c) 2025, Rye Stahle-Smith
December 2nd, 2025 - scheduler.h
Description: Scheduler API definitions including process structures, states, trapframe references, and public scheduling functions. */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#pragma once
#include <stdint.h>

#define MAX_PROCS 16
#define MAX_SEMS 32
#define DEFAULT_STACK_SIZE 4096

#define SYSCALL_EXIT 93
#define SYSCALL_YIELD 124
#define SYSCALL_SEM_CREATE 150
#define SYSCALL_SEM_WAIT 151
#define SYSCALL_SEM_SIGNAL 152
#define SYSCALL_SEM_DESTROY 153

// Process states
enum ProcState {
    PROC_FREE,
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED_SEM,  // blocked on semaphore
    PROC_SLEEP,
    PROC_ZOMBIE
};

// Process control block
struct Process {
    int pid;
    const char* name;
    void (*entry)();
    uint8_t* stack;
    uint8_t* stack_top;  // pointer to top of stack
    uint32_t stack_size;
    ProcState state;
    int blocked_sem_id;  // which semaphore it's blocked on
    Process* next_blocked;  // linked list of blocked processes
};

// Semaphore structure
struct Semaphore {
    int id;
    int value;
    int owner_pid;  // PID that created it
    Process* blocked_list;  // head of linked list of blocked processes
    bool in_use;
};

// Global process table
extern Process proc_table[MAX_PROCS];
extern int current;
extern uintptr_t kernel_saved_sp;
extern uintptr_t kernel_resume_pc;

// Process management
bool scheduler_init();
int create_process(void (*entry)(), const char* name, uint32_t stack_size);
int create_process_from_binary(const uint8_t* binary, uint32_t binary_size,
                                const char* name, uint32_t stack_size);
void schedule_yield();
int scheduler_proc_count();
Process* scheduler_get_process_table();
int scheduler_get_max_procs();
Process* scheduler_get_proc_by_pid(int pid);
int scheduler_run_pid(int pid);
void terminate_process(int pid);
void scheduler_main();

// Semaphore management
int sem_create(int initial_value);
void sem_wait(int sem_id);
void sem_signal(int sem_id);
bool sem_destroy(int sem_id);
Semaphore* sem_get(int sem_id);

// Context switching
extern "C" void scheduler_process_return();

#endif
