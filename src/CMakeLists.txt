add_subdirectory(bootloader)
add_subdirectory(kernel)

set(STAGE1_BIN ${CMAKE_CURRENT_BINARY_DIR}/bootloader/stage1/boot_stage1)
set(STAGE1_MAP ${CMAKE_CURRENT_BINARY_DIR}/bootloader/stage1/stage1.map)
set(STAGE2_ELF ${CMAKE_CURRENT_BINARY_DIR}/bootloader/stage2/boot_stage2)
set(KERNEL_ELF ${CMAKE_CURRENT_BINARY_DIR}/kernel/kernel.elf)

set(IMAGE_FILE ${CMAKE_CURRENT_BINARY_DIR}/boot.img)
set(VDI_IMAGE ${CMAKE_CURRENT_BINARY_DIR}/boot.vdi)

math(EXPR STAGE2_SECTORS "((${STAGE2_SIZE} + 511) / 512)")
math(EXPR RESERVED_SECTORS "${STAGE2_SECTORS} + 20")  # 2 = 1 for stage1 + 1 extra buffer

set(DISK_SIZE_SECTORS 2880)  # 1.44 mb floppy
#set(DISK_SIZE_SECTORS 131072)  # 64mib hhd
set(SECTOR_SIZE 512)
math(EXPR DISK_SIZE_BYTES "${DISK_SIZE_SECTORS} * ${SECTOR_SIZE}")

set(FS_TYPE "fat")  # can be [fat]
set(FAT_TYPE "-F 12") # can be -F [12, 16, 32]

add_custom_command(
    OUTPUT ${IMAGE_FILE}
    COMMENT "Creating ${FS_TYPE}${FAT_TYPE} disk image with custom bootloader and filesystem..."

    # Step 1: Create empty floppy image
    COMMAND dd if=/dev/zero of=${IMAGE_FILE} bs=${SECTOR_SIZE} count=${DISK_SIZE_SECTORS}

    # Step 2: Compute reserved sector count (stage1 + stage2 in sectors)
    # Use bash-style arithmetic: ceil(stage2 size / 512) + 2
    # We add +2: sector 0 for boot sector, sector 1 for rest of stage1

    COMMAND bash -c "RESERVED=\$(( ( ( $(stat -c%s '${STAGE2_ELF}') + 511 ) / 512 ) + 2 )); \
                     echo Using RESERVED_SECTORS=\$RESERVED; \
                     mkfs.${FS_TYPE} -a ${FAT_TYPE} -R \$RESERVED '${IMAGE_FILE}'"

    # Step 3: Inject JMP instruction (first 3 bytes of stage1)
    COMMAND dd if=${STAGE1_BIN} of=${IMAGE_FILE} bs=1 count=3 conv=notrunc

    # Step 4: Inject rest of stage1 after BPB (skip ~62 bytes)
    COMMAND dd if=${STAGE1_BIN} of=${IMAGE_FILE} bs=1 skip=62 seek=62 conv=notrunc

    # Step 5: Inject stage2 starting at sector 2
    COMMAND dd if=${STAGE2_ELF} of=${IMAGE_FILE} bs=512 seek=2 conv=notrunc

    COMMAND mcopy -i ${IMAGE_FILE} ${KERNEL_ELF} ::kernel.elf

    DEPENDS ${STAGE1_BIN} ${STAGE2_ELF} ${KERNEL_ELF} patch_stage2_info
    VERBATIM
)


# Check if VBoxManage exists
find_program(VBOXMANAGE_EXE VBoxManage)

if (VBOXMANAGE_EXE)
    message(STATUS "VBoxManage found at: ${VBOXMANAGE_EXE}")
    
    add_custom_command(
        OUTPUT ${VDI_IMAGE}
        DEPENDS ${IMAGE_FILE}
        COMMAND ${CMAKE_COMMAND} -E echo "Detaching old VDI from all VMs \\(if attached\\)..."
        COMMAND ${VBOXMANAGE_EXE} list vms | cut -d'{' -f2 | cut -d'}' -f1 |
                xargs -I{} ${VBOXMANAGE_EXE} storageattach {} --storagectl "PIIX4" --port 0 --device 0 --medium none || true
        COMMAND ${CMAKE_COMMAND} -E echo "Closing and deleting old VDI \\(if exists\\)..."
        COMMAND ${VBOXMANAGE_EXE} closemedium disk ${VDI_IMAGE} --delete || true
        COMMAND ${CMAKE_COMMAND} -E echo "Converting raw image to VDI..."
        COMMAND ${VBOXMANAGE_EXE} convertfromraw ${IMAGE_FILE} ${VDI_IMAGE} --format VDI
        COMMENT "Regenerating VDI image from raw boot image..."
    )

    add_custom_target(convert_to_vdi ALL DEPENDS ${VDI_IMAGE})
    add_dependencies(convert_to_vdi generate_image)

else()
    message(WARNING "VBoxManage not found. VDI conversion will be skipped.")
endif()

add_custom_target(generate_image ALL
    DEPENDS ${IMAGE_FILE}
)

message(STATUS "Included 'bootloader' and 'kernel' sub-projects from 'src'.")