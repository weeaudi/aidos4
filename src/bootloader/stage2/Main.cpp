#include "dev/io/text/vga/vga.hpp"
#include "dev/io/disk/ata.hpp"
#include "stdio.hpp"
#include "globals.hpp"
#include "dev/io/fs/fs.hpp"
#include "memory.hpp"
#include "elf.hpp"

VGADriver vga;
ATADrive ata;

#pragma pack(push, 1)         

struct MemoryRegion
{
    uint64_t Base;
    uint64_t Length;
    uint32_t type;
    uint32_t extra;
};


struct BootInfo
{
    MemoryRegion    mem_map[2048 / sizeof(MemoryRegion)];
    uint8_t         reserved[2048 - ((2048 / sizeof(MemoryRegion)) * sizeof(MemoryRegion))];  
    uint32_t        mem_map_count;  
    uint32_t        rsdp_addr;      
    uint32_t        ebda;           
    uint8_t         boot_drive;     
};

struct BootInfoExtended
{
    uint8_t memoryMapIndex;
    uint64_t* PML4Address;
    uint64_t kernelPhysicalBase;
};

#pragma pack(pop)

extern "C" BootInfo boot_info;
BootInfoExtended bootInfoExtended;

#define HEADS 2
#define SECTORS 18

extern "C" void stage2_main() {

    setup_paging();

    bootInfoExtended.PML4Address = (uint64_t*)getPML4();

    vga.initialize();

    if(!ata.initialize()){
        stdio::printf(vga, "ERROR: FAILED TO INITIALIZE ATA DRIVER");
        while(true){}
    }

    FileSystem* fs = detectAndMount(static_cast<BlockIODevice*>(&ata));

    if(fs == nullptr){
        stdio::printf(vga, "ERROR: FAILED TO INITIALIZE FILESYSTEM ON DRIVE");
        while(true){}
    }

    File* hello = fs->open("/kernel.elf");

    if(hello == nullptr) {
        stdio::printf(vga, "ERROR: Could not load kernel.elf!");
        while(true){}
    }

    void* kernelStart = (void*)0x0;

    for (int i = 0; i < boot_info.mem_map_count; i++) {
        if (boot_info.mem_map[i].Base >= 0x100000){
            if (boot_info.mem_map[i].Length >= 0x100000){
                if(boot_info.mem_map[i].type == 1){
                    kernelStart = (void*)boot_info.mem_map[i].Base;
                    bootInfoExtended.memoryMapIndex = i;
                    break;
                }
            }
        }
    }

    if(kernelStart == nullptr) {
        stdio::printf(vga, "ERROR: Could not find any memory segments for kernel!");
        while(true){}
    }

    ElfInfo info = readElf(fs, hello, kernelStart);
    bootInfoExtended.kernelPhysicalBase = info.phys_base;
    void* entry = (void*)info.entry;

    stdio::printf(vga, "Stage 2 cpp loaded!");

    ((void (*)(BootInfo*, BootInfoExtended*))entry)(&boot_info, &bootInfoExtended); // VODO MAGIC (call the kernel);

    while(true){}
}
