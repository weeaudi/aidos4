bits 16                                             ; We are in 16-bit real mode
global start
global stage_2_info
global print_char

extern __stage2_info_start

section .fsjump
    jmp short start
    nop

;
;   This is just here for placeholders.
;   Example for a 1.44MB floppy
;
section .fsheaders
    bdb_oem:                    db 'MSWIN4.1'       ; OEM Identifier (8 bytes)
    bdb_bytes_per_sector:       dw 512              ; Bytes per sector (2 bytes)
    bdb_sectors_per_cluster:    db 1                ; Sectors per cluster (1 byte)
    bdb_reserved_sectors:       dw 19               ; Reserved sectors (2 bytes)
    bdb_fat_count:              db 2                ; Number of FATs (1 byte)
    bdb_dir_entries_count:      dw 0E0h             ; Directory entries count (2 bytes)
    bdb_total_sectors:          dw 2880             ; Total sectors (2 bytes) (1.44MB floppy)
    bdb_media_descriptor:       db 0F0h             ; Media descriptor (1 byte, F0h = 3.5" floppy)
    bdb_sectors_per_fat:        dw 9                ; Sectors per FAT (2 bytes)
    bdb_sectors_per_track:      dw 18               ; Sectors per track (2 bytes)
    bdb_heads:                  dw 2                ; Number of heads (2 bytes)
    bdb_hidden_sectors:         dd 0                ; Hidden sectors (4 bytes)
    bdb_large_sector_count:     dd 0                ; Large sector count (4 bytes)

    ebr_drive_number:           db 0                ; Drive number (0x00 = floppy, 0x80 = hard drive)
    ebr_reserved:               db 0                ; Reserved (1 byte)
    ebr_signature:              db 29h              ; Extended boot signature (1 byte)
    ebr_volume_id:              db 'AIDC'           ; Volume ID (4 bytes, serial number not significant)
    ebr_volume_label:           db 'AID OS V4  '    ; Volume label (11 bytes, padded with spaces)
    ebr_system_id:              db 'FAT12   '       ; File system type (8 bytes, padded with spaces)

section .text
start:
    mov ax, 0                                       ; Set up data segment
    mov ds, ax
    mov es, ax

    mov sp, 0x7C00                                  ; Setup stack to where we are located.

    mov [boot_drive], dl                            ; move boot drive to memory

    mov si, init_msg                                ; Pointer to message

    call print_char

    call disk_reset

    call check_disk_extended

    call disk_get_geometry

    ; read the second sector which should contain important info about stage 2

    mov ax, 0
    mov es, ax

    mov eax, 1
    mov cl, 1
    mov bx, __stage2_info_start

    call disk_read

    mov ax, 0
    mov es, ax

    mov eax, [stage_2_info.location]
    mov cl,  [stage_2_info.size]
    mov bx,  [stage_2_info.load]

    call disk_read

    mov dx, [boot_drive]

    mov eax, [stage_2_info.start]
    jmp eax

    jmp halt
    
halt:
    cli                                             ; Clear interrupts
    hlt                                             ; Halt the processor

;
; Param
; - si[in] - Address to null terminated string 
;
print_char:
    lodsb                                           ; Load byte from [SI] into AL, and increment SI
    or al, al                                       ; Check if AL is zero (end of string)
    jz .done                                        ; If zero, return
    mov ah, 0x0E                                    ; BIOS teletype output function
    mov bh, 0x00                                    ; Page number
    mov bl, 0x07                                    ; White text on black background
    int 0x10                                        ; Call BIOS video service
    jmp print_char                                  ; Repeat for next character
.done:
    ret

disk_reset:
    push ax
    push dx

    xor ax, ax                                      ; ax = 0; disk reset on int 13h

    mov dl, [boot_drive]                            ; dl = boot drive; drive to be reset

    int 13h

    pop dx
    pop ax

    ret

disk_get_geometry:
    push ax
    push bx
    push cx
    push dx

    mov ah, 08h
    mov dl, [boot_drive]
    int 13h

    jc .fail  ; If BIOS call fails, skip (leave defaults)

    ; CX: bits 6-0   = sector number (1-63)
    ;     bits 13-8  = cylinder low bits
    ;     bits 7     = high bit of cylinder high bits
    ; DX: high byte  = head count (0-based)

    and cx, 0x3F        ; Sector count is in bits 0–5 of CX
    mov [bdb_sectors_per_track], cx

    mov dx, dx
    shr dx, 8
    inc dx              ; head count is 0-based, so add 1
    mov [bdb_heads], dx

.fail:
    pop dx
    pop cx
    pop bx
    pop ax
    ret

check_disk_extended:
    push ax
    push bx
    push cx
    push dx

    stc
    mov ah, 41h
    mov bx, 55AAh
    int 13h

    jc .no_disk_extended
    cmp bx, 0xAA55
    jne .no_disk_extended

    ; Extended disk functions are present.
    mov byte [disk_extended_present], 1
    jmp .after_disk_check

.no_disk_extended:
    mov byte [disk_extended_present], 0
    

.after_disk_check:
    pop dx
    pop cx
    pop bx
    pop ax
    ret
;
; Param
; - ax - contains the LBA.
; Output
; - cx - holds the sector number (low 6 bits) and the high bits of cylinder,
; - ch - holds the lower 8 bits of the cylinder,
; - dh - contains the head number.
;-------------------------------------------------------------------------
lba_to_chs:
    push ax
    push bx
    push si

    ; Save LBA
    mov bx, ax             ; BX = LBA

    ; Load SPT and HPC
    mov si, [bdb_sectors_per_track]
    mov cx, si             ; CX = SPT

    mov si, [bdb_heads]
    mov dx, si             ; DX = HPC

    ; -----------------------
    ; Cylinder = LBA / (HPC * SPT)
    ; -----------------------
    mov ax, dx             ; AX = HPC
    mul cx                 ; DX:AX = HPC * SPT
    ; Assume DX = 0 → result in AX
    xchg ax, si            ; SI = HPC * SPT

    mov ax, bx             ; AX = LBA
    xor dx, dx
    div si                 ; AX = Cylinder
    mov si, ax             ; Save Cylinder in SI

    ; -----------------------
    ; Head = (LBA / SPT) % HPC
    ; -----------------------
    mov ax, bx             ; AX = LBA
    xor dx, dx
    div cx                 ; AX = LBA / SPT, DX = LBA % SPT
    ;xchg ax, dx            ; AX = remainder, DX = quotient
    xor dx, dx
    div word [bdb_heads]   ; AX = quotient, DX = remainder = head
    mov dh, dl             ; DH = head
    push dx

    ; -----------------------
    ; Sector = (LBA % SPT) + 1
    ; -----------------------
    mov ax, bx             ; AX = original LBA
    xor dx, dx
    div cx                 ; DX = LBA % SPT
    mov cl, dl             ; CL = LBA % SPT
    inc cl                 ; Sector = (LBA % SPT) + 1
    and cl, 0x3F           ; Keep only bits 0–5

    pop dx

    ; -----------------------
    ; Encode Cylinder
    ; -----------------------
    mov ax, si             ; AX = Cylinder
    mov ch, al             ; CH = low 8 bits
    shr ax, 8              ; AL = high 2 bits
    and al, 0x03
    shl al, 6              ; Move to bits 6–7
    or cl, al              ; Combine into CL

    pop si
    pop bx
    pop ax
    ret

;
; Param
; - eax - LBA address of the sector(s) to read.
; - cl - Number of sectors to read (up to 128).
; - dl - Drive number.
; - es:bx - Destination memory address for the data.
;
disk_read:
    pusha
    push es

    cmp byte [disk_extended_present], 1
    jne .no_disk_extensions

    mov byte [extension_dap.size], 10h
    mov [extension_dap.address], eax
    mov [extension_dap.segment], es
    mov [extension_dap.offset], bx
    mov [extension_dap.count], cl

    mov ah, 0x42
    mov si, extension_dap
    mov di, 3

.ext_retry:
    pusha                                           ; Save all registers (BIOS call may modify them)
    stc                                             ; Set carry flag (some BIOSes require this)
    int 13h                                         ; Extended disk read
    popa
    jnc .done                                       ; Jump if successful (carry flag clear)
    call disk_reset
    dec di
    cmp di, 0
    jne .ext_retry
    jmp floppy_error

.no_disk_extensions:
    mov esi, eax                                    ; save lba to esi
    xor ch, ch
    mov di, cx                                      ; number of sectors to di

.outer_loop:
    mov eax, esi
    call lba_to_chs                                 ; convert each lba to chs format
    mov al, 1                                       ; read one at a time

    dec di
    push di
    mov di, 3                                       ; retry count
    mov ah, 02h

.inner_loop:
    pusha
    mov dl, [boot_drive]
    stc
    int 13h
    jnc .inner_done
    popa
    dec di
    cmp di, 0
    jne .inner_loop
    jmp floppy_error

.inner_done:
    popa

    pop di
    cmp di, 0
    je .done

    inc esi

    mov ax, es
    add ax, 32
    mov es, ax
    jmp .outer_loop

.done:
    pop es
    popa
    ret

floppy_error:
    mov si, flpy_err
    call print_char
    cli
    hlt

section .data
disk_extended_present: db 0                         ; default to false

section .rodata
init_msg: db "Bootloader Stage 1 Loaded!", 0xA, 0xD, 0          ; Initial message
flpy_err: db "ERR: floppy", 0xA, 0xD, 0                         ; Floppy error

section .bios_footer

section .stage2_info

stage_2_info:
    .size:      db "A"                            ; size in sectors to read
    .location:  dq "IDCRAFTO"                     ; lba of stage 2
    .load:      dw "S "                           ; address to load stage 2 into        
    .start:     dw "v4"                           ; address to jump to

section .bss
boot_drive: resb 1

extension_dap:
    .size:      resb 1
    .reserved:  resb 1
    .count:     resb 2
    .offset:    resb 2
    .segment:   resb 2
    .address:   resb 8

buffer: resb 512