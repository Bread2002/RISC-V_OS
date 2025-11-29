# Curated Questions for Building a RISC-V OS
### Copyright (c) 2025, Rye Stahle-Smith

---

## 📌 File Description
This file is a curated set of questions that a developer can use to guide an LLM (ChatGPT, Claude, Gemini, etc.)through the process of building a similar RISC‑V operating system. Each question corresponds to a conceptual milestone in this OS (bootloading, trap handling, scheduling, filesystem design, and user‑program execution, etc.)

---

## 🧩 1. Understanding the Goal (High-Level Concepts)

### Phase 1: “What am I even building?”
- “How might I build a simple RISC-V operating system from scratch?”
- “What are the minimum components the OS might need?”

### Phase 2: Tools & Environment
- “How do I install the RISC-V GCC toolchain?”
- “How do I run a bare-metal RISC-V binary in QEMU?”

---

## 🧱 2. Bootloader & Startup Assembly

### Phase 3: Bootloader Basics
- “How do I write a minimal RISC-V bootloader in assembly?”
- “How do I set up the stack pointer in RISC-V assembly?”
- “How do I pass control from a bootloader to a C function?”
- “How do I configure the RISC-V trap vector during boot?”

### Phase 4: Printing Text
- “How do I print characters to a UART device in bare-metal RISC-V?”
- “What memory-mapped IO address does QEMU use for UART?”

---

## 🔧 3. Kernel Initialization

### Phase 5: Kernel Basics
- “How do I write the entry point of a RISC-V kernel in C?”
- “How should I structure initialization code in a small OS?”

### Phase 6: Services Setup
- “How do I initialize a bump allocator for dynamic memory?”
- “What is a trap handler, and how do I install one in RISC-V?”
- “What’s the simplest way to implement a scheduler?”
- “How do I mount a simple in-memory filesystem?”

---

## 🚨 4. Traps & Syscalls

### Phase 7: Entering the Trap World
- “How do I write a trap handler in RISC-V assembly?”
- “How do I save and restore registers correctly in the trap handler?”
- “What does mcause mean in RISC-V?”
- “How might I detect an ecall from user mode?”

### Phase 8: Defining Syscalls
- “How would I implement a syscall interface on RISC-V?”
- “How might I add a syscall for exiting user programs?”

---

## 🧵 5. User Programs

### Phase 9: Writing User Assembly
- “How do I write a simple assembly program that runs in user mode?”
- “How do I embed user programs into an OS image?”
- “How do I compile assembly programs into flat binaries?”

### Phase 10: Interacting with the Kernel
- “How do user programs call syscalls using `ecall`?”
- “How does the kernel load and execute a user program?”

---

## 📦 6. Filesystem

### Phase 11: Directory & File Model
- “How do I implement a simple memory-based FAT-like filesystem?”
- “What’s the minimum structure for a directory and file?”
- “How do I recursively create directories?”

### Phase 12: Shell Commands
- “How do I implement `ls`, `cd`, `cat`, `rm`, and other related commands?”
- “How do I store file contents in an in-memory OS?”
- “How do I run user programs via a shell command like `run <file>`?”

---

## ⚙️ 7. Scheduler & Multitasking

### Phase 13: First Steps
- “What is cooperative multitasking, and how does it differ from preemptive?”
- “How do I design a process table in C?”
- “What does `RUNNING`, `READY`, `ZOMBIE` mean?”

### Phase 14: Context Switching
- “How do I save all registers during an `exit` in RISC-V?”
- “How does the scheduler switch between processes?”
