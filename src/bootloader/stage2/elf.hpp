#include <stdint.h>
#include "dev/io/fs/fs.hpp"

/**
 * @file elf.hpp
 * @brief Simple ELF loader utilities used by the bootloader.
 */

/**
 * @brief Information about a loaded ELF binary.
 */
struct ElfInfo {
    uint64_t entry;        ///< Entry point of the loaded ELF
    uint64_t phys_base;    ///< Physical base address where the ELF was loaded
};

/**
 * @brief Read and load an ELF image from the given file system.
 */
ElfInfo readElf(FileSystem* fs, File* file, void* data);