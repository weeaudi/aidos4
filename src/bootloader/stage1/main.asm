bits 16         ; We are in 16-bit real mode

org 0x7C00      ; BIOS loads bootloader at this address

start:
    mov ax, 0x07C0  ; Set up data segment
    mov ds, ax
    mov es, ax

    mov si, message ; Pointer to message
print_char:
    lodsb           ; Load byte from [SI] into AL, and increment SI
    or al, al       ; Check if AL is zero (end of string)
    jz halt         ; If zero, jump to halt
    mov ah, 0x0E    ; BIOS teletype output function
    mov bh, 0x00    ; Page number
    mov bl, 0x07    ; White text on black background
    int 0x10        ; Call BIOS video service
    jmp print_char  ; Repeat for next character

halt:
    cli             ; Clear interrupts
    hlt             ; Halt the processor

message:
    db "Bootloader Stage 1 Loaded!", 0 ; Null-terminated string

; Padding and magic number
times 510 - ($-$$) db 0   ; Pad remainder of 510 bytes with 0
dw 0xAA55                 ; Boot signature