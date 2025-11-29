/* Copyright (c) 2025, Rye Stahle-Smith
December 2nd, 2025 - embedded_user_programs.h
Description: Data structures and declarations for storing embedded user program binaries and source code, generated dynamically at build time. */
#ifndef EMBEDDED_USER_PROGRAMS_H
#define EMBEDDED_USER_PROGRAMS_H

#include <stdint.h>

struct EmbeddedFile {
    const char* name;
    const uint8_t* binary;    // compiled binary
    uint32_t binary_size;
    const char* source;       // assembly source code text
    uint32_t source_size;
};

extern const EmbeddedFile embedded_files[];
extern const unsigned int embedded_file_count;

#endif
