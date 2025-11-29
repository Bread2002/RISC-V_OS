/* Copyright (c) 2025, Rye Stahle-Smith
December 2nd, 2025 - scheduler.cpp
Description: Cooperative RISC-V process scheduler implementing PID management, process creation, state transitions, yielding, and round-robin task selection. */
#include "scheduler.h"
#include "shell.h"
#include "memory.h"

static char proc_name_buf[MAX_PROCS][16];

// Process table
Process proc_table[MAX_PROCS];
static int next_pid = 1;
int current = -1;

uintptr_t kernel_saved_sp;
uintptr_t kernel_resume_pc;

// ---------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------
static Process* pid_to_proc(int pid) {
    if (pid <= 0) return nullptr;
    for (int i = 0; i < MAX_PROCS; ++i) {
        if (proc_table[i].pid == pid) return &proc_table[i];
    }
    return nullptr;
}

static Process* find_free_slot() {
    for (int i = 0; i < MAX_PROCS; ++i) {
        if (proc_table[i].state == PROC_FREE) return &proc_table[i];
    }
    return nullptr;
}

static Process* find_next_ready(int start_idx) {
    // 1. First search for RUNNING processes (round-robin)
    for (int offset = 0; offset < MAX_PROCS; ++offset) {
        int i = (start_idx + offset) % MAX_PROCS;
        if (proc_table[i].state == PROC_RUNNING)
            return &proc_table[i];
    }

    // 2. If none RUNNING, search for READY processes
    for (int offset = 0; offset < MAX_PROCS; ++offset) {
        int i = (start_idx + offset) % MAX_PROCS;
        if (proc_table[i].state == PROC_READY)
            return &proc_table[i];
    }

    return nullptr;
}

void terminate_process(int pid) {
    Process* p = pid_to_proc(pid);
    if (!p) return;
    p->state = PROC_ZOMBIE;
}

// Simple stack switch runner:
// Save current kernel sp, switch to proc->stack_top, call proc->entry,
// on return restore kernel sp.
static void run_process(Process* p) {
    if (!p || !p->entry) return;

    print_str("(scheduler) Starting process...\n");

    uintptr_t saved_sp;
    asm volatile("mv %0, sp" : "=r"(saved_sp));

    kernel_saved_sp = saved_sp;
    kernel_resume_pc = (uintptr_t)&scheduler_process_return;

    asm volatile("mv sp, %0" :: "r"(p->stack_top));
    current = p->pid;

    p->state = PROC_RUNNING;
    p->entry();          // either returns, or triggers ecall + trap

    scheduler_process_return();
}

extern "C" void scheduler_process_return() {
    // Restore kernel stack
    asm volatile("mv sp, %0" :: "r"(kernel_saved_sp));

    // Free resources for previous process
    Process* p = scheduler_get_proc_by_pid(current);
    if (p->state == PROC_ZOMBIE) {
        p->state = PROC_FREE;
        p->pid = 0;
        p->entry = nullptr;
        p->name = nullptr;
        p->stack = nullptr;
        p->stack_top = nullptr;
        p->stack_size = 0;
    }

    // Return to shell
    p = find_next_ready(0);
    if (p) {
        current = p->pid;

        if (p->name && strcmp(p->name, "shell") == 0) {
            p->state = PROC_RUNNING;  // Keep it running
            // Don't mark as ZOMBIE
        } else {
            // If the process yielded, it should remain READY; otherwise mark as ZOMBIE
            if (p->state == PROC_RUNNING)
                p->state = PROC_READY;
            else if (p->state != PROC_ZOMBIE)
                p->state = PROC_ZOMBIE;
        }
    }
}

// ---------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------
bool scheduler_init() {
    // initialize process table
    for (int i = 0; i < MAX_PROCS; ++i) {
        proc_table[i].pid = 0;
        proc_table[i].name = nullptr;
        proc_table[i].entry = nullptr;
        proc_table[i].stack = nullptr;
        proc_table[i].stack_top = nullptr;
        proc_table[i].stack_size = 0;
        proc_table[i].state = PROC_FREE;
    }
    next_pid = 1;
    current = -1;
    return true;
}

int create_process(void (*entry)(), const char* name, uint32_t stack_size) {
    Process* slot = find_free_slot();
    if (!slot) return -1;

    int slot_idx = slot - proc_table;

    // allocate stack...
    void* stk = kmalloc(stack_size);
    if (!stk) return -1;

    slot->pid = next_pid++;
    slot->entry = entry;
    slot->stack = (uint8_t*)stk;
    slot->stack_size = stack_size;
    slot->stack_top = slot->stack + slot->stack_size;
    slot->stack_top = (uint8_t*)((uintptr_t)slot->stack_top & ~0xFULL);

    // copy name into persistent global buffer
    const char* src = name ? name : "proc";
    int j = 0;
    while (src[j] && j < (int)sizeof(proc_name_buf[0]) - 1) {
        proc_name_buf[slot_idx][j] = src[j];
        j++;
    }
    proc_name_buf[slot_idx][j] = '\0';
    slot->name = proc_name_buf[slot_idx];

    slot->state = PROC_READY;

    char pid_str[16];
    itoa(slot->pid, pid_str);
    print_str("(scheduler) Process created for '");
    print_str(name);
    print_str("' [PID ");
    print_str(pid_str);
    print_str("].\n");

    return slot->pid;
}

