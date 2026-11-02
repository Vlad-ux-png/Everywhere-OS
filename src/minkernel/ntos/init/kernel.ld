ENTRY(start)

SECTIONS
{
    . = 0x00100000;

    .multiboot :
    {
        KEEP(*(.multiboot))
    }

    .text :
    {
        *(.text*)
    }

    .rodata :
    {
        *(.rodata*)
    }

    .data :
    {
        *(.data*)
    }

    __bss_start = .;
    .bss :
    {
        *(.bss*)
        *(COMMON)
    }
    __end = .;
}
