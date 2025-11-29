# Copyright (c) 2025, Rye Stahle-Smith
# December 2nd, 2025 - Makefile
# Description: Build system for compiling, linking, embedding, and running the RISC-V OS and its user programs under QEMU.

AS       = riscv64-unknown-elf-as
CC       = riscv64-unknown-elf-g++
LD       = riscv64-unknown-elf-ld
OBJCOPY  = riscv64-unknown-elf-objcopy

CFLAGS   = -ffreestanding -nostdlib -march=rv64imaczicsr -mabi=lp64 -O2 -mcmodel=medany
ASFLAGS  = -march=rv64imaczicsr -mabi=lp64

OBJS     = boot.o kernel.o trap.o trap_S.o shell.o memory.o scheduler.o fat.o embedded_user_programs.o
KERNEL   = kernel.elf
LDSCRIPT = linker.ld

PROGRAMS_DIR = user_programs
USER_SOURCES = $(wildcard $(PROGRAMS_DIR)/*.S)
USER_BINS    = $(USER_SOURCES:.S=.bin)

# Default target
all: $(PROGRAMS_DIR) $(USER_BINS) embedded_user_programs.c $(KERNEL)

# User Program Pipeline: .S → .o → .elf → .bin
$(PROGRAMS_DIR)/%.o: $(PROGRAMS_DIR)/%.S
	$(AS) $(ASFLAGS) $< -o $@

$(PROGRAMS_DIR)/%.elf: $(PROGRAMS_DIR)/%.o
	$(LD) -T user_program.ld $< -o $@

$(PROGRAMS_DIR)/%.bin: $(PROGRAMS_DIR)/%.elf
	$(OBJCOPY) -O binary $< $@

# Kernel Objects
trap.o: trap.cpp
	$(CC) $(CFLAGS) -c $< -o $@

trap_S.o: trap.S
	$(CC) $(ASFLAGS) -c $< -o $@

boot.o: boot.S
	$(CC) $(ASFLAGS) -c $< -o $@

kernel.o: kernel.cpp
	$(CC) $(CFLAGS) -c $< -o $@

shell.o: shell.cpp shell.h
	$(CC) $(CFLAGS) -c $< -o $@

memory.o: memory.cpp memory.h
	$(CC) $(CFLAGS) -c $< -o $@
	
scheduler.o: scheduler.cpp scheduler.h
	$(CC) $(CFLAGS) -c $< -o $@

fat.o: fat.cpp fat.h
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL): $(OBJS) $(LDSCRIPT)
	$(LD) -T $(LDSCRIPT) $(OBJS) -o $(KERNEL)

# Auto-embed user programs into C arrays
embedded_user_programs.c: $(USER_BINS)
	@echo "Generating embedded_user_programs.c..."
	@echo "#include <stdint.h>"                               > embedded_user_programs.c
	@echo "#include \"embedded_user_programs.h\""            >> embedded_user_programs.c
	@echo ""                                                 >> embedded_user_programs.c
	@rm -f embedded_user_programs.tmp
	@touch embedded_user_programs.tmp

	@i=0; \
	for f in $(USER_SOURCES); do \
		name=$$(basename $$f .S); \
		var=$$(echo $$name | sed 's/[^A-Za-z0-9_]/_/g'); \
		binfile=$(PROGRAMS_DIR)/$$name.bin; \
		\
		echo "  embedding: $$name"; \
		echo "// Assembly source for $$name"              >> embedded_user_programs.c; \
		echo "const char source_$$var[] = "              >> embedded_user_programs.c; \
		sed 's/\\/\\\\/g; s/"/\\"/g; s/^/"/; s/$$/\\n"/' $$f >> embedded_user_programs.c; \
		echo ";"                                          >> embedded_user_programs.c; \
		echo ""                                           >> embedded_user_programs.c; \
		\
		echo "// Binary for $$name"                       >> embedded_user_programs.c; \
		echo "const uint8_t binary_$$var[] = {"           >> embedded_user_programs.c; \
		xxd -i < $$binfile                                >> embedded_user_programs.c; \
		echo "};"                                         >> embedded_user_programs.c; \
		echo ""                                           >> embedded_user_programs.c; \
		\
		echo "{ \"$$name\", binary_$$var, sizeof(binary_$$var)," \
			"source_$$var, sizeof(source_$$var) },"       >> embedded_user_programs.tmp; \
		i=$$((i+1)); \
	done; \
	echo "const EmbeddedFile embedded_files[] = {"         >> embedded_user_programs.c; \
	cat embedded_user_programs.tmp                         >> embedded_user_programs.c; \
	echo "};"                                              >> embedded_user_programs.c; \
	echo "const unsigned int embedded_file_count = $$i;"   >> embedded_user_programs.c; \
	rm -f embedded_user_programs.tmp

embedded_user_programs.o: embedded_user_programs.c embedded_user_programs.h
	$(CC) $(CFLAGS) -c embedded_user_programs.c -o embedded_user_programs.o

# Cleaning
clean:
	rm -f $(OBJS) $(KERNEL) embedded_user_programs.c
	rm -f $(PROGRAMS_DIR)/*.o $(PROGRAMS_DIR)/*.elf $(PROGRAMS_DIR)/*.bin

deep_clean: clean
	rm -rf $(PROGRAMS_DIR)

# Run in QEMU
run: $(KERNEL)
	qemu-system-riscv64 \
		-machine virt \
		-m 128M \
		-nographic \
		-bios none \
		-kernel $(KERNEL) \
		-serial mon:stdio \
		# -d guest_errors,int,mmu,cpu
