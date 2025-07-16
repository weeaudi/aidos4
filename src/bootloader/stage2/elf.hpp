#include <stdint.h>
#include "dev/io/fs/fs.hpp"

struct ElfInfo {
    uint64_t entry;        // Entry point of the loaded ELF
    uint64_t phys_base;    // Physical base address where the ELF was loaded
};

ElfInfo readElf(FileSystem* fs, File* file, void* data);