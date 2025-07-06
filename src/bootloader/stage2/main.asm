bits 16

global start

section .text
start:

    mov sp, 0xA000

    mov si, text
    call print_char

    cli
    hlt

;
; Param
; - si[in] - Address to null terminated string 
;
print_char:
    lodsb                                           ; Load byte from [SI] into AL, and increment SI
    or al, al                                       ; Check if AL is zero (end of string)
    jz .done                                        ; If zero, jump to halt
    mov ah, 0x0E                                    ; BIOS teletype output function
    mov bh, 0x00                                    ; Page number
    mov bl, 0x07                                    ; White text on black background
    int 0x10                                        ; Call BIOS video service
    jmp print_char                                  ; Repeat for next character
.done:
    ret

section .rodata
text: db "HELLO FROM STAGE2 OF THE BOOTLOADER", 0xA, 0xD, 0