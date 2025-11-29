/* Copyright (c) 2025, Rye Stahle-Smith
December 2nd, 2025 - fat.h
Description: Definitions for FAT-style filesystem structures, limits, and public APIs used by the kernel and shell. */
#ifndef FAT_H
#define FAT_H

#pragma once
#include <stdint.h>

constexpr int MAX_NAME_LEN = 16;
constexpr int MAX_FILES = 64;
constexpr int MAX_DIRS = 16;
constexpr int MAX_FILE_SIZE = 16384;  // ~16 KB

struct File {
    char name[MAX_NAME_LEN];
    uint8_t data[MAX_FILE_SIZE];
    int size;
    bool used;
};

struct Directory {
    char name[MAX_NAME_LEN];
    Directory* parent;
    Directory* subdirs[MAX_DIRS];
    int subdir_count;
    File* files[MAX_FILES];
    int file_count;
    bool used;
};

class FAT {
public:
    FAT();
    Directory* get_root();

    Directory* mkdir(Directory* dir, const char* name);
    Directory* mkdir_recursive(Directory* dir, const char* path);
    bool rmdir(Directory* dir, const char* name);
    File* touch(Directory* dir, const char* name);
    Directory* touch_recursive(Directory* start, const char* path, char* out_name);
    bool rm(Directory* dir, const char* name);
    bool mv(Directory* src_dir, const char* name, Directory* dest_dir);
    void ls(Directory* dir, const char* path = nullptr);

    // helper find functions
    Directory* find_subdir_recursive(Directory* start, const char* path);
    Directory* find_subdir(Directory* dir, const char* name);
    File* find_file(Directory* dir, const char* name);

    int count_used_dirs() const;
    int count_free_dirs() const;
    int count_used_files() const;
    int count_free_files() const;
    uint32_t total_file_bytes() const;

private:
    Directory root;

    // object pools
    Directory dir_pool[MAX_DIRS];
    File file_pool[MAX_FILES];
};
extern FAT fat;

#endif
