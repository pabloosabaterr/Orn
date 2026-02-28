.text
.globl _start
_start:
    call main
    movq $0,  %rdi
    movq $60, %rax
    syscall