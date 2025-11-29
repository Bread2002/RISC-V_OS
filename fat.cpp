/* Copyright (c) 2025, Rye Stahle-Smith
December 2nd, 2025 - fat.cpp
Description: In-memory FAT-like filesystem implementation supporting directories, files, path traversal, CRUD operations, and resource reporting. */
#include "shell.h"
#include "fat.h"

// Constructor initializes root and pools
FAT::FAT() {
    // mark all pool objects as free
    for (int i = 0; i < MAX_DIRS; i++) dir_pool[i].used = false;
    for (int i = 0; i < MAX_FILES; i++) file_pool[i].used = false;

    // init root
    strcpy(root.name, "/");
    root.parent = nullptr;
    root.subdir_count = 0;
    root.file_count = 0;
    root.used = true;
}

Directory* FAT::get_root() { return &root; }

Directory* FAT::find_subdir_recursive(Directory* start, const char* path) {
    if (!start || !path || !*path) return start;

    // Copy path to buffer so we can split
    char buf[32];
    int i = 0;
    while (path[i] && path[i] != '/' && i < sizeof(buf)-1) {
        buf[i] = path[i];
        i++;
    }
    buf[i] = '\0';

    Directory* next = find_subdir(start, buf); // find immediate child
    if (!next) return nullptr;

    // If we hit a '/', continue recursively
    if (path[i] == '/') return find_subdir_recursive(next, path + i + 1);
    return next; // last component
}

Directory* FAT::find_subdir(Directory* dir, const char* name) {
    for (int i = 0; i < dir->subdir_count; i++) {
        if (strcmp(dir->subdirs[i]->name, name) == 0) return dir->subdirs[i];
    }
    return nullptr;
}

File* FAT::find_file(Directory* dir, const char* name) {
    for (int i = 0; i < dir->file_count; i++) {
        if (strcmp(dir->files[i]->name, name) == 0) return dir->files[i];
    }
    return nullptr;
}

static bool is_name_invalid(const char* name) {
    if (!name || !name[0]) return true;        // empty or null

    bool all_spaces = true;

    for (int i = 0; name[i]; i++) {
        if (name[i] == '/') return true;       // reject: contains '/'

        if (name[i] != ' ') all_spaces = false;
    }

    return all_spaces;                          // reject: name is only spaces
}

// recursive mkdir
Directory* FAT::mkdir_recursive(Directory* start, const char* path) {
    if (!start || !path || *path == '\0') return nullptr;

    Directory* curr = start;
    const char* seg_start = path;
    char seg_name[MAX_NAME_LEN];

    while (*seg_start) {
        // find end of current segment
        const char* seg_end = seg_start;
        while (*seg_end && *seg_end != '/') seg_end++;

        int len = seg_end - seg_start;
        if (len == 0 || len >= MAX_NAME_LEN) return nullptr; // reject empty or too long names
        // copy segment
        for (int i = 0; i < len; i++) seg_name[i] = seg_start[i];
        seg_name[len] = '\0';

        // try to find existing subdir
        Directory* next = find_subdir(curr, seg_name);
        if (!next) {
            // create it
            next = mkdir(curr, seg_name);
            if (!next) return nullptr; // creation failed
        }

        curr = next;

        // skip '/' if present
        seg_start = (*seg_end) ? seg_end + 1 : seg_end;
    }

    return curr;
}

// mkdir uses static pool
Directory* FAT::mkdir(Directory* dir, const char* name) {
    if (is_name_invalid(name)) return nullptr;          // reject empty names
    if (dir->subdir_count >= MAX_DIRS) return nullptr;
    if (find_subdir(dir, name)) return nullptr;

    // find free directory in pool
    for (int i = 0; i < MAX_DIRS; i++) {
        if (!dir_pool[i].used) {
            Directory* new_dir = &dir_pool[i];
            new_dir->used = true;
            strcpy(new_dir->name, name);
            new_dir->parent = dir;
            new_dir->subdir_count = 0;
            new_dir->file_count = 0;
            dir->subdirs[dir->subdir_count++] = new_dir;
            return new_dir;
        }
    }
    return nullptr; // no free directories
}

