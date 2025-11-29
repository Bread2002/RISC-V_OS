/* Copyright (c) 2025, Rye Stahle-Smith
December 2nd, 2025 - shell.h
Description: Shell interface and command dispatcher declarations for the RISC-V OS console environment. */
#ifndef SHELL_H
#define SHELL_H

#pragma once
#include <stdint.h>

extern "C" void shell_main();
extern "C" char getchar();
extern "C" void putchar(char c);
extern "C" void print_str(const char* s);
extern "C" void print_hex(uint32_t val);

extern "C" int strcmp(const char* a, const char* b);
extern "C" int strncmp(const char* a, const char* b, int n);
extern "C" void strcpy(char* dest, const char* src);
extern "C" char* strncpy(char* dest, const char* src, int n);
extern "C" void strcat(char* dest, const char* src);
extern "C" const char* strrchr(const char* s, int c);
extern "C" int strlen(const char* str);
extern "C" void* memset(void* s, int c, int n);
extern "C" void* memcpy(void* dest, const void* src, int n);
void itoa(uint32_t value, char* str, int base = 10);

#endif
