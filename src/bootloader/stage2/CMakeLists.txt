set(CMAKE_CXX_STANDARD_LIBRARIES "")
set(CMAKE_C_STANDARD_LIBRARIES "")


file(GLOB_RECURSE BOOTLOADER_STAGE2_ASM_SOURCE *.asm)
file(GLOB_RECURSE BOOTLOADER_STAGE2_C_SOURCE *.c)
file(GLOB_RECURSE BOOTLOADER_STAGE2_CXX_SOURCE *.cpp)
set(BOOTLOADER_STAGE2_NAME boot_stage2)
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld)

set_source_files_properties(${BOOTLOADER_STAGE2_C_SOURCE} PROPERTIES
        COMPILE_FLAGS "-ffreestanding -fno-exceptions -fno-rtti -I${CMAKE_CURRENT_SOURCE_DIR} -fno-use-cxa-atexit"
)

set_source_files_properties(${BOOTLOADER_STAGE2_CXX_SOURCE} PROPERTIES
        COMPILE_FLAGS "-ffreestanding -fno-exceptions -fno-rtti -I${CMAKE_CURRENT_SOURCE_DIR} -fno-use-cxa-atexit"
)

foreach(src ${BOOTLOADER_STAGE2_ASM_SOURCE})
    if(src MATCHES "crti\\.asm$" OR src MATCHES "crtn\\.asm$")
        list(REMOVE_ITEM BOOTLOADER_STAGE2_ASM_SOURCE ${src})
    endif()
endforeach()

set(STAGE2_SRC ${BOOTLOADER_STAGE2_ASM_SOURCE} ${BOOTLOADER_STAGE2_C_SOURCE} ${BOOTLOADER_STAGE2_CXX_SOURCE})

set(CRTBEGIN_OBJ ${CMAKE_SOURCE_DIR}/${PREFIX_DIR}/lib/gcc/x86_64-elf/13.2.0/crtbegin.o)
set(CRTEND_OBJ   ${CMAKE_SOURCE_DIR}/${PREFIX_DIR}/lib/gcc/x86_64-elf/13.2.0/crtend.o)

# execute_process(
#     COMMAND ${CMAKE_C_COMPILER} -print-file-name=crtbegin.o
#     OUTPUT_VARIABLE CRTBEGIN_OBJ_PATH
#     OUTPUT_STRIP_TRAILING_WHITESPACE
# )

# execute_process(
#     COMMAND ${CMAKE_C_COMPILER} -print-file-name=crtend.o
#     OUTPUT_VARIABLE CRTEND_OBJ_PATH
#     OUTPUT_STRIP_TRAILING_WHITESPACE
# )

# message(STATUS "Found crtbegin.o at: ${CRTBEGIN_OBJ_PATH}")
# message(STATUS "Found crtend.o at: ${CRTEND_OBJ_PATH}")

# set(CRTBEGIN_OBJ ${CRTBEGIN_OBJ_PATH})
# set(CRTEND_OBJ ${CRTEND_OBJ_PATH})

set(CRTI_SRC ${CMAKE_CURRENT_SOURCE_DIR}/crti.asm)
set(CRTN_SRC ${CMAKE_CURRENT_SOURCE_DIR}/crtn.asm)

set(CRTI_OBJ ${CMAKE_CURRENT_BINARY_DIR}/crti.o)
set(CRTN_OBJ ${CMAKE_CURRENT_BINARY_DIR}/crtn.o)

add_custom_command(
    OUTPUT ${CRTI_OBJ}
    COMMAND nasm -f elf64 ${CRTI_SRC} -o ${CRTI_OBJ}
    DEPENDS ${CRTI_SRC}
    COMMENT "Assembling crti.o with nasm"
)

# Assemble crtn.o using yasm.
add_custom_command(
    OUTPUT ${CRTN_OBJ}
    COMMAND nasm -f elf64 ${CRTN_SRC} -o ${CRTN_OBJ}
    DEPENDS ${CRTN_SRC}
    COMMENT "Assembling crtn.o with nasm"
)

add_custom_target(crt_objects ALL 
    DEPENDS ${CRTI_OBJ} ${CRTN_OBJ}
)

set_source_files_properties(${BOOTLOADER_STAGE2_ASM_SOURCE} PROPERTIES LANGUAGE ASM_NASM)

add_executable(${BOOTLOADER_STAGE2_NAME} ${STAGE2_SRC})

add_dependencies(${BOOTLOADER_STAGE2_NAME} crt_objects)

set_target_properties(${BOOTLOADER_STAGE2_NAME} PROPERTIES
    LINKER_LANGUAGE CXX
    LINK_SEARCH_START_STATIC TRUE
    LINK_SEARCH_END_STATIC TRUE
    LINK_LIBRARIES ""                     
    INTERFACE_LINK_LIBRARIES ""
)

target_link_options(${BOOTLOADER_STAGE2_NAME} PRIVATE
    -Wl,-T ${LINKER_SCRIPT}
    -nostdlib
    -no-pie
    -Wl,-Map=stage2.map
    -Wl,--no-warn-rwx-segments
    -Wl,--build-id=none
    -Wl,-z,noexecstack
    ${CRTI_OBJ}
    ${CRTBEGIN_OBJ}
)

set(CMAKE_CXX_LINK_EXECUTABLE
    "<CMAKE_CXX_COMPILER> <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> ${LINK_LIBRARIES} ${CRTEND_OBJ} ${CRTN_OBJ} -o <TARGET>"
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_custom_command(TARGET ${BOOTLOADER_STAGE2_NAME} POST_BUILD
        COMMAND ${CMAKE_STRIP} $<TARGET_FILE:${BOOTLOADER_STAGE2_NAME}>
        COMMENT "Stripping symbols from release binary"
    )
endif()

message(STATUS "Configuring Bootloader Stage 2: ${STAGE2_SRC} -> ${BOOTLOADER_STAGE2_NAME}")