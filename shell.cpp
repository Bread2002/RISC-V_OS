/* Copyright (c) 2025, Rye Stahle-Smith
December 2nd, 2025 - shell.cpp
Description: Interactive command-line shell providing filesystem operations, process control, directory navigation, and user program invocation. */
#include "shell.h"
#include "fat.h"
#include "scheduler.h"
#include "embedded_user_programs.h"

// Create a single global FAT instance
FAT fat;
Directory* cwd = nullptr;
char cwd_path[128] = "/";

volatile uint64_t* const UART0 = (uint64_t*)0x10000000;

// Minimal UART console
extern "C" void putchar(char c) { *UART0 = (uint64_t)c; }
extern "C" char getchar() {
    volatile uint8_t* uart = (volatile uint8_t*)0x10000000;
    while ((uart[5] & 0x01) == 0); // wait until data ready
    return uart[0]; // read character
}

extern "C" void print_str(const char* s) {
    while (*s) putchar(*s++);
}

extern "C" void print_hex(uint32_t val) {
    char buf[11]; // "0x" + 8 hex digits + null
    buf[0] = '0';
    buf[1] = 'x';
    
    for (int i = 7; i >= 0; i--) {
        uint8_t nibble = (val >> (i * 4)) & 0xF;
        buf[9 - i] = (nibble < 10) ? ('0' + nibble) : ('a' + nibble - 10);
    }
    buf[10] = '\0';
    
    print_str(buf);
}

extern "C" int strcmp(const char* a, const char* b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return *(unsigned char*)a - *(unsigned char*)b;
}

extern "C" int strncmp(const char* a, const char* b, int n) {
    for (int i = 0; i < n; i++) {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = (unsigned char)b[i];
        if (ca != cb) return ca - cb;
        if (ca == '\0') return 0;
    }
    return 0;
}

extern "C" void strcpy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0'; // null-terminate
}

char* strncpy(char* dest, const char* src, int n) {
    int i = 0;
    for (; i < n && src[i] != '\0'; i++)
        dest[i] = src[i];

    // pad the rest with nulls
    for (; i < n; i++)
        dest[i] = '\0';

    return dest;
}

extern "C" void strcat(char* dest, const char* src) {
    // move to the end of dest
    while (*dest) dest++;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0'; // null-terminate
}

extern "C" const char* strrchr(const char* s, int c) {
    const char* last = nullptr;

    while (*s) {
        if (*s == (char)c) {
            last = s;
        }
        s++;
    }

    // Check for c == '\0' — return pointer to end of string
    if (c == 0) {
        return s;
    }

    return last;
}

