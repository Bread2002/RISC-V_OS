.section .text
.globl _start

_start:
    # UART base address
    li      t0, 0x10000000
    
    # Counter: start at 0
    li      t1, 0
    
count_loop:
    # Check if we've reached 10
    li      t2, 10
    bge     t1, t2, done
    
    # Convert counter to ASCII ('0' + counter)
    addi    t3, t1, 0x30
    
    # Write digit to UART
    sb      t3, 0(t0)
    
    # Write newline
    li      t3, 0x0A
    sb      t3, 0(t0)
    
    # Simple delay loop
    li      t4, 1000000
delay:
    addi    t4, t4, -1
    bnez    t4, delay
    
    # Increment counter
    addi    t1, t1, 1
    j       count_loop
    
done:
    li a7, 93      # SYSCALL_EXIT
    ecall
    