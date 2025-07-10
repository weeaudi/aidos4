[bits 64]

section .init
global _init
_init:
    push rbp
    mov rbp, rsp
    ; gcc will put the content of crtbegin.o here

section .fini
global _fini
_fini:
    push rbp
    mov rbp, rsp
    ; gcc will put the content of crtend.o here