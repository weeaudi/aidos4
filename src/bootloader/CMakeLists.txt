project(Bootloader LANGUAGES ASM_NASM)

message(STATUS "Configuring Bootloader Subproject")

add_subdirectory(stage1)
add_subdirectory(stage2)