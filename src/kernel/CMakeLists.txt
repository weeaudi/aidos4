set(CMAKE_CXX_STANDARD_LIBRARIES "")
set(CMAKE_C_STANDARD_LIBRARIES "")


file(GLOB_RECURSE KERNEL_ASM_SOURCE *.asm)
file(GLOB_RECURSE KERNEL_C_SOURCE *.c)
file(GLOB_RECURSE KERNEL_CXX_SOURCE *.cpp)
set(KERNEL_NAME kernel.elf)
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld)

set_source_files_properties(${KERNEL_C_SOURCE} PROPERTIES
        COMPILE_FLAGS "-ffreestanding -fno-exceptions -mcmodel=large -mno-red-zone -fno-rtti -I${CMAKE_CURRENT_SOURCE_DIR} -fno-use-cxa-atexit"
)

set_source_files_properties(${KERNEL_CXX_SOURCE} PROPERTIES
        COMPILE_FLAGS "-ffreestanding -fno-exceptions -mcmodel=large -mno-red-zone -fno-rtti -I${CMAKE_CURRENT_SOURCE_DIR} -fno-use-cxa-atexit"
)

foreach(src ${KERNEL_ASM_SOURCE})
    if(src MATCHES "crti\\.asm$" OR src MATCHES "crtn\\.asm$")
        list(REMOVE_ITEM KERNEL_ASM_SOURCE ${src})
    endif()
endforeach()

set(KERNEL_SRC ${KERNEL_ASM_SOURCE} ${KERNEL_C_SOURCE} ${KERNEL_CXX_SOURCE})

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

add_custom_target(crt_objects_kernel ALL 
    DEPENDS ${CRTI_OBJ} ${CRTN_OBJ}
)

set_source_files_properties(${KERNEL_ASM_SOURCE} PROPERTIES LANGUAGE ASM_NASM)

add_executable(${KERNEL_NAME} ${KERNEL_SRC})

add_dependencies(${KERNEL_NAME} crt_objects_kernel)

set_target_properties(${KERNEL_NAME} PROPERTIES
    LINKER_LANGUAGE CXX
    LINK_SEARCH_START_STATIC TRUE
    LINK_SEARCH_END_STATIC TRUE
    LINK_LIBRARIES ""                     
    INTERFACE_LINK_LIBRARIES ""
)

target_link_options(${KERNEL_NAME} PRIVATE
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
    add_custom_command(TARGET ${KERNEL_NAME} POST_BUILD
        COMMAND ${CMAKE_STRIP} $<TARGET_FILE:${KERNEL_NAME}>
        COMMENT "Stripping symbols from release binary"
    )
endif()

message(STATUS "Configuring Kernel: ${KERNEL_SRC} -> ${KERNEL_NAME}")