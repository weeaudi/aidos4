bits 16

global start

extern start16_32

section .text
start:
    mov sp, 0xA000

    mov si, text
    call print_char
    call newline

    mov si, ebda_msg
    call print_char
    call detect_ebda
    mov eax, [boot_info.ebda]
    call print_hex32
    call newline

    mov si, rsdp_msg
    call print_char
    call detect_rsdp
    mov eax, [boot_info.rsdp_addr]
    call print_hex32
    call newline

    mov si, mem_map_msg
    call print_char
    call detect_memory_map
    mov ax, [boot_info.mem_map_count]
    call print_hex16
    mov si, mem_map_msg2
    call print_char
    mov ax, boot_info.mem_map
    call print_hex16
    call newline

    mov cx, [boot_info.mem_map_count]   ; total entries
    mov si, boot_info.mem_map            ; pointer to entries

    ; Load mem_map_count into CX
    mov ax, [boot_info.mem_map_count]
    mov cx, ax            ; loop counter = number of entries

    mov bx, boot_info.mem_map ; segment of mem_map  

.loop_start:
    ; Print "Base: 0x"
    mov si, base_str
    call print_char

    ; Print 64-bit base address (8 bytes)
    mov si, bx            ; offset of entry in mem_map
    call print_hex64      ; DS:SI points to base addr

    add bx, 8             ; move offset to length field

    ; Print " Len: 0x"
    mov si, len_str
    call print_char

    mov si, bx            ; offset points to length field
    call print_hex64      ; print 64-bit length

    add bx, 8             ; move offset to type field

    ; Print " Type: 0x"
    mov si, type_str
    call print_char

    ; type field is 4 bytes (32-bit)
    mov eax, 0
    mov ax, word [ds:bx]      ; load lower 16 bits of type
    mov dx, word [ds:bx+2]    ; load upper 16 bits
    shl edx, 16
    or eax, edx               ; eax = full 32-bit type
    call print_hex32

    add bx, 8                 ; move to next entry (total 24 bytes per entry)

    ; Call newline function here instead of printing newline string
    call newline

    loop .loop_start

    call start16_32


hang:
    hlt
    jmp hang


;
; Param
; - si[in] - Address to null terminated string 
;
print_char:

    push ax
    push bx

.loop:

    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x07
    int 0x10
    jmp .loop
.done:

    pop bx
    pop ax

    ret

newline:

    push ax
    push bx

    mov ah, 0x0E
    mov al, 0x0D
    int 0x10
    mov al, 0x0A
    int 0x10

    pop bx
    pop ax

    ret


;
; Param:
; - ax[in] - 16-bit value to print as hex
;
print_hex16:
    push ax
    push cx
    mov cx, 4
.print_hex16_loop:
    rol ax, 4
    push ax
    and al, 0x0F
    call print_hex_digit
    pop ax
    loop .print_hex16_loop
    pop cx
    pop ax
    ret

;
; Param:
; - eax[in] - 32-bit value to print as 8 hex digits
;
print_hex32:
    push eax
    push ecx

    mov ecx, 8
.print_loop:
    rol eax, 4
    push eax
    and al, 0x0F
    call print_hex_digit
    pop eax
    loop .print_loop

    pop ecx
    pop eax
    ret

; print_hex64
; DS:SI points to 8-byte value (low dword first)
print_hex64:
    push si
    push eax
    push dx

    ; Print high 32 bits
    add si, 4                 ; point to high dword
    mov eax, [si]             

    call print_hex32          ; print high 32 bits

    ; Print low 32 bits
    sub si, 4                 ; point back to low dword
    mov eax, [si]             

    call print_hex32          ; print low 32 bits

    pop dx
    pop eax
    pop si
    ret


;
; Param:
; - al[in] - lower 4 bits are the hex digit
;
print_hex_digit:

    push ax
    push bx

    and al, 0x0F
    cmp al, 10
    jl .digit
    add al, 'A' - 10
    jmp .emit
.digit:
    add al, '0'
.emit:
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x07
    int 0x10

    pop bx
    pop ax

    ret



;
; Output:
; - word [boot_info.ebda] = segment address of EBDA
;
detect_ebda:
    mov ax, 0x40       ; BIOS Data Area segment
    mov es, ax
    mov bx, 0x0E       ; Offset 0x40E contains EBDA segment (in KB)
    xor eax, eax
    mov ax, [es:bx]
    shl eax, 4          ; Convert KB segment to real address
    mov [boot_info.ebda], eax
    ret

