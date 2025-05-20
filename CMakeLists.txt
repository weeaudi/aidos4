cmake_minimum_required(VERSION 3.15) # A reasonable minimum version

project(MyOS LANGUAGES C CXX ASM_NASM)

set(CMAKE_ASM_NASM_SOURCE_FILE_EXTENSIONS asm)
enable_language(ASM_NASM)

# set(CMAKE_ASM_NASM_OBJECT_FORMAT bin) # Default to raw binary if not specified otherwise by target

# Setup for specific compilers
# if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
#     add_compile_options(-Wall -Wextra -Wpedantic)
# elseif(CMAKE_C_COMPILER_ID MATCHES "MSVC")
#     add_compile_options(/W4)
# endif()


message(STATUS "Building Project: ${PROJECT_NAME}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER_ID} - ${CMAKE_C_COMPILER}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER_ID} - ${CMAKE_CXX_COMPILER}")
message(STATUS "Assembler: NASM - ${CMAKE_ASM_NASM_COMPILER}")

add_subdirectory(src)

# Set a common output directory for all binaries if desired.
# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)