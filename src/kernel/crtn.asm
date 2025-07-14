[bits 64]

section .init
    ; gcc will put the content of crtbegin.o here
    pop rbp
    ret

section .fini
    ; gcc will put the content of crtend.o here
    pop rbp
    ret