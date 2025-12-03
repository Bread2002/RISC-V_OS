/* Copyright (c) 2025, Rye Stahle-Smith
December 2nd, 2025 - scheduler.cpp
Description: Process scheduler with semaphore synchronization, blocking/waking, and concurrent round-robin execution. */
#include "scheduler.h"
#include "shell.h"
#include "memory.h"

static char proc_name_buf[MAX_PROCS][16];

// Process table
Process proc_table[MAX_PROCS];
static int next_pid = 1;
int current = -1;

// Semaphore table
static Semaphore sem_table[MAX_SEMS];
static int next_sem_id = 1;

// Kernel state
uintptr_t kernel_saved_sp;
uintptr_t kernel_resume_pc;

// Memory barrier for visibility
static void memory_barrier() {
    asm volatile("fence rw,rw" ::: "memory");
}

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

static Semaphore* find_free_sem_slot() {
    for (int i = 0; i < MAX_SEMS; ++i) {
        if (!sem_table[i].in_use) return &sem_table[i];
    }
    return nullptr;
}

static Process* find_next_ready(int start_idx) {
    // Search for next READY or RUNNING process, skipping blocked processes
    for (int offset = 0; offset < MAX_PROCS; ++offset) {
        int i = (start_idx + offset) % MAX_PROCS;
        if (proc_table[i].state == PROC_READY || proc_table[i].state == PROC_RUNNING)
            return &proc_table[i];
    }
    return nullptr;
}

void terminate_process(int pid) {
    Process* p = pid_to_proc(pid);
    if (p) {
        p->state = PROC_ZOMBIE;
    }
}

// Simple stack switch runner
static void run_process(Process* p) {
    if (!p || !p->entry) return;

    print_str("(scheduler) Starting process '");
    print_str(p->name);
    print_str("' [PID ");
    char pid_str[16];
    itoa(p->pid, pid_str, 10);
    print_str(pid_str);
    print_str("]...\n");

    uintptr_t saved_sp;
    asm volatile("mv %0, sp" : "=r"(saved_sp));

    kernel_saved_sp = saved_sp;
    kernel_resume_pc = (uintptr_t)&scheduler_process_return;

    asm volatile("mv sp, %0" :: "r"(p->stack_top));
    current = p->pid;

    p->state = PROC_RUNNING;
    memory_barrier();
    
    p->entry();  // either returns, or triggers ecall + trap

    scheduler_process_return();
}

extern "C" void scheduler_process_return() {
    // Restore kernel stack
    asm volatile("mv sp, %0" :: "r"(kernel_saved_sp));
    memory_barrier();

    // Free resources for previous process if zombie
    Process* p = pid_to_proc(current);
    if (p && p->state == PROC_ZOMBIE) {
        p->state = PROC_FREE;
        p->pid = 0;
        p->entry = nullptr;
        p->name = nullptr;
        p->stack = nullptr;
        p->stack_top = nullptr;
        p->stack_size = 0;
        p->blocked_sem_id = -1;
        p->next_blocked = nullptr;
    }

    current = -1;
}

// Block a process on a semaphore and add to blocked list
static void block_on_semaphore(Process* p, int sem_id) {
    Semaphore* sem = sem_get(sem_id);
    if (!sem) {
        return;
    }

    p->state = PROC_BLOCKED_SEM;
    p->blocked_sem_id = sem_id;
    p->next_blocked = sem->blocked_list;
    sem->blocked_list = p;
}

// Wake one blocked process from a semaphore
static void wake_one_from_semaphore(int sem_id) {
    Semaphore* sem = sem_get(sem_id);
    if (!sem || !sem->blocked_list) {
        return;
    }

    // Wake the first blocked process
    Process* p = sem->blocked_list;
    sem->blocked_list = p->next_blocked;
    p->next_blocked = nullptr;
    p->state = PROC_READY;
    p->blocked_sem_id = -1;
}

// ---------------------------------------------------------------------
// Public API - Process Management
// ---------------------------------------------------------------------
bool scheduler_init() {
    for (int i = 0; i < MAX_PROCS; ++i) {
        proc_table[i].pid = 0;
        proc_table[i].name = nullptr;
        proc_table[i].entry = nullptr;
        proc_table[i].stack = nullptr;
        proc_table[i].stack_top = nullptr;
        proc_table[i].stack_size = 0;
        proc_table[i].state = PROC_FREE;
        proc_table[i].blocked_sem_id = -1;
        proc_table[i].next_blocked = nullptr;
    }

    for (int i = 0; i < MAX_SEMS; ++i) {
        sem_table[i].id = 0;
        sem_table[i].value = 0;
        sem_table[i].owner_pid = 0;
        sem_table[i].blocked_list = nullptr;
        sem_table[i].in_use = false;
    }

    next_pid = 1;
    next_sem_id = 1;
    current = -1;
    return true;
}

