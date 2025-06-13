# check_x86_64_elf_toolchain.cmake

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

1. Prebuilt Toolchain: (Untested)
   - Obtain a trusted x86_64-elf binutils/GCC bundle.
   - Unpack and add its 'bin' directory to your PATH.
   - Make sure to define X86_64_ELF_GCC and X86_64_ELF_LD
   - Or to rename them to x86_64-elf-gcc and x86_64-elf-ld

2. WSL Environment (Windows Subsystem for Linux):
   - Install WSL and set up a Ubuntu (or Debian) distro.
   - In the WSL shell, install prerequisites:
       sudo apt update
       sudo apt install build-essential gcc
   - Reclone the repository and start the build steps from the start

3. MSYS2 Manual Build: (Untested)
   - In an MSYS2 shell, install build essentials:
       pacman -Syu
       pacman -S base-devel
   - Download binutils and GCC sources.
   - Build and install binutils:
       ./configure --target=x86_64-elf --prefix=/mingw64 --disable-nls
       make && make install
   - Build and install GCC (no headers/libstdc++):
       ./configure --target=x86_64-elf --prefix=/mingw64 --disable-nls --disable-libstdcxx --without-headers
       make all-gcc && make install-gcc
   - Make sure to add C:\msys2\mingw64\bin to the path

Make sure 'x86_64-elf-gcc' is in your PATH before running CMake.
Or define X86_64_ELF_GCC and X86_64_ELF_LD to their respective paths
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

This is required to link ELF binaries and is normally part of the x86_64-elf binutils.

Make sure 'x86_64-elf-ld' is in your PATH before running CMake.
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

    message(STATUS "Configured cross-compilation for x86_64-elf")
endif()
