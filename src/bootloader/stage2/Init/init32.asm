bits 16

global start16_32

extern start32_64

section .text
start16_32:
    ; Start switch to 32 bit mode

    ; 1 - Disable interrupts
    cli

    ; 2 - A20 line
    call check_a20
    cmp ax, 1
    je .next

    ; A20 not enabled by BIOS enable it ourselfs

    in al, 0x92
    test al, 2
    jnz .after
    or al, 2
    and al, 0xFE
    out 0x92, al
.after:

    call check_a20
    cmp ax, 1
    je .next

    mov si, a20_err
    jmp error

.next:

    ; 3 - Load GDT
    lgdt [gdt_descriptor]

    ; 4 - Enable CR0
    mov eax, cr0 
    or al, 1       ; set PE (Protection Enable) bit in CR0 (Control Register 0)
    mov cr0, eax

    ; 5 - Jump to protected mode
    jmp 08h:protected_start

[bits 32]
protected_start:

    mov ax, 0x10        ; data selector
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    jmp start32_64

.hlt:
    cli
    hlt
    jmp .hlt 
[bits 16]

check_a20:
    pushf
    push ds
    push es
    push di
    push si

    cli

    xor ax, ax ; ax = 0
    mov es, ax

    not ax ; ax = 0xFFFF
    mov ds, ax

    mov di, 0x0500
    mov si, 0x0510

    mov al, byte [es:di]
    push ax

    mov al, byte [ds:si]
    push ax

    mov byte [es:di], 0x00
    mov byte [ds:si], 0xFF

    cmp byte [es:di], 0xFF

    pop ax
    mov byte [ds:si], al

    pop ax
    mov byte [es:di], al

    mov ax, 0
    je check_a20__exit

    mov ax, 1

check_a20__exit:
    pop si
    pop di
    pop es
    pop ds
    popf

    ret

;
; Param
; - si[in] - Address to null terminated error message
error:
    push si
    mov si, error_msg
    call print_char
    pop si
    call print_char
.hlt:
    hlt
    jmp .hlt

;
; Param
; - si[in] - Address to null terminated string 
;
print_char:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x07
    int 0x10
    jmp print_char
.done:
    ret



section .data

; GDT - Global Descriptor Table (flat model, 32-bit)
align 4
gdt_start:
gdt_null:                       ; Null descriptor (mandatory)
    dq 0

gdt_code:                       ; Code segment: base=0, limit=4GB, exec/read
    dw 0xFFFF                   ; Limit (bits 0-15)
    dw 0x0000                   ; Base (bits 0-15)
    db 0x00                     ; Base (bits 16-23)
    db 10011010b                ; Access: present, ring 0, code segment, readable
    db 11001111b                ; Flags: granularity=4K, 32-bit, limit (bits 16-19)
    db 0x00                     ; Base (bits 24-31)

gdt_data:                       ; Data segment: base=0, limit=4GB, read/write
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b                ; Access: present, ring 0, data segment, writable
    db 11001111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1 ; Size of GDT - 1
    dd gdt_start               ; Linear address of GDT

section .rodata
error_msg: db "ERROR: ", 0
a20_err: db "A20 NOT ENABLED!!!", 0