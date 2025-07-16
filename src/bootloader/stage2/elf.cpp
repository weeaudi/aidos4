#include "elf.hpp"
#include "memory.hpp"

struct Elf64_Ehdr {
    uint8_t  e_ident[16]; // Magic, class, data encoding, etc.
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

struct Elf64_Phdr {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
};

enum {
    PT_NULL = 0,
    PT_LOAD = 1,
    // others ignored for now
};

ElfInfo readElf(FileSystem* fs, File* file, void* data) {
    Elf64_Ehdr elfHeader;
    fs->seek(file, 0);
    fs->read(file, sizeof(elfHeader), &elfHeader);

    // Validate ELF magic
    if (!(elfHeader.e_ident[0] == 0x7F &&
          elfHeader.e_ident[1] == 'E' &&
          elfHeader.e_ident[2] == 'L' &&
          elfHeader.e_ident[3] == 'F')) {
        return {0, 0};
    }

    uint64_t physBase = (uint64_t)-1;

    for (int i = 0; i < elfHeader.e_phnum; ++i) {
        fs->seek(file, elfHeader.e_phoff + i * elfHeader.e_phentsize);
        Elf64_Phdr phdr;
        fs->read(file, sizeof(phdr), &phdr);

        if (phdr.p_type != PT_LOAD)
            continue;

        // Compute physical location inside 'data' for this segment
        uint64_t phys = reinterpret_cast<uint64_t>(data) + phdr.p_paddr;

        if (phys < physBase)
            physBase = phys;
        uint64_t virt = phdr.p_vaddr;
        uint64_t size = phdr.p_memsz;

        // === Step 1: Map virtual to physical ===
        constexpr uint64_t PAGE_4K = 0x1000;
        constexpr uint64_t PAGE_2M = 0x200000;

        if ((virt % PAGE_2M == 0) && (phys % PAGE_2M == 0) && (size % PAGE_2M == 0)) {
            map_2m(virt, phys, size / PAGE_2M);
        } else {
            map_4k(virt, phys, (size + PAGE_4K - 1) / PAGE_4K);
        }

        // === Step 2: Copy segment from file ===
        fs->seek(file, phdr.p_offset);
        fs->read(file, phdr.p_filesz, reinterpret_cast<void*>(virt));

        // === Step 3: Zero the .bss ===
        if (phdr.p_memsz > phdr.p_filesz) {
            memset(reinterpret_cast<void*>(virt + phdr.p_filesz), 0, phdr.p_memsz - phdr.p_filesz);
        }
    }

    ElfInfo info{};
    info.entry = elfHeader.e_entry;
    info.phys_base = (physBase == (uint64_t)-1) ? (uint64_t)data : physBase;
    return info;
}