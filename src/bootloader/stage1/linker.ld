ENTRY(start)
OUTPUT_FORMAT("binary")
phys = 0x7C00;

SECTIONS
{
    . = phys;

    .fsjump                     : {                         *(.fsjump)      }
    .fsheaders                  : {                         *(.fsheaders)   }
    .entry                      : { __entry_start = .;      *(.entry)       }
    .text                       : { __text_start = .;       *(.text)        }
    .data                       : { __data_start = .;       *(.data)        }
    .rodata                     : { __rodata_start = .;     *(.rodata)      }
    .bios_footer phys + 0x1FE   : {                         SHORT(0xAA55)   }

    . = phys + 0x200;

    .stage2_info :
    {
      __stage2_info_start = .;
      *(.stage2_info)
      __stage2_info_end = .;
      . += (512 - (__stage2_info_end - __stage2_info_start));
    }

    .bss                        : { __bss_start = .;        *(.bss)         }

    __end = .;
}