# Simple semaphore test with output - create, signal, and wait
.section .text
.global _start

# UART write function (address 0x10000000)
.macro uart_putchar char
    li t5, 0x10000000
    li t6, \char
    sd t6, 0(t5)
.endm

_start:
    # Initialize stack
    li sp, 0x80020000

    # Print: "Creating semaphore..."
    uart_putchar 'C'
    uart_putchar 'r'
    uart_putchar 'e'
    uart_putchar 'a'
    uart_putchar 't'
    uart_putchar 'i'
    uart_putchar 'n'
    uart_putchar 'g'
    uart_putchar ' '
    uart_putchar 's'
    uart_putchar 'e'
    uart_putchar 'm'
    uart_putchar '.'
    uart_putchar '.'
    uart_putchar '.'
    uart_putchar '\n'

    # Create a semaphore with initial value 0
    li a7, 150          # SYSCALL_SEM_CREATE
    li a0, 0            # initial value = 0
    ecall
    
    # a0 now contains semaphore ID
    mv s0, a0           # s0 = semaphore ID
    
    # Print: "Semaphore created with ID: "
    uart_putchar 'S'
    uart_putchar 'e'
    uart_putchar 'm'
    uart_putchar 'I'
    uart_putchar 'D'
    uart_putchar '='
    
    # Print the semaphore ID (just as a single digit for now)
    addi t0, a0, 48     # Convert to ASCII (assuming ID < 10)
    li t5, 0x10000000
    sd t0, 0(t5)
    uart_putchar '\n'

    # Signal the semaphore once
    uart_putchar 'S'
    uart_putchar 'i'
    uart_putchar 'g'
    uart_putchar 'n'
    uart_putchar 'a'
    uart_putchar 'l'
    uart_putchar 'i'
    uart_putchar 'n'
    uart_putchar 'g'
    uart_putchar '.'
    uart_putchar '.'
    uart_putchar '.'
    uart_putchar '\n'
    
    mv a0, s0           # a0 = semaphore ID
    li a7, 152          # SYSCALL_SEM_SIGNAL
    ecall
    
    # Print: "Signaled!"
    uart_putchar 'S'
    uart_putchar 'i'
    uart_putchar 'g'
    uart_putchar 'n'
    uart_putchar 'a'
    uart_putchar 'l'
    uart_putchar 'e'
    uart_putchar 'd'
    uart_putchar '!'
    uart_putchar '\n'

    # Now try to wait on the semaphore (should succeed since we just signaled)
    uart_putchar 'W'
    uart_putchar 'a'
    uart_putchar 'i'
    uart_putchar 't'
    uart_putchar 'i'
    uart_putchar 'n'
    uart_putchar 'g'
    uart_putchar '.'
    uart_putchar '.'
    uart_putchar '.'
    uart_putchar '\n'
    
    mv a0, s0           # a0 = semaphore ID
    li a7, 151          # SYSCALL_SEM_WAIT
    ecall
    
    # Print: "Wait returned!"
    uart_putchar 'W'
    uart_putchar 'a'
    uart_putchar 'i'
    uart_putchar 't'
    uart_putchar ' '
    uart_putchar 'd'
    uart_putchar 'o'
    uart_putchar 'n'
    uart_putchar 'e'
    uart_putchar '!'
    uart_putchar '\n'
    
    # Exit
    li a7, 93           # SYSCALL_EXIT
    ecall
    
    jal x0, _start      # Spin forever if exit doesn't work
