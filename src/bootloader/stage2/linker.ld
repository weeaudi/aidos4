ENTRY(start)
OUTPUT_FORMAT("elf64-x86-64")
phys = 0xA000;

SECTIONS
{
    . = phys;

    .text                       : { __text_start = .;       *(.text)        }
    .data                       : { __data_start = .;       *(.data)        }
    .rodata                     : { __rodata_start = .;     *(.rodata)      }
    .bss                        : { __bss_start = .;        *(.bss)         }

    .heap ALIGN(8) : {
        __heap_start = .;
        . = . + 64K;      /* Reserve 32 KiB */
        __heap_end = .;
    }

    __end = .;
}