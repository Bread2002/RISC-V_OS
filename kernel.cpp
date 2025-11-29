/* Copyright (c) 2025, Rye Stahle-Smith
December 2nd, 2025 - kernel.cpp
Description: Core kernel initialization for console, scheduler, memory, traps, filesystem, and embedded user programs. Entry point after boot. */
#include <stdint.h>
#include "fat.h"
#include "shell.h"
#include "scheduler.h"
#include "embedded_user_programs.h"

extern FAT fat;
extern "C" uint8_t _kernel_heap_start[];
extern "C" uint8_t _kernel_heap_end[];

bool service_memory() {
    uint8_t* start = _kernel_heap_start;
    uint8_t* end   = _kernel_heap_end;

    // Basic sanity check: end must be > start
    if (end <= start)
        return false;

    // Write a test pattern and verify
    start[0] = 0xAA;
    start[1] = 0x55;

    return (start[0] == 0xAA && start[1] == 0x55);
}

bool service_traps() {
    uint64_t stvec;
    asm volatile("csrr %0, stvec" : "=r"(stvec));
    return stvec != 0;
}

bool service_scheduler() { return scheduler_init(); }

bool service_filesystem() { return (fat.get_root() != nullptr); }

bool service_userprog() {
    // Check if we have any embedded programs
    if (embedded_file_count == 0) {
        return false;
    }

    // Create user_programs directory
    Directory* dir = fat.mkdir_recursive(fat.get_root(), "user_programs");
    if (!dir) {
        return false;
    }

    // Create a file for each embedded program
    for (unsigned int i = 0; i < embedded_file_count; i++) {
        const EmbeddedFile* ef = &embedded_files[i];

        // Create filename with .S extension
        char filename[MAX_NAME_LEN];
        int j = 0;
        
        // Copy name
        while (ef->name[j] && j < MAX_NAME_LEN - 3) {
            filename[j] = ef->name[j];
            j++;
        }
        filename[j] = '\0';
        
        // Append .S
        filename[j] = '.';
        filename[j+1] = 'S';
        filename[j+2] = '\0';

        File* f = fat.touch(dir, filename);
        if (!f) {
            return false;
        }

        // Copy assembly source code into virtual FAT file
        uint32_t copy_size = ef->source_size;
        if (copy_size > MAX_FILE_SIZE) {
            copy_size = MAX_FILE_SIZE;
        }

        for (uint32_t b = 0; b < copy_size; b++) {
            f->data[b] = ef->source[b];
        }
        
        // Set file size (don't add extra null terminator if source already has one)
        f->size = copy_size;
        f->used = true;
    }

    return true;
}

struct Service { const char* name; bool (*check)(); };
Service services[] = {
    {"scheduler", service_scheduler},
    {"memory", service_memory},
    {"traps", service_traps},
    {"filesystem", service_filesystem},
    {"user programs", service_userprog}
};

// Helper to print current privilege level
extern "C" void print_current_mode() {
    uint64_t mstatus;
    asm volatile("csrr %0, mstatus" : "=r"(mstatus));

    uint64_t mpp = (mstatus >> 11) & 0x3; // MPP bits
    uint64_t spp = (mstatus >> 8) & 0x1;  // SPP bit for sret
    // This is approximate; we also know our boot sequence
    // For display purposes:
    if (mpp == 3) print_str("Machine Mode");      // MPP=11 -> M-mode
    else if (mpp == 1) print_str("Supervisor Mode"); // MPP=01 -> S-mode
    else print_str("User Mode");                  // fallback
}

extern "C" void kernel_main() {
    print_str("(kernel) ");
    print_current_mode();
    print_str(" Active. Starting RISC-V OS v1.0...\n");

    print_str("(kernel) Initializing services:\n");
    print_str("  • console........ OK\n");
    for (int i = 0; i < sizeof(services)/sizeof(services[0]); i++) {
        print_str("  • ");
        print_str(services[i].name);
        print_str("........ ");
        print_str(services[i].check() ? "OK\n" : "FAIL\n");
    }

    print_str("\n(kernel) System ready. Starting scheduler...\n");
    print_str("================================\n\n");

    // Hand off to scheduler
    scheduler_main();

    // Loop forever (should never return)
    while (1) asm volatile("wfi");
}
