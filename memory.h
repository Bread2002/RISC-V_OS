/* Copyright (c) 2025, Rye Stahle-Smith
December 2nd, 2025 - memory.h
Description: Memory allocation interface providing declarations for the bump allocator and kernel memory utilities. */
#ifndef MEMORY_H
#define MEMORY_H

#pragma once
#include <stdint.h>

// Structure describing a new process's memory (code + stack)
struct ProcessMemory {
    uint8_t* code;
    uint64_t code_size;

    uint8_t* stack;
    uint64_t stack_size;
};

// Basic heap allocator
void* kmalloc(uint64_t size);

// Page allocator (4 KiB)
void* alloc_page();

// Allocate memory regions for a process
ProcessMemory alloc_process_memory(uint64_t code_size, uint64_t stack_size);

#endif
