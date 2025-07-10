set(BOOTLOADER_STAGE1_ASM_SOURCE main.asm)
set(BOOTLOADER_STAGE1_NAME boot_stage1)
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld)

set_source_files_properties(${BOOTLOADER_STAGE1_ASM_SOURCE} PROPERTIES LANGUAGE ASM_NASM)

add_executable(${BOOTLOADER_STAGE1_NAME} ${BOOTLOADER_STAGE1_ASM_SOURCE})

set_target_properties(${BOOTLOADER_STAGE1_NAME} PROPERTIES
    LINK_FLAGS "-T ${LINKER_SCRIPT} -nostdlib -Wl,-Map=stage1.map"
    LINKER_LANGUAGE C
)

message(STATUS "Configuring Bootloader Stage 1: ${BOOTLOADER_STAGE1_ASM_SOURCE} -> ${BOOTLOADER_STAGE1_NAME}")

if(CMAKE_BUILD_TYPE)
    if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        set(DEBUG_BOOTLOADER_STAGE1_NAME boot_stage1_debug)
        set(DEBUG_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/debug.ld)

        add_executable(${DEBUG_BOOTLOADER_STAGE1_NAME} ${BOOTLOADER_STAGE1_ASM_SOURCE})

        set_target_properties(${DEBUG_BOOTLOADER_STAGE1_NAME} PROPERTIES
            LINK_FLAGS "-T ${DEBUG_LINKER_SCRIPT} -nostdlib -no-pie -Wl,--build-id=none,--no-warn-rwx-segments,-Map=\"stage1_debug.map\""
            LINKER_LANGUAGE C
        )

        message(STATUS "Configuring Debug Bootloader Stage 1: ${BOOTLOADER_STAGE1_ASM_SOURCE} -> ${DEBUG_BOOTLOADER_STAGE1_NAME}")
    else()
        message(STATUS "Non debug build type, skipping debug build")
    endif()
else()
    message(STATUS "No build type specified skipping debug build")
endif()