extern "C" int strlen(const char* str) {
    if (!str) return 0;

    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

extern "C" void* memset(void* s, int c, int n) {
    unsigned char* p = (unsigned char*)s;
    for (int i = 0; i < n; i++) p[i] = (unsigned char)c;
    return s;
}

extern "C" void* memcpy(void* dest, const void* src, int n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    for (int i = 0; i < n; i++) d[i] = s[i];
    return dest;
}

void itoa(uint32_t value, char* str, int base) {
    char buffer[16]; // enough for 32-bit numbers
    int i = 0;

    if (value == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }

    while (value > 0 && i < (int)sizeof(buffer)-1) {
        int digit = value % base;
        buffer[i++] = (digit < 10) ? ('0' + digit) : ('A' + (digit-10));
        value /= base;
    }

    // reverse the string into str
    int j = 0;
    while (i > 0) {
        str[j++] = buffer[--i];
    }
    str[j] = '\0';
}

void cmd_echo(const char* args) {
    print_str(args);
    print_str("\n");
}

void cmd_clear(const char* args) {
    // ANSI escape code to clear screen and reset cursor
    print_str("\033[2J\033[H");
}

// --- Filesystem commands ---
void cmd_mkdir(const char* args) {
    if (!args || strlen(args) == 0) { print_str("Usage: mkdir <path>\n"); return; }

    Directory* created = fat.mkdir_recursive(cwd, args);
    if (created) print_str("Directory created.\n");
    else print_str("Failed to create directory.\n");
}

void cmd_rmdir(const char* args) {
    if (fat.rmdir(cwd, args)) {
        print_str("Directory removed.\n");
    } else {
        print_str("Failed to remove directory (not empty or does not exist).\n");
    }
}

void cmd_ls(const char* args) {
    if (!args || args[0] == '\0') {
        fat.ls(cwd); // just current directory
    } else {
        fat.ls(cwd, args); // path provided
    }
}

void cmd_touch(const char* args) {
    char name[32];
    Directory* parent = fat.touch_recursive(cwd, args, name);

    if (!parent) {
        print_str("Invalid path.\n");
        return;
    }

    if (fat.touch(parent, name))
        print_str("File created.\n");
    else
        print_str("Failed to create file.\n");
}

void cmd_rm(const char* args) {
    if (fat.rm(cwd, args)) {
        print_str("File removed.\n");
    } else {
        print_str("File not found.\n");
    }
}

// Resolve "./" prefix for relative paths
const char* resolve_path(const char* path) {
    if (strncmp(path, "./", 2) == 0) return path + 2; // skip "./"
    return path;
}

// Traverse a path from a starting directory
Directory* traverse_path(const char* path, Directory* start_dir) {
    if (!path || !*path) return nullptr;
    Directory* d = (path[0] == '/') ? fat.get_root() : start_dir;

    char buf[32];
    int buf_idx = 0;
    for (const char* p = path; ; p++) {
        if (*p == '/' || *p == '\0') {
            buf[buf_idx] = '\0';
            if (buf_idx > 0) {
                if (strcmp(buf, ".") == 0) {
                    // current directory, do nothing
                } else if (strcmp(buf, "..") == 0) {
                    if (d->parent) d = d->parent;
                } else {
                    Directory* next = fat.find_subdir(d, buf);
                    if (!next) return nullptr;
                    d = next;
                }
            }
            buf_idx = 0;
            if (*p == '\0') break;
        } else {
            if (buf_idx < 31) buf[buf_idx++] = *p;
        }
    }
    return d;
}

void cmd_mv(const char* args) {
    char src[32], dest[32];
    int i = 0;

    // Parse source
    while (args[i] && args[i] != ' ' && i < 31) src[i++] = args[i];
    src[i] = '\0';

    // Skip spaces
    while (args[i] == ' ') i++;

    // Parse destination
    int j = 0;
    while (args[i] && j < 31) dest[j++] = args[i++];
    dest[j] = '\0';

    // Resolve source relative path
    const char* src_name = resolve_path(src);

    // Resolve destination directory
    Directory* dest_dir = traverse_path(dest, cwd);
    if (!dest_dir) {
        print_str("Move failed: invalid destination\n");
        return;
    }

    if (fat.mv(cwd, src_name, dest_dir)) {
        print_str("Moved successfully.\n");
    } else {
        print_str("Move failed.\n");
    }
}

void cmd_cd(const char* path) {
    if (!path || strlen(path) == 0) return;

    Directory* dir = (path[0] == '/') ? fat.get_root() : cwd;
    char component[MAX_NAME_LEN];
    int i = 0;

    while (*path) {
        // Extract next component
        i = 0;
        while (*path && *path != '/' && i < MAX_NAME_LEN-1) {
            component[i++] = *path++;
        }
        component[i] = '\0';
        if (*path == '/') path++; // skip '/'

        if (strcmp(component, "..") == 0) {
            if (dir->parent) dir = dir->parent;
        } else if (strlen(component) > 0) {
            Directory* next = fat.find_subdir(dir, component);
            if (!next) {
                print_str("Error: directory not found\n");
                return;
            }
            dir = next;
        }
    }

    cwd = dir;
}

void cmd_ps(const char* args) {
    Process* table = scheduler_get_process_table();
    int max = scheduler_get_max_procs();

    print_str("PID\tName\t\tState\n");
    print_str("-------------------------------\n");

    for (int i = 0; i < max; ++i) {
        Process* p = &table[i];
        if (p->state == PROC_FREE) continue;

        // PID
        char buf[8];
        itoa(p->pid, buf, 10);
        print_str(buf);
        print_str("\t");

        // Name
        if (p->name) {
            print_str(p->name);

            int len = strlen(p->name);

            if (len < 8)
                print_str("\t\t");   // short name → more spacing
            else
                print_str("\t");     // long name → single tab
        } else {
            print_str("(no name)\t");
        }

        // State
        switch (p->state) {
            case PROC_READY:   print_str("READY"); break;
            case PROC_RUNNING: print_str("RUNNING"); break;
            case PROC_SLEEP:   print_str("SLEEP"); break;
            case PROC_ZOMBIE:  print_str("ZOMBIE"); break;
            default:           print_str("UNKNOWN"); break;
        }
        print_str("\n");
    }
}

void update_cwd_path() {
    Directory* temp = cwd;
    char rev_path[128] = "";

    while (temp) {
        if (temp->parent == nullptr) {
            if (rev_path[0] == '\0') {
                strcpy(rev_path, "/");
            } else {
                char buffer[128] = "/";
                strcat(buffer, rev_path);
                strcpy(rev_path, buffer);
            }
            break;
        }

        char buffer[128];
        strcpy(buffer, temp->name);

        if (rev_path[0] != '\0') {
            strcat(buffer, "/");
            strcat(buffer, rev_path);
        }

        strcpy(rev_path, buffer);
        temp = temp->parent;
    }

    // Save into global variable
    strcpy(cwd_path, rev_path);
}

void cmd_pwd(const char* args) {
    update_cwd_path();
    print_str(cwd_path);
    print_str("\n");
}

void cmd_cat(const char* args) {
    if (!args || strlen(args) == 0) {
        print_str("Usage: cat <filename>\n");
        return;
    }

    File* f = fat.find_file(cwd, args);
    if (!f) {
        print_str("File not found\n");
        return;
    }

    for (int i = 0; i <= f->size; i++) {
        putchar(f->data[i]);
    }
    putchar('\n');
}

void cmd_edit(const char* args, bool append_mode = false) {
    if (!args || strlen(args) == 0) {
        print_str("Usage: edit|append <filename>\n");
        return;
    }

    File* f = fat.find_file(cwd, args);
    if (!f) {
        print_str("File not found\n");
        return;
    }

    int pos = append_mode ? f->size : 0;
    if (!append_mode) f->size = 0; // clear for normal edit

    print_str(append_mode ? "Append mode (Ctrl+D to finish):\n" 
                          : "Enter new content (end with Ctrl+D):\n");

    while (pos < MAX_FILE_SIZE) {
        char c = getchar();

        if (c == 4) break; // Ctrl+D ends input

        // Handle newlines correctly
        if (c == '\r' || c == '\n') {
            putchar('\n');    // move cursor down
            f->data[pos++] = '\n'; // store LF only
        } else {
            putchar(c);
            f->data[pos++] = c;
        }
    }

    f->size = pos;
    f->used = true;
    print_str("\nFile updated.\n");
}

void cmd_df(const char* args) {
    char buf[32];

    int used_dirs = fat.count_used_dirs();
    int free_dirs = fat.count_free_dirs();
    int used_files = fat.count_used_files();
    int free_files = fat.count_free_files();
    uint32_t total_bytes = fat.total_file_bytes();

    print_str("Resource\tUsed\tFree\tMax\n");
    print_str("-------------------------------------\n");

    itoa(used_dirs, buf, 10);
    print_str("Directories\t"); print_str(buf); print_str("\t");
    itoa(free_dirs, buf, 10);
    print_str(buf); print_str("\t");
    itoa(MAX_DIRS, buf, 10);
    print_str(buf); print_str("\n");

    itoa(used_files, buf, 10);
    print_str("Files\t\t"); print_str(buf); print_str("\t");
    itoa(free_files, buf, 10);
    print_str(buf); print_str("\t");
    itoa(MAX_FILES, buf, 10);
    print_str(buf); print_str("\n\n");

    itoa(total_bytes / 1024, buf, 10);
    print_str("Used Space: "); print_str(buf); print_str(" KB\n");

    itoa((MAX_FILES * MAX_FILE_SIZE) / 1024, buf, 10);
    print_str("Total Space: "); print_str(buf); print_str(" MB\n");
}

void cmd_edit_wrapper(const char* args) { cmd_edit(args, false); }
void cmd_append_wrapper(const char* args) { cmd_edit(args, true); }

// Run program with visual display
void cmd_run(const char* args) {
    if (!args || strlen(args) == 0) {
        print_str("Usage: run <program.S>\n");
        return;
    }

    // Must be in /user_programs
    if (!cwd || strcmp(cwd->name, "user_programs") != 0) {
        print_str("Error: No user programs were found\n");
        return;
    }

    // Require .S extension
    const char* ext = strrchr(args, '.');
    if (!ext || strcmp(ext, ".S") != 0) {
        print_str("Error: You must specify an assembly (.S) file\n");
        return;
    }

    // Build base name without .S
    char base[64];
    int base_len = (int)(ext - args);  // number of chars before ".S"
    if (base_len <= 0 || base_len >= (int)sizeof(base)) {
        print_str("Error: Invalid program name\n");
        return;
    }

    for (int i = 0; i < base_len; ++i) {
        base[i] = args[i];
    }
    base[base_len] = '\0';

    // Search for the embedded program by base name (e.g. "counter")
    for (unsigned int i = 0; i < embedded_file_count; i++) {
        if (strcmp(base, embedded_files[i].name) == 0) {
            const EmbeddedFile* prog = &embedded_files[i];

            int pid = create_process_from_binary(
                prog->binary,
                prog->binary_size,
                base,               // process name = "counter", not "counter.S"
                DEFAULT_STACK_SIZE
            );

            if (pid <= 0) {
                print_str("Error: Failed to create process\n");
            } else {
                scheduler_run_pid(pid);
            }
            return;
        }
    }

    print_str("Error: Program has no binary or doesn't exist\n");
}

void cmd_exit(const char* args) {
    print_str("To perform a clean exit, use 'Ctrl+A X'.\n");
    print_str("Otherwise, use 'Ctrl+A C' to enter the QEMU monitor, then type 'quit'.\n");
}

void cmd_help(const char* args) {
    print_str("Available Commands:\n");
    print_str("  • 'help'\t\tShow this help message.\n");
    print_str("  • 'echo <args>'\tEcho arguments.\n");
    print_str("  • 'clear'\t\tClear the screen.\n");
    print_str("  • 'mkdir <name>'\tCreate a new directory.\n");
    print_str("  • 'rmdir <name>'\tRemove a directory.\n");
    print_str("  • 'ls'\t\tList files and directories.\n");
    print_str("  • 'touch <name>'\tCreate a new file.\n");
    print_str("  • 'rm <name>'\t\tDelete a file.\n");
    print_str("  • 'run <name>'\tRun a user program.\n");
    print_str("  • 'mv <src> <dest>'\tMove a file to another directory.\n");
    print_str("  • 'cd <dir>'\t\tChange current directory.\n");
    print_str("  • 'df'\t\tDisplay current storage and resources.\n");
    print_str("  • 'pwd'\t\tPrint current working directory.\n");
    print_str("  • 'ps'\t\tDisplay all currently running processes.\n");
    print_str("  • 'cat <name>'\tDump a file's contents to the console.\n");
    print_str("  • 'edit <name>'\tOverwrite a file's contents.\n");
    print_str("  • 'append <name>'\tAppend to a file's contents.\n");
    print_str("  • 'exit'\t\tAdvises the user on how to exit the OS.\n");
}

struct Command {
    const char* name;
    void (*func)(const char* args);
};

Command commands[] = {
    {"help", cmd_help},
    {"echo", cmd_echo},
    {"clear", cmd_clear},
    {"mkdir", cmd_mkdir},
    {"rmdir", cmd_rmdir},
    {"ls", cmd_ls},
    {"touch", cmd_touch},
    {"rm", cmd_rm},
    {"mv", cmd_mv},
    {"cd", cmd_cd},
    {"df", cmd_df},
    {"pwd", cmd_pwd},
    {"ps", cmd_ps},
    {"cat", cmd_cat},
    {"edit", cmd_edit_wrapper},
    {"run", cmd_run},
    {"append", cmd_append_wrapper},
    {"exit", cmd_exit},
    {nullptr, nullptr}  // sentinel
};

extern "C" void handle_command(const char* line) {
    char cmd[32];
    const char* args = nullptr;

    // Extract command (first word)
    int i = 0;
    while (line[i] && line[i] != ' ' && i < 31) {
        cmd[i] = line[i];
        i++;
    }
    cmd[i] = '\0';

    // Skip space to get args
    args = line + i;
    while (*args == ' ') args++;

    // Lookup command
    for (int j = 0; commands[j].name != nullptr; j++) {
        if (strcmp(cmd, commands[j].name) == 0) {
            commands[j].func(args);
            return;
        }
    }

    print_str("Unknown command: ");
    print_str(cmd);
    print_str("\n");
}

extern "C" void shell_main() {
    char line[128];
    int pos = 0;
    const int min_pos = 0; // Editable area starts after prompt
    char name_buf[64];
    cwd = fat.get_root();

    while (1) {
        // Print current directory in brackets
        print_str("(shell) user [");
        if (strcmp(cwd->name, "") != 0) {
            strcpy(name_buf, "../");
            strcat(name_buf, cwd->name);
            print_str(name_buf);
        } else {
            print_str("/");
        }
        print_str("] > ");      // followed by prompt

        pos = 0;

        // Read a line
        while (1) {
            char c = getchar();

            // Handle escape sequences (arrow keys)
            if (c == 0x1B) { // ESC
                char next1 = getchar();
                char next2 = getchar();
                if (next1 == '[') {
                    // Ignore arrows
                    switch (next2) {
                        case 'D': /* left arrow */ break;
                        case 'A': /* up arrow */ break;
                        case 'C': /* right arrow */ break;
                        case 'B': /* down arrow */ break;
                    }
                }
                continue;
            }

            if (c == '\r' || c == '\n') { // Enter key
                putchar('\n');
                line[pos] = '\0';
                break;
            }

            if (c == '\b' || c == 127) { // Backspace
                if (pos > min_pos) {
                    pos--;
                    print_str("\b \b");
                }
            } else if (pos < 127) {
                line[pos++] = c;
                putchar(c);
            }
        }

        handle_command(line);
    }
}
