ENTRY(start)
OUTPUT_FORMAT("elf64-x86-64")
phys = 0xFFFF800000000000;

SECTIONS
{
    . = phys;

    .text                       : AT (0x0) { __text_start = .;       *(.text)        }
    .data                       : { __data_start = .;       *(.data)        }
    .rodata                     : { __rodata_start = .;     *(.rodata)      }
    .bss                        : { __bss_start = .;        *(.bss)         }

    __end = .;
}