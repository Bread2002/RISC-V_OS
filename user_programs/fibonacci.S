    .section .text
    .globl _start

_start:
    # UART base address
    li      t0, 0x10000000

    # Fibonacci with only t-registers:
    # t1 = a, t2 = b, t3 = counter
    li      t1, 0          # a = 0
    li      t2, 1          # b = 1
    li      t3, 0          # counter = 0

fib_loop:
    # stop after 10 numbers
    li      t4, 10
    bge     t3, t4, done

    # value to print = t1
    mv      t5, t1         # t5 = value

    # if value < 10: one digit
    li      t4, 10
    blt     t5, t4, one_digit

    # two-digit (max is 34)
    divu    t6, t5, t4     # t6 = tens
    remu    t5, t5, t4     # t5 = ones
    addi    t6, t6, 48     # '0' + tens
    addi    t5, t5, 48     # '0' + ones
    sb      t6, 0(t0)
    sb      t5, 0(t0)
    j       printed

one_digit:
    addi    t5, t5, 48     # '0' + value
    sb      t5, 0(t0)

printed:
    # newline
    li      t4, 10         # '\n'
    sb      t4, 0(t0)

    # next = a + b
    add     t4, t1, t2     # t4 = next

    # a = b ; b = next
    mv      t1, t2
    mv      t2, t4

    # counter++
    addi    t3, t3, 1
    j       fib_loop

done:
    li      a7, 93          # SYSCALL_EXIT
    ecall