// rmdir
bool FAT::rmdir(Directory* dir, const char* name) {
    for (int i = 0; i < dir->subdir_count; i++) {
        Directory* sub = dir->subdirs[i];
        if (strcmp(sub->name, name) == 0) {
            if (sub->subdir_count > 0 || sub->file_count > 0) return false; // not empty
            sub->used = false;
            for (int j = i; j < dir->subdir_count - 1; j++) dir->subdirs[j] = dir->subdirs[j+1];
            dir->subdir_count--;
            return true;
        }
    }
    return false;
}

// recursive touch
Directory* FAT::touch_recursive(Directory* start, const char* path, char* out_name) {
    Directory* current = start;

    char part[32];
    int pi = 0;

    int last_slash = -1;
    int len = strlen(path);

    // find last slash to separate filename
    for (int i = 0; i < len; i++)
        if (path[i] == '/') last_slash = i;

    if (last_slash < 0) {
        // no slash → parent is current dir
        strncpy(out_name, path, 31);
        out_name[31] = '\0';
        return current;
    }

    // copy filename
    int idx = 0;
    for (int i = last_slash + 1; i < len && idx < 31; i++)
        out_name[idx++] = path[i];
    out_name[idx] = '\0';

    if (is_name_invalid(out_name))
        return nullptr;

    // resolve parent path
    const char* parent_path = path;
    char buf[64];
    for (int i = 0; i < last_slash && i < 63; i++)
        buf[i] = path[i];
    buf[last_slash] = '\0';

    return find_subdir_recursive(current, buf);
}


// touch uses static pool
File* FAT::touch(Directory* dir, const char* name) {
    if (is_name_invalid(name)) return nullptr;          // reject empty names
    if (dir->file_count >= MAX_FILES) return nullptr;
    if (find_file(dir, name)) return nullptr;

    for (int i = 0; i < MAX_FILES; i++) {
        if (!file_pool[i].used) {
            File* f = &file_pool[i];
            f->used = true;
            strcpy(f->name, name);
            f->size = 0;
            dir->files[dir->file_count++] = f;
            return f;
        }
    }
    return nullptr;
}

// rm
bool FAT::rm(Directory* dir, const char* name) {
    for (int i = 0; i < dir->file_count; i++) {
        if (strcmp(dir->files[i]->name, name) == 0) {
            dir->files[i]->used = false;
            for (int j = i; j < dir->file_count - 1; j++) dir->files[j] = dir->files[j+1];
            dir->file_count--;
            return true;
        }
    }
    return false;
}

// mv
bool FAT::mv(Directory* src_dir, const char* name, Directory* dest_dir) {
    File* f = find_file(src_dir, name);
    if (!f || dest_dir->file_count >= MAX_FILES) return false;

    rm(src_dir, name);
    dest_dir->files[dest_dir->file_count++] = f;
    return true;
}

// ls
void FAT::ls(Directory* cwd, const char* path) {
    Directory* dir = cwd;
    if (path && path[0] != '\0') {
        dir = find_subdir_recursive(cwd, path);
        if (!dir) {
            print_str("Error: invalid directory\n");
            return;
        }
    }

    print_str("Directories:\n");
    if (dir->subdir_count == 0) print_str("  • (none)\n");
    for (int i = 0; i < dir->subdir_count; i++) {
        print_str("  • ");
        print_str(dir->subdirs[i]->name);
        print_str("\n");
    }

    print_str("Files:\n");
    if (dir->file_count == 0) print_str("  • (none)\n");
    for (int i = 0; i < dir->file_count; i++) {
        print_str("  • ");
        print_str(dir->files[i]->name);
        print_str("\n");
    }
}

// Returns the number of used directories in the pool
int FAT::count_used_dirs() const {
    int cnt = 0;
    for (int i = 0; i < MAX_DIRS; i++) if (dir_pool[i].used) cnt++;
    return cnt;
}

// Returns the number of free directories
int FAT::count_free_dirs() const { return MAX_DIRS - count_used_dirs(); }

// Returns the number of used files in the pool
int FAT::count_used_files() const {
    int cnt = 0;
    for (int i = 0; i < MAX_FILES; i++) if (file_pool[i].used) cnt++;
    return cnt;
}

// Returns the number of free files
int FAT::count_free_files() const { return MAX_FILES - count_used_files(); }

// Returns total bytes used by all files
uint32_t FAT::total_file_bytes() const {
    uint32_t total = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_pool[i].used) total += file_pool[i].size;
    }
    return total;
}
