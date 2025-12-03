# RISC-V OS: A Minimal Yet Capable Educational Operating System
### Copyright (c) 2025, Rye Stahle-Smith

---

### 📌 Project Overview

**RISC-V OS** is a compact, modular operating system designed for bare-metal RISC-V environments. It boots from a handwritten assembly loader, enters Supervisor mode, initializes a lightweight kernel, mounts a virtual in-memory FAT-style filesystem, and launches a cooperative scheduler that manages both system tasks and embedded user programs. The project itself was developed with assistance from large language models (LLMs), including Claude and ChatGPT, to accelerate design exploration, refine system architecture, and streamline debugging throughout the build process.

In addition to built-in components, the OS also supports user-provided assembly programs: any `.S` files embedded prior to boot are automatically detected, converted into internal binary/ source pairs, and written into the virtual filesystem under `/user_programs` during startup. This allows developers to inject custom user programs directly into the image and have them appear as local files at runtime, ready to be inspected or executed via the shell.

This OS is intentionally minimal so developers and students can explore low-level systems topics (interrupts, trap handling, privilege modes, memory allocation, context frames, and process scheduling) without wading through the complexity of a large kernel.

---

### 🚀 Prerequisites

-   QEMU Emulator (`qemu-system-riscv64`)
-   RISC-V GCC Toolchain (`riscv64-unknown-elf-gcc`, `-ld`, `-objcopy`)
-   `make`

To install `make`:
``` bash
sudo apt update
sudo apt install build-essential

make --version  # Verify installation
```

To install QEMU:
``` bash
sudo apt install qemu-system-misc

qemu-system-riscv64 --version  # Verify installation
```

To install the RISC-V GCC Toolchain:
``` bash
sudo apt install gcc-riscv64-unknown-elf
sudo apt install binutils-riscv64-unknown-elf

riscv64-unknown-elf-gcc --version  # Verify installation
```

---

### 📂 Building & Running the OS

To initialize and run the operating system:
``` bash
make all
make run
```

To clean the project:
``` bash
make clean
```

To deep clean the project (remove user programs):
``` bash
make deep_clean
```

---

### 🖥️ Shell Commands Overview

The RISC-V OS includes a minimal Unix-like command shell that allows interaction with the virtual filesystem, process scheduler, and user-program subsystem. Below is a complete reference of all supported commands and their behaviors.

#### 📚 General Commands

| Command       | Description                                           |
|---------------|-------------------------------------------------------|
| `help`        | Display all available commands and their usage hints. |
| `echo <args>` | Print the provided text to the console.               |
| `clear`       | Clear the console using ANSI escape sequences.        |
| `exit`        | Advises the user on how to exit the OS.               |

---

#### 📁 Filesystem Management

| Command            | Description                                                             |
|--------------------|-------------------------------------------------------------------------|
| `mkdir <name>`     | Create a new directory (recursively if needed).                         |
| `rmdir <name>`     | Remove a directory **only if empty**.                                   |
| `ls` or `ls <path>`| List directories and files in the current or given path.                |
| `touch <path>`     | Create a file, creating intermediate directories if required.           |
| `rm <name>`        | Remove a file from the current directory.                               |
| `mv <src> <dest>`  | Move a file to another directory.                                       |
| `cd <dir>`         | Change the current working directory.                                   |
| `pwd`              | Print the absolute path of the current directory.                       |
| `cat <name>`       | Display the contents of a file to the console.                          |
| `edit <name>`      | Overwrite the contents of a file (ends with Ctrl+D).                    |
| `append <name>`    | Append to an existing file (also ends with Ctrl+D).                     |
| `df`               | Display resource usage: used/free directory entries, files, and storage.|

---

#### 🧵 Process & Program Control

| Command          | Description                                                           |
|------------------|-----------------------------------------------------------------------|
| `ps`             | Display all active processes, their PIDs, names, and states.          |
| `run <program.S>`| Execute an embedded user program from `/user_programs`.               |

Running programs requires being inside the `/user_programs` directory.  
Programs originate from embedded `.S` files supplied at build time; the OS converts them into local files on boot and exposes them to the shell.

---

### 🧠 System Architecture

#### Bootloader

Initializes stack, trap vectors, prints boot message, jumps to kernel.

#### Kernel

Initializes services: console, scheduler, memory, traps, filesystem, user programs.

#### Scheduler

Cooperative round‑robin with PID assignment and cleanup.

#### Shell

An interactive CLI with command dispatch and user-program execution.

#### Filesystem

A FAT-like structured memory with directories and files, supports various commands.

#### Memory

In-memory system with bump allocation, page allocation, and process memory setup.

#### Traps

Includes a full register save/restore, syscall handling, and error reporting for protection.

---

### 📦 Repository Structure

    riscv-os/
    ├── Makefile
    ├── boot.S
    ├── kernel.cpp
    ├── trap.S
    ├── trap.cpp
    ├── scheduler.cpp
    ├── scheduler.h
    ├── memory.cpp
    ├── memory.h
    ├── fat.cpp
    ├── fat.h
    ├── shell.cpp
    ├── shell.h
    ├── embedded_user_programs.h
    ├── user_program.ld
    ├── linker.ld
    └── user_programs/  # Upload user programs here
        ├── hello.S
        ├── counter.S
        └── fibonacci.S
