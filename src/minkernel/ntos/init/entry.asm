; Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.
[BITS 32]

section .multiboot
align 4
    dd 0x1BADB002              ; magic
    dd 0x00000004              ; flags (bit 2 = request video mode)
    dd -(0x1BADB002 + 0x4)     ; checksum
    dd 0, 0, 0, 0, 0          ; address fields (unused without bit 16)
    dd 0                       ; mode_type: 0 = linear/graphics
    dd 640                     ; width
    dd 480                     ; height
    dd 8                       ; depth (8 bpp = 256 colors)

section .data
align 16
gdt_start:
    ; Null descriptor
    dq 0
    ; Code segment: 0x08 - base 0, limit 4GB, 32-bit, ring 0
    dw 0xFFFF       ; limit low
    dw 0x0000       ; base low
    db 0x00         ; base mid
    db 10011010b    ; access: present, ring 0, code, exec/read
    db 11001111b    ; flags: 4KB granularity, 32-bit + limit high
    db 0x00         ; base high
    ; Data segment: 0x10 - base 0, limit 4GB, 32-bit, ring 0
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b    ; access: present, ring 0, data, read/write
    db 11001111b
    db 0x00
gdt_end:

gdt_ptr:
    dw gdt_end - gdt_start - 1
    dd gdt_start

section .text
global start
extern KiStartupSystem

start:
    ; Load our own GDT so segment selectors are known
    lgdt [gdt_ptr]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush_cs

.flush_cs:
    mov esp, 0x90000
    push ebx                   ; pass multiboot info pointer to KiStartupSystem
    call KiStartupSystem

.hang:
    cli
    hlt
    jmp .hang