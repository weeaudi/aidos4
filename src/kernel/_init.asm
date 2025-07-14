bits 64

global start

extern _init
extern main

section .text

start:

    push rdi
    push rsi

    call _init

    pop rsi
    pop rdi

    call main