// Add after existing functions, before scheduler_main
int create_process_from_binary(const uint8_t* binary, uint32_t binary_size, 
                                const char* name, uint32_t stack_size) {
    Process* slot = find_free_slot();
    if (!slot) return -1;

    int slot_idx = slot - proc_table;

    // Allocate memory for code + stack
    uint32_t code_size = (binary_size + 15) & ~15ULL; // align to 16 bytes
    void* code_mem = kmalloc(code_size);
    if (!code_mem) {
        print_str("(scheduler) Failed to allocate code memory\n");
        return -1;
    }

    void* stack_mem = kmalloc(stack_size);
    if (!stack_mem) {
        print_str("(scheduler) Failed to allocate stack memory\n");
        return -1;
    }

    // Copy binary into allocated memory
    memcpy(code_mem, binary, binary_size);

    slot->pid = next_pid++;
    slot->entry = (void(*)())code_mem;  // Entry point is start of code
    slot->stack = (uint8_t*)stack_mem;
    slot->stack_size = stack_size;
    slot->stack_top = slot->stack + slot->stack_size;
    slot->stack_top = (uint8_t*)((uintptr_t)slot->stack_top & ~0xFULL);

    // Copy name into persistent buffer
    const char* src = name ? name : "userproc";
    int j = 0;
    while (src[j] && j < (int)sizeof(proc_name_buf[0]) - 1) {
        proc_name_buf[slot_idx][j] = src[j];
        j++;
    }
    proc_name_buf[slot_idx][j] = '\0';
    slot->name = proc_name_buf[slot_idx];

    slot->state = PROC_READY;

    char pid_str[16];
    itoa(slot->pid, pid_str);
    print_str("(scheduler) Process created for '");
    print_str(name);
    print_str("' [PID ");
    print_str(pid_str);
    print_str("].\n");

    return slot->pid;
}

void schedule_yield() {
    // Cooperative yield stub:
    // For now, do nothing. In the future: save registers and switch to scheduler.
    // Processes should simply return or call a syscall to yield.
    // (We keep this API so later user processes can call schedule_yield()).
    asm volatile("nop");
}

int scheduler_proc_count() {
    int cnt = 0;
    for (int i = 0; i < MAX_PROCS; ++i) {
        if (proc_table[i].state != PROC_FREE) ++cnt;
    }
    return cnt;
}

Process* scheduler_get_process_table() {
    return proc_table; // proc_table can remain static
}

int scheduler_get_max_procs() {
    return MAX_PROCS;
}

// find process by pid (public)
Process* scheduler_get_proc_by_pid(int pid) {
    return pid_to_proc(pid);
}

// run process by pid (blocking)
int scheduler_run_pid(int pid) {
    Process *p = pid_to_proc(pid);
    if (!p) return -1;

    // We need to call the internal run_process; it's currently static.
    // If you keep run_process static, move this wrapper into the same .cpp file
    // so it can call run_process(p). If you move run_process to non-static, keep care.
    run_process(p);
    print_str("(scheduler) Process finished or exited...\n");
    return 0;
}

// ---------------------------------------------------------------------
// scheduler_main - very small loop for initial testing
// - If no processes exist, create the shell process
// - Run ready processes one by one (cooperative)
// ---------------------------------------------------------------------
void scheduler_main() {
    print_str("(scheduler) Entering main loop...\n");

    // ensure scheduler initialized
    scheduler_init();

    // If there are no processes, create the shell as a process
    bool has_any = false;
    for (int i = 0; i < MAX_PROCS; ++i) {
        if (proc_table[i].state != PROC_FREE) { has_any = true; break; }
    }
    if (!has_any) {
        int pid = create_process((void(*)())shell_main, "shell", DEFAULT_STACK_SIZE);
        if (pid < 0) {
            print_str("(scheduler) Failed to create shell process...\n");
        }
    }

    // Very simple cooperative round robin:
    while (1) {
        // find the next ready process (if current == -1, start at -1 so first = 0)
        int start_idx = 0;
        if (current > 0) {
            // find index of current pid
            for (int i = 0; i < MAX_PROCS; ++i) {
                if (proc_table[i].pid == current) { start_idx = i; break; }
            }
        } else {
            start_idx = 0;
        }

        Process* next = find_next_ready(start_idx);
        if (next) {
            // Run it until it returns (cooperative)
            run_process(next);
        } else {
            // No ready processes: idle - spin or wait for interrupts
            asm volatile("wfi");
        }
    }
}