;
; Output:
; - dword [boot_info.rsdp_addr] = physical address of RSDP (if found), else 0
;
detect_rsdp:
    pushad
    push ds
    push es

    ;----------------------------------------------------
    ; Step 1: Read EBDA segment from BIOS Data Area (0x40E)
    ;----------------------------------------------------
    mov ax, 0x40
    push ds
    mov ds, ax
    mov bx, 0x0E
    xor eax, eax
    mov ax, [ds:bx]       ; EBDA segment in paragraphs
    pop ds
    test ax, ax
    jz .scan_bios_area    ; If 0, skip to BIOS scan

    shl eax, 4            ; Convert to physical address
    mov esi, eax           ; SI = physical address of EBDA
    call scan_rsdp_area_1kb
    test eax, eax
    jnz .found

.scan_bios_area:
    mov esi, 0xE0000       ; Physical address of BIOS scan start
    call scan_rsdp_area_128kb
    ; eax = 0 if not found

.found:
    mov [boot_info.rsdp_addr], eax

    pop es
    pop ds
    popad
    ret

;----------------------------------------------------
; Scan 1 KB for RSDP (EBDA)
; Input: SI = physical address
; Output: EAX = physical address of RSDP if found, 0 if not
;----------------------------------------------------
scan_rsdp_area_1kb:
    push es
    push di
    push esi
    push cx

    shr esi, 4
    mov ax, si
    mov es, ax
    and si, 0x0F
    mov di, si
    mov cx, 1024 / 16     ; 1 KB / 16-byte steps

.next_1kb:
    push si
    push di
    push cx
    mov cx, 8
    mov si, rsdp_sig
    repe cmpsb
    pop cx
    pop di
    pop si
    je .found_1kb

    add di, 16
    loop .next_1kb

    xor eax, eax
    jmp .done_1kb

.found_1kb:
    mov ax, es
    movzx eax, ax
    shl eax, 4
    movzx edi, di
    add eax, edi

.done_1kb:
    pop cx
    pop esi
    pop di
    pop es
    ret

;----------------------------------------------------
; Scan 0xE0000–0x100000 (128 KB) for RSDP
; Input: SI = physical address (start of scan)
; Output: EAX = physical address of RSDP if found, 0 if not
;----------------------------------------------------
scan_rsdp_area_128kb:
    push es
    push di
    push si
    push cx

    mov eax, esi           ; Starting physical addr: 0xE0000
    mov cx, (0x100000 - 0xE0000) / 16  ; = 8192 entries

    shr esi, 4
    mov ax, si
    mov es, ax
    and si, 0x0F
    mov di, si

.next_128kb:
    push si
    push di
    push cx
    mov cx, 8
    mov si, rsdp_sig
    repe cmpsb
    pop cx
    pop di
    pop si
    je .found_128kb

    push ax
    mov ax, es
    add ax, 1
    mov es, ax
    pop ax

    loop .next_128kb

    xor eax, eax
    jmp .done_128kb

.found_128kb:
    mov ax, es
    movzx eax, ax
    shl eax, 4
    movzx edi, di
    add eax, edi

.done_128kb:
    pop cx
    pop si
    pop di
    pop es
    ret


;
; Output:
; - saves E820 memory entries to boot_info.mem_map
; - updates [boot_info.mem_map_count] with number of entries
;
detect_memory_map:
    pushad
    push es
    push ds

    xor ax, ax
    mov ds, ax                      ; Flat DS to access boot_info

    xor ebx, ebx                    ; Continuation value = 0
    mov edx, 0x534D4150             ; "SMAP"
    mov eax, 0xE820
    mov ecx, 24

    mov si, boot_info.mem_map      ; SI = destination for entries
    mov di, si
    shr si, 4
    mov es, si                     ; ES segment from SI
    and di, 0xF                    ; Offset within segment

    xor bp, bp                     ; Entry count = 0

.next_entry:
    push es
    push di
    mov eax, 0xE820
    mov edx, 0x534D4150
    mov ecx, 24
    int 0x15
    pop di
    pop es

    jc .done
    cmp eax, 0x534D4150
    jne .done

    add di, 24                     ; Move to next entry
    inc bp                         ; Increment entry count

    test ebx, ebx
    jnz .next_entry

.done:
    mov [boot_info.mem_map_count], bp

    pop ds
    pop es
    popad
    ret

section .rodata
text: db "Getting Bios Data...", 0
ebda_msg: db "Getting EBDA data... Address: 0x", 0
rsdp_msg: db "Getting RSDP address... Address: 0x", 0
mem_map_msg: db "Getting Memory Map... Count: ", 0
mem_map_msg2: db " At 0x", 0
base_str: db 'Base: 0x',0
len_str:  db ' Len: 0x',0
type_str: db ' Type: 0x',0

rsdp_sig: db "RSD PTR ", 0


section .bss
boot_info:
    align 16
    .mem_map:        resb 2048
    .mem_map_count:  resd 1
    .rsdp_addr:      resd 1
    .ebda:           resd 1
    .boot_drive:     resb 1
