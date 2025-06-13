# check_x86_64_elf_toolchain.cmake

if(WIN32)
    message(STATUS "Detected Windows host")

    # Check for x86_64-elf-gcc
    find_program(X86_64_ELF_GCC x86_64-elf-gcc)
    if(NOT X86_64_ELF_GCC)
        message(FATAL_ERROR "
============================================================
Missing required tool: x86_64-elf-gcc

Please install a cross-compiler toolchain for x86_64-elf.

Recommended methods:

1. MSYS2 (recommended for Windows):
   - Install MSYS2 and open its shell.
   - Run:
       pacman -Syu
       pacman -S mingw-w64-x86_64-gcc

2. Build Your Own:
   - Build binutils and GCC from source with:
       ./configure --target=x86_64-elf --prefix=/your/install/path
       make && make install

Make sure 'x86_64-elf-gcc' is in your system PATH before running CMake.
============================================================
")
    else()
        message(STATUS "Found x86_64-elf-gcc: ${X86_64_ELF_GCC}")
    endif()

    # Check for x86_64-elf-ld
    find_program(X86_64_ELF_LD x86_64-elf-ld)
    if(NOT X86_64_ELF_LD)
        message(FATAL_ERROR "
============================================================
Missing required tool: x86_64-elf-ld

This is required to link ELF binaries.

It is typically included with x86_64-elf-gcc.

Make sure 'x86_64-elf-ld' is in your system PATH before running CMake.
============================================================
")
    else()
        message(STATUS "Found x86_64-elf-ld: ${X86_64_ELF_LD}")
    endif()
endif()

if(X86_64_ELF_GCC AND X86_64_ELF_LD)

    set(CMAKE_C_COMPILER ${X86_64_ELF_GCC})
    set(CMAKE_CXX_COMPILER ${X86_64_ELF_GCC})
    set(CMAKE_LINKER ${X86_64_ELF_LD})

    find_program(X86_64_ELF_AR x86_64-elf-ar)
    find_program(X86_64_ELF_OBJCOPY x86_64-elf-objcopy)

    if(X86_64_ELF_AR)
        set(CMAKE_AR ${X86_64_ELF_AR})
    endif()

    if(X86_64_ELF_OBJCOPY)
        set(CMAKE_OBJCOPY ${X86_64_ELF_OBJCOPY})
    endif()

    message(STATUS "Configured cross-compilation for x86_64-elf")
endif()
