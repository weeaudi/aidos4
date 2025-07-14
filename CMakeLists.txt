cmake_minimum_required(VERSION 3.15) # A reasonable minimum version

project(AidOS)

include(ExternalProject)

set(TARGET x86_64-elf)
set(PREFIX_DIR toolchain)

set(BINUTILS_URL https://ftp.gnu.org/gnu/binutils/binutils-2.44.tar.gz)
set(GCC_URL https://ftp.gnu.org/gnu/gcc/gcc-13.2.0/gcc-13.2.0.tar.gz)

ExternalProject_Add(
  binutils
  URL ${BINUTILS_URL}
  URL_MD5 6c02ef5e0de5096f0e9cd6f9d48a4be3
  PREFIX "${CMAKE_SOURCE_DIR}/${PREFIX_DIR}"
  CONFIGURE_COMMAND <SOURCE_DIR>/configure --target=${TARGET} --prefix=${CMAKE_SOURCE_DIR}/${PREFIX_DIR} --with-sysroot= --disable-nls --disable-werror
  BUILD_COMMAND make -j16
  INSTALL_COMMAND make install
  DOWNLOAD_EXTRACT_TIMESTAMP true
  BUILD_ALWAYS OFF
  STAMP_DIR ${CMAKE_SOURCE_DIR}/${PREFIX_DIR}/_deps/binutils/stamp
  EXCLUDE_FROM_ALL 1
)

set(GCC_BUILD_LIBS make -j6\\ all-target-libgcc\\ CFLAGS_FOR_TARGET=\"-mcmodel=large\")

ExternalProject_Add(
  gcc
  DEPENDS binutils
  URL ${GCC_URL}
  URL_MD5 aeb5ac806c34d47e725bdd025f34bac4
  PREFIX "${CMAKE_SOURCE_DIR}/${PREFIX_DIR}"
  CONFIGURE_COMMAND <SOURCE_DIR>/configure
    --target=${TARGET}
    --prefix=${CMAKE_SOURCE_DIR}/${PREFIX_DIR}
    --disable-nls
    --enable-languages=c,c++
    --without-headers
  BUILD_COMMAND make -j6 all-gcc
      && ${GCC_BUILD_LIBS}
  INSTALL_COMMAND make -j6 install-gcc
      && make -j6 install-target-libgcc
  DOWNLOAD_EXTRACT_TIMESTAMP true
  BUILD_ALWAYS OFF
  STAMP_DIR ${CMAKE_SOURCE_DIR}/${PREFIX_DIR}/_deps/gcc/stamp
  EXCLUDE_FROM_ALL 1
)

set(CMAKE_C_COMPILER "${CMAKE_SOURCE_DIR}/${PREFIX_DIR}/bin/${TARGET}-gcc")
set(CMAKE_CXX_COMPILER "${CMAKE_SOURCE_DIR}/${PREFIX_DIR}/bin/${TARGET}-g++")
set(CMAKE_ASM_NASM_FLAGS "-f elf64 -g")
set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -o <OBJECT> <SOURCE>")
set(CMAKE_LINKER "${CMAKE_SOURCE_DIR}/${PREFIX_DIR}/bin/${TARGET}-g++")
set(CMAKE_AR "${CMAKE_SOURCE_DIR}/${PREFIX_DIR}/bin/${TARGET}-ar")
set(CMAKE_NM "${CMAKE_SOURCE_DIR}/${PREFIX_DIR}/bin/${TARGET}-nm")
set(CMAKE_OBJCOPY "${CMAKE_SOURCE_DIR}/${PREFIX_DIR}/bin/${TARGET}-objcopy")
set(CMAKE_OBJDUMP "${CMAKE_SOURCE_DIR}/${PREFIX_DIR}/bin/${TARGET}-objdump")

enable_language(ASM_NASM)
set(CMAKE_ASM_NASM_FLAGS_DEBUG "-g -F dwarf")

message(STATUS "Building Project: ${PROJECT_NAME}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER_ID} - ${CMAKE_C_COMPILER}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER_ID} - ${CMAKE_CXX_COMPILER}")
message(STATUS "Assembler: NASM - ${CMAKE_ASM_NASM_COMPILER}")

add_subdirectory(src)