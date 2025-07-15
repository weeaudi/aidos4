bits 64

global start
global load_cr3

extern _init
extern __stack_top
extern main

section .text

start:

    mov rsp, __stack_top

    push rdi
    push rsi

    call _init

    pop rsi
    pop rdi

    call main


load_cr3:

    push rax

    mov rax, rdi
    mov cr3, rax
    
    pop rax

    ret

