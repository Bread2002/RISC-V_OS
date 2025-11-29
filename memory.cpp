/* Copyright (c) 2025, Rye Stahle-Smith
December 2nd, 2025 - memory.cpp
Description: Simple bump allocator and memory utilities used by the kernel and process loader for stack, heap, and program memory allocation. */
#include "shell.h"
#include "memory.h"

// ------------------------------------------------------------
//  Simple bump allocator
// ------------------------------------------------------------
// Change this to wherever you want your kernel heap to begin.
// For now, we'll just define a linker symbol and assign it.

extern uint8_t _kernel_heap_start;   // defined in linker.ld
extern uint8_t _kernel_heap_end;     // defined in linker.ld

static uint8_t* heap_ptr = &_kernel_heap_start;
static uint8_t* heap_limit = &_kernel_heap_end;

// ------------------------------------------------------------
// Allocate 'size' bytes from kernel heap, naturally aligned.
// No deallocation yet (not needed until later)
// ------------------------------------------------------------
void* kmalloc(uint64_t size) {
    if (size == 0) return nullptr;

    // Align to 16 bytes
    size = (size + 15) & ~15ULL;

    if (heap_ptr + size >= heap_limit) {
        print_str("(memory) Out of memory!\n");
        return nullptr;
    }

    void* result = heap_ptr;
    heap_ptr += size;

    return result;
}

// ------------------------------------------------------------
// Page allocator for processes (4 KiB pages)
// ------------------------------------------------------------

static const uint64_t PAGE_SIZE = 4096;

void* alloc_page() {
    return kmalloc(PAGE_SIZE);
}

// ------------------------------------------------------------
// Per-process memory allocation helpers
// ------------------------------------------------------------

// Allocate memory for a new process: code + stack
ProcessMemory alloc_process_memory(uint64_t code_size, uint64_t stack_size) {
    ProcessMemory mem;

    mem.code = (uint8_t*) kmalloc(code_size);
    mem.code_size = code_size;

    mem.stack = (uint8_t*) kmalloc(stack_size);
    mem.stack_size = stack_size;

    if (!mem.code || !mem.stack) {
        print_str("(memory) Failed to allocate process memory\n");
        mem.code = nullptr;
        mem.stack = nullptr;
    }

    return mem;
}
