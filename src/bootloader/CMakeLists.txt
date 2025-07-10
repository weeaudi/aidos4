message(STATUS "Configuring Bootloader Subproject")

add_subdirectory(stage1)
add_subdirectory(stage2)

set(STAGE1_BIN ${CMAKE_CURRENT_BINARY_DIR}/stage1/boot_stage1)
set(STAGE1_MAP ${CMAKE_CURRENT_BINARY_DIR}/stage1/stage1.map)
set(STAGE2_ELF ${CMAKE_CURRENT_BINARY_DIR}/stage2/boot_stage2)

message(STATUS "Configuring Bootloader Interconnection")

find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_custom_command(
    OUTPUT ${STAGE1_BIN}_patched
    COMMAND ${CMAKE_COMMAND} -E echo "[+] Patching stage1 binary with stage2 info..."
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/Install_Stage2_info.py
            ${STAGE1_BIN} ${STAGE1_MAP} ${STAGE2_ELF}
    COMMAND ${CMAKE_COMMAND} -E touch ${STAGE1_BIN}_patched
    DEPENDS boot_stage1 boot_stage2
    COMMENT "Running Install_Stage2_info.py"
    VERBATIM
)

add_custom_target(patch_stage2_info ALL
    DEPENDS ${STAGE1_BIN}_patched
)

set(IMAGE_FILE ${CMAKE_CURRENT_BINARY_DIR}/boot.img)

add_custom_command(
    OUTPUT ${IMAGE_FILE}
    COMMAND dd if=/dev/zero     of=${IMAGE_FILE} bs=512 count=2880
    COMMAND dd if=${STAGE1_BIN} of=${IMAGE_FILE} bs=512 count=2 conv=notrunc
    COMMAND dd if=${STAGE2_ELF} of=${IMAGE_FILE} bs=512 seek=2 conv=notrunc
    DEPENDS boot_stage1 boot_stage2 patch_stage2_info
    COMMENT "Creating Disk Image"
)

add_custom_target(generate_image ALL
    DEPENDS ${IMAGE_FILE}
)