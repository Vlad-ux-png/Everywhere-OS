;
; Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.
;
; Module Name:
;
;     entry.asm
;
; Abstract:
;
;     Minimal Multiboot v1 entry point for the MM regression test kernel.
;     Differs from the main entry.asm in three ways:
;
;       1. Flags word is 0x00000001 (page-align modules only).  No
;          framebuffer mode is requested so QEMU -nographic is safe.
;
;       2. The kernel entry function is testMain, not kernelMain.
;
;       3. The stack is placed at MmTestStackTop (defined in .bss below).
;          A 16 KB region is reserved -- generous for a test kernel that
;          does no recursive call chains beyond the pool allocator depth.
;
; Author:
;
;     Noah Juopperi <nipfswd@gmail.com>
;
; Environment:
;
;     Protected mode, ring 0, 32-bit flat model.
;     Entered by a Multiboot v1 compliant loader (QEMU -kernel).
;

[BITS 32]

; Multiboot v1 header
;
; Placed in its own section so the linker script can guarantee it falls
; whitin the first 8 KB of the binary, as required by the specification.
;
; Flags:
;   Bit 0 - align all boot modules to page (4 KB) boundaries.
;   Bit 2 is NOT sSET -- no video mode is requested.

section .multiboot
align 4
    dd  0x1BADB002                          ; Multiboot magic
    dd  0x00000001                          ; flags
    dd  -(0x1BADB002 + 0x00000001)          ; checksum


; Global Descriptor Table
;
; Flat 32-bit code and data segments covering the full 4 GB address space.
; Identical layout to the main kernel GDT.  Selector assignments:
;
;   0x00 - null descriptor (required by the architecture)
;   0x08 - code segment, DPL 0, execute/read, 4 KB granularity
;   0x10 - data segment, DPL 0, read/write, 4 KB granularity

section .data
align 16

MmTestGdtStart:

    ; Null descriptor
    dq 0

    ; Code segment: base 0, limit 4 GB, 32-bit, ring 0
    dw  0xFFFF          ; limit low 16 bits
    dw  0x0000          ; base  low 16 bits
    db  0x00            ; base  middle 8 bits
    db  10011010b       ; access: present I ring-0 | code | exec+read, hehe i love using these "|" things
    db  11001111b       ; flags: 4 KB granularity | 32-bit | limit high nibble
    db  0x00            ; base  high 8 bits

    ; Data segment: base 0, limit 4 GB, 32-bit, ring 0
    dw  0xFFFF
    dw  0x0000
    db  0x00
    db  10010010b       ; access: present | ring-0 | data | read+write
    db  11001111b
    db  0x00

MmTestGdtEnd:

MmTestGdtPtr:
    dw  MmTestGdtEnd - MmTestGdtStart - 1
    dd  MmTestGdtStart


; Stack
;
; 16 KB reserved in .bss.  The test kernel makes no deep call chains and
; holds no large local buffers; 16 KB is conservative overhead.

section .bss
align 16
    resb 16384
MmTestStackTop:


; -------------------------------------------------------------------------
; Kernel entry
; -------------------------------------------------------------------------

section .text
global start

extern __bss_start
extern __end

extern testMain

start:
    mov edi, __bss_start
    mov ecx, __end
    sub ecx, edi
    xor al, al
    rep stosb

    ; Load the GDT defined above and reload all segment registers so they
    ; reference the flat descriptors.
    lgdt    [MmTestGdtPtr]
    mov     ax, 0x10
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax
    jmp     0x08:.flush_cs

.flush_cs:
    ; Establish the test stack and call testMain with the Multiboot
    ; information pointer that the bootloader placed in EBX.
    mov     esp, MmTestStackTop
    push    ebx
    call    testMain

    ; testMain should never return (it halts after printing the summary).
    ; If it does, spin here.
.hang:
    cli
    hlt
    jmp     .hang