int create_process(void (*entry)(), const char* name, uint32_t stack_size) {
    Process* slot = find_free_slot();
    if (!slot) {
        return -1;
    }

    int slot_idx = slot - proc_table;

    void* stk = kmalloc(stack_size);
    if (!stk) {
        return -1;
    }

    slot->pid = next_pid++;
    slot->entry = entry;
    slot->stack = (uint8_t*)stk;
    slot->stack_size = stack_size;
    slot->stack_top = slot->stack + slot->stack_size;
    slot->stack_top = (uint8_t*)((uintptr_t)slot->stack_top & ~0xFULL);
    slot->blocked_sem_id = -1;
    slot->next_blocked = nullptr;

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
    itoa(slot->pid, pid_str, 10);
    print_str("(scheduler) Process created for '");
    print_str(name);
    print_str("' [PID ");
    print_str(pid_str);
    print_str("].\n");

    return slot->pid;
}

int create_process_from_binary(const uint8_t* binary, uint32_t binary_size,
                                const char* name, uint32_t stack_size) {
    Process* slot = find_free_slot();
    if (!slot) {
        return -1;
    }

    int slot_idx = slot - proc_table;

    uint32_t code_size = (binary_size + 15) & ~15ULL;
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

    memcpy(code_mem, binary, binary_size);

    slot->pid = next_pid++;
    slot->entry = (void(*)())code_mem;
    slot->stack = (uint8_t*)stack_mem;
    slot->stack_size = stack_size;
    slot->stack_top = slot->stack + slot->stack_size;
    slot->stack_top = (uint8_t*)((uintptr_t)slot->stack_top & ~0xFULL);
    slot->blocked_sem_id = -1;
    slot->next_blocked = nullptr;

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
    itoa(slot->pid, pid_str, 10);
    print_str("(scheduler) Process created for '");
    print_str(name);
    print_str("' [PID ");
    print_str(pid_str);
    print_str("].\n");

    return slot->pid;
}

void schedule_yield() {
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
    return proc_table;
}

int scheduler_get_max_procs() {
    return MAX_PROCS;
}

Process* scheduler_get_proc_by_pid(int pid) {
    return pid_to_proc(pid);
}

int scheduler_run_pid(int pid) {
    Process *p = pid_to_proc(pid);
    if (!p) return -1;
    run_process(p);
    return 0;
}

// ---------------------------------------------------------------------
// Public API - Semaphore Management
// Note: In cooperative multitasking, we don't need locks around these
// because processes only yield at explicit points (ecall)
// ---------------------------------------------------------------------
int sem_create(int initial_value) {
    Semaphore* slot = find_free_sem_slot();
    if (!slot) {
        return -1;
    }

    int sem_id = next_sem_id++;
    slot->id = sem_id;
    slot->value = initial_value;
    slot->owner_pid = current;
    slot->blocked_list = nullptr;
    slot->in_use = true;

    return sem_id;
}

void sem_wait(int sem_id) {
    Semaphore* sem = sem_get(sem_id);
    if (!sem) {
        return;
    }

    sem->value--;

    if (sem->value < 0) {
        // Block this process
        Process* p = pid_to_proc(current);
        if (p) {
            p->state = PROC_BLOCKED_SEM;
            p->blocked_sem_id = sem_id;
            p->next_blocked = sem->blocked_list;
            sem->blocked_list = p;
        }
        
        // Jump back to scheduler
        asm volatile("mv sp, %0" :: "r"(kernel_saved_sp));
        asm volatile("jr %0" :: "r"(kernel_resume_pc));
        // Never reaches here
    }
}

void sem_signal(int sem_id) {
    Semaphore* sem = sem_get(sem_id);
    if (!sem) {
        return;
    }

    sem->value++;

    // If there are blocked processes, wake one
    if (sem->value <= 0 && sem->blocked_list) {
        Process* p = sem->blocked_list;
        sem->blocked_list = p->next_blocked;
        p->next_blocked = nullptr;
        p->state = PROC_READY;
        p->blocked_sem_id = -1;
    }
}

bool sem_destroy(int sem_id) {
    for (int i = 0; i < MAX_SEMS; ++i) {
        if (sem_table[i].id == sem_id && sem_table[i].in_use) {
            sem_table[i].in_use = false;
            sem_table[i].id = 0;
            sem_table[i].value = 0;
            sem_table[i].blocked_list = nullptr;
            return true;
        }
    }
    return false;
}

Semaphore* sem_get(int sem_id) {
    for (int i = 0; i < MAX_SEMS; ++i) {
        if (sem_table[i].id == sem_id && sem_table[i].in_use) {
            return &sem_table[i];
        }
    }
    return nullptr;
}

// ---------------------------------------------------------------------
// scheduler_main - Concurrent round-robin with blocking support
// ---------------------------------------------------------------------
void scheduler_main() {
    print_str("(scheduler) Entering main loop with concurrent support...\n");

    scheduler_init();

    bool has_any = false;
    for (int i = 0; i < MAX_PROCS; ++i) {
        if (proc_table[i].state != PROC_FREE) {
            has_any = true;
            break;
        }
    }
    if (!has_any) {
        int pid = create_process((void(*)())shell_main, "shell", DEFAULT_STACK_SIZE);
        if (pid < 0) {
            print_str("(scheduler) Failed to create shell process...\n");
        }
    }

    int start_idx = 0;

    while (1) {
        Process* next = find_next_ready(start_idx);
        
        if (next) {
            int next_idx = next - proc_table;
            start_idx = (next_idx + 1) % MAX_PROCS;
            run_process(next);
        } else {
            // No ready processes: idle
            asm volatile("wfi");
        }
    }
}
