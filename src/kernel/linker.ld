ENTRY(start)
OUTPUT_FORMAT("elf64-x86-64")
phys = 0xFFFF800000000000;

SECTIONS
{
    . = phys;

    __start = .;

    .text                       : AT (0x0) { __text_start = .;       *(.text)        }
    .data                       : { __data_start = .;       *(.data)        }
    .rodata                     : { __rodata_start = .;     *(.rodata)      }
    .bss                        : { __bss_start = .;        *(.bss)         }

    .stack ALIGN(4096) : {
        __stack_bottom = .;
        . = . + 4K;
        __stack_top = .;
    }

    .heap ALIGN(8) : {
        __heap_start = .;
        . = . + 16M;      /* Reserve 16 MiB */
        __heap_end = .;
    }

    __end = .;
}