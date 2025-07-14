bits 64

global load_cr3

section .text

load_cr3:
    mov rax, rdi
    mov cr3, rax
    ret