    .section .text
    .globl _start

_start:
    # UART base address
    li      t0, 0x10000000

    # Load address of message (in same section as code)
    la      t1, message

print_loop:
    # Load byte from message
    lbu     t2, 0(t1)

    # Check if null terminator
    beqz    t2, done

    # Write to UART
    sb      t2, 0(t0)

    # Move to next character
    addi    t1, t1, 1
    j       print_loop

done:
    li      a7, 1          # syscall: exit
    ecall

    # Keep data in .text so it’s included in the flat binary
message:
    .string "Hello from assembly user program!\n"
