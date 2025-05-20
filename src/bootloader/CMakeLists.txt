project(Bootloader LANGUAGES ASM)

message(STATUS "Configuring Bootloader Subproject")

add_subdirectory(stage1)
add_subdirectory(stage2)