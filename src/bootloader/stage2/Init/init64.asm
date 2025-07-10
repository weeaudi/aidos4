bits 32

global start32_64

extern _init
extern stage2_main

section .text

start32_64:

    ; 1 - check for cpuid
    call check_cpuid
    cmp eax, 0
    jne .next

    mov si, cpuid_err
    call error

.next:

    ; 2 - check long mode
    call check_long
    ; check long will auto send an error

    ; 3 - enable paging
    call setup_page_tables
    call enable_paging_and_longmode

    ; 6 - Load 64 bit GDT
    lgdt [gdt64_descriptor]

    ; 7 - jump to 64 bit code
    jmp 08h:long_start

    cli
    hlt

check_cpuid:
    pushfd                               ;Save EFLAGS
    pushfd                               ;Store EFLAGS
    xor dword [esp],0x00200000           ;Invert the ID bit in stored EFLAGS
    popfd                                ;Load stored EFLAGS (with ID bit inverted)
    pushfd                               ;Store EFLAGS again (ID bit may or may not be inverted)
    pop eax                              ;eax = modified EFLAGS (ID bit may or may not be inverted)
    xor eax,[esp]                        ;eax = whichever bits were changed
    popfd                                ;Restore original EFLAGS
    and eax,0x00200000                   ;eax = zero if ID bit can't be changed, else non-zero
    ret

check_long:
    mov     eax, 0x80000000
    cpuid
    cmp     eax, 0x80000001
    jb      .no_longmode    ; Extended function not supported

    mov     eax, 0x80000001
    cpuid
    test    edx, 1 << 29    ; Check bit 29 of EDX (Long Mode)
    jz      .no_longmode

.longmode_supported:
    jmp     .done

.no_longmode:
    mov si, longmode_error
    call error

.done:
    ret

setup_page_tables:
    [bits 32]
    push eax
    push ecx

    ; Set L4: mark the level 3 table as present and writable.
    mov eax, page_table_l3
    or eax, 11b                 ; Flags: present, writable.
    mov [page_table_l4], eax

    ; Set L3: mark the level 2 table as present and writable.
    mov eax, page_table_l2
    or eax, 11b                 ; Flags: present, writable.
    mov [page_table_l3], eax

    mov ecx, 0                  ; Initialize counter for L2 entries.

.loop:
    ; Calculate physical address for the 2 MiB page: 0x200000 * ECX.
    mov eax, 0x200000
    mul ecx                     ; EAX = 0x200000 * ECX.
    or eax, 10000011b           ; Set flags: present, writable, huge page.
    mov [page_table_l2 + ecx * 8], eax

    inc ecx
    cmp ecx, 512                ; Map 512 entries (512 * 2MiB = 1 GiB).
    jne .loop

    pop ecx
    pop eax
    ret

enable_paging_and_longmode:
    [bits 32]
    ; Load L4 page table address into CR3.
    mov eax, page_table_l4
    mov cr3, eax

    ; Enable PAE in CR4.
    mov eax, cr4
    or eax, 1 << 5 | 1 << 7     ; Enable PAE and related flags.
    mov cr4, eax

.enable_long_mode:
    mov ecx, 0xC0000080         ; IA32_EFER MSR.
    rdmsr
    or eax, 1 << 8              ; Set the Long Mode Enable (LME) bit.
    wrmsr

.enable_paging:
    mov eax, cr0
    or eax, 1 << 31 | 1 << 0     ; Enable paging (PG) and protection (PE).
    mov cr0, eax

    ret

error:
    mov edi, 0xB8000

.next_char:
    lodsb                 ; load byte at [SI] into AL and increment SI
    test al, al           ; check for null terminator
    jz .done              ; if zero, end of string
    mov ah, 0x07          ; attribute byte (light gray on black)
    stosw                 ; write AX to [ES:DI] and increment DI by 2
    jmp .next_char

.done:
    hlt
    jmp .done

[bits 64]
long_start:

    cli

    call _init

    jmp stage2_main

    cli
    hlt

[bits 32]

section .data
align 8
gdt64_start:
gdt64_null:                       ; Null descriptor
    dq 0

gdt64_code:                       ; 64-bit code segment descriptor
    dw 0xFFFF                     ; Limit low (ignored in 64-bit but set for compatibility)
    dw 0x0000                     ; Base low
    db 0x00                       ; Base mid
    db 10011010b                  ; Access byte: present, ring 0, code, executable, readable
    db 00100000b                  ; Flags: granularity=0, 64-bit (L=1), D=0
    db 0x00                       ; Base high

gdt64_data:                       ; 64-bit data segment descriptor
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b                  ; Access byte: present, ring 0, data segment, writable
    db 11001111b                  ; Flags: granularity=4K, D=1 (32-bit segment)
    db 0x00

gdt64_end:

gdt64_descriptor:
    dw gdt64_end - gdt64_start - 1
    dd gdt64_start

section .rodata
cpuid_err: db "ERROR: CPUID NOT SUPPORTED", 0
longmode_error: db "ERROR: LONG MODE NOT SUPPORTED. PLEASE USE A 64 BIT SYSTEM!", 0

section .bss
align 4096
page_table_l4:
    resb 4096
page_table_l3:
    resb 4096
page_table_l2:
    resb 4096